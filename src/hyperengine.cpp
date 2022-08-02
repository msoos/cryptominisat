/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#include "hyperengine.h"
#include "clauseallocator.h"

using namespace CMSat;

HyperEngine::HyperEngine(
    const SolverConf *_conf
    , Solver* _solver
    , std::atomic<bool>* _must_interrupt_inter) :
    PropEngine(_conf, _solver, _must_interrupt_inter)
{
}

HyperEngine::~HyperEngine() = default;

Lit HyperEngine::propagate_bfs(const uint64_t timeout)
{
    timedOutPropagateFull = false;
    propStats.otfHyperPropCalled++;
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Prop full BFS started" << endl;
    #endif

    PropBy confl;
    assert(uselessBin.empty());

    //The toplevel decision has to be set specifically
    //If we came here as part of a backtrack to decision level 1, then
    //this is already set, and there is no need to set it
    if (trail.size() - trail_lim.back() == 1) {
        //Set up root node
        Lit root = trail[qhead].lit;
        varData[root.var()].reason = PropBy(~lit_Undef, false, false, false, 0);
    }

    uint32_t nlBinQHead = qhead;
    uint32_t lBinQHead = qhead;

    needToAddBinClause.clear();
    PropResult ret = PROP_NOTHING;
    start:

    //Early-abort if too much time was used (from prober)
    if (propStats.otfHyperTime + propStats.bogoProps > timeout) {
        timedOutPropagateFull = true;
        return lit_Undef;
    }

    //Propagate binary irred
    while (nlBinQHead < trail.size()) {
        const Lit p = trail[nlBinQHead++].lit;
        watch_subarray_const ws = watches[~p];
        propStats.bogoProps += 1;
        for(const Watched *k = ws.begin(), *end = ws.end()
            ; k != end
            ; k++
        ) {
            //If something other than irred binary, skip
            if (!k->isBin() || k->red()) continue;
            ret = prop_bin_with_ancestor_info(p, k, confl);
            if (ret == PROP_FAIL)
                return analyzeFail(confl);

        }
        propStats.bogoProps += ws.size()*4;
    }

    //Propagate binary redundant
    ret = PROP_NOTHING;
    while (lBinQHead < trail.size()) {
        const Lit p = trail[lBinQHead].lit;
        watch_subarray_const ws = watches[~p];
        propStats.bogoProps += 1;
        size_t done = 0;

        for(const Watched *k = ws.begin(), *end = ws.end(); k != end; k++, done++) {

            //If something other than redundant binary, skip
            if (!k->isBin() || !k->red())
                continue;

            ret = prop_bin_with_ancestor_info(p, k, confl);
            if (ret == PROP_FAIL) {
                return analyzeFail(confl);
            } else if (ret == PROP_SOMETHING) {
                propStats.bogoProps += done*4;
                goto start;
            } else {
                assert(ret == PROP_NOTHING);
            }
        }
        lBinQHead++;
        propStats.bogoProps += done*4;
    }

    ret = PROP_NOTHING;
    while (qhead < trail.size()) {
        const Lit p = trail[qhead].lit;
        watch_subarray ws = watches[~p];
        propStats.bogoProps += 1;

        Watched* i = ws.begin();
        Watched* j = ws.begin();
        Watched* end = ws.end();
        for(; i != end; i++) {
            if (i->isBin()) {
                *j++ = *i;
                continue;
            }

            if (i->isClause()) {
                ret = prop_normal_cl_with_ancestor_info(i, j, p, confl);
                if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    i++;
                    break;
                } else {
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            }
        }
        propStats.bogoProps += ws.size()*4;
        while(i != end)
            *j++ = *i++;
        ws.shrink_(end-j);

        if (ret == PROP_FAIL) {
            return analyzeFail(confl);
        } else if (ret == PROP_SOMETHING) {
            propStats.bogoProps += ws.size()*4;
            goto start;
        }

        qhead++;
        propStats.bogoProps += ws.size()*4;
    }

    return lit_Undef;
}

//Add binary clause to deepest common ancestor
void HyperEngine::add_hyper_bin(const Lit p)
{
    propStats.otfHyperTime += 2;

    Lit deepestAncestor = lit_Undef;
    bool hyperBinNotAdded = true;
    const int32_t ID = ++clauseID;
    if (currAncestors.size() > 1) {
        deepestAncestor = deepest_common_ancestor();

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Adding hyper-bin clause: " << p << " , "
        << ~deepestAncestor << " ID: " << ID << endl;
        #endif
        needToAddBinClause.insert(BinaryClause(p, ~deepestAncestor, true, ID));

        hyperBinNotAdded = false;
    } else {
        //0-level propagation is NEVER made by propFull
        assert(currAncestors.size() > 0);

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout
        << "Not adding hyper-bin because only ONE lit is not set at"
        << "level 0 in long clause, but that long clause needs to be cleaned"
        << endl;
        #endif
        deepestAncestor = currAncestors[0];
        hyperBinNotAdded = true;
    }

    enqueue_with_acestor_info(p, deepestAncestor, true, ID);
    varData[p.var()].reason.setHyperbin(true);
    varData[p.var()].reason.setHyperbinNotAdded(hyperBinNotAdded);
}

/**
We can try both ways: either binary clause can be removed.
Try to remove one, then the other
Return which one is to be removed
*/
Lit HyperEngine::remove_which_bin_due_to_trans_red(
    Lit conflict
    , Lit thisAncestor
    , bool thisStepRed
) {
    propStats.otfHyperTime += 1;
    const PropBy& data = varData[conflict.var()].reason;

    bool onlyIrred = !data.isRedStep();
    Lit lookingForAncestor = data.getAncestor();

    if (thisAncestor == lit_Undef || lookingForAncestor == lit_Undef)
        return lit_Undef;

    propStats.otfHyperTime += 1;
    bool second_is_deeper = false;
    bool ambivalent = true;
    if (use_depth_trick) {
        ambivalent = depth[thisAncestor.var()] == depth[lookingForAncestor.var()];
        if (depth[thisAncestor.var()] < depth[lookingForAncestor.var()]) {
            second_is_deeper = true;
        }
    }
    #ifdef DEBUG_DEPTH
    cout
    << "1st: " << std::setw(6) << thisAncestor
    << " depth: " << std::setw(4) << depth[thisAncestor.var()]
    << "  2nd: " << std::setw(6) << lookingForAncestor
    << " depth: " << std::setw(4) << depth[lookingForAncestor.var()]
    ;
    #endif


    if ((ambivalent || !second_is_deeper) &&
        is_ancestor_of(
        conflict
        , thisAncestor
        , thisStepRed
        , onlyIrred
        , lookingForAncestor
        )
    ) {
        #ifdef DEBUG_DEPTH
        cout << " -- OK" << endl;
        #endif
        //assert(ambivalent || !second_is_deeper);
        return thisAncestor;
    }

    onlyIrred = !thisStepRed;
    thisStepRed = data.isRedStep();
    std::swap(lookingForAncestor, thisAncestor);
    if ((ambivalent || second_is_deeper) &&
        is_ancestor_of(
        conflict
        , thisAncestor
        , thisStepRed
        , onlyIrred
        , lookingForAncestor
        )
    ) {
        #ifdef DEBUG_DEPTH
        cout << " -- OK" << endl;
        #endif
        //assert(ambivalent || second_is_deeper);
        return thisAncestor;
    }

    #ifdef DEBUG_DEPTH
    cout << " -- NOTK" << endl;
    #endif

    return lit_Undef;
}

/**
hop backwards from thisAncestor until:
1) we reach ancestor of 'conflict' -- at this point, we return TRUE
2) we reach an invalid point. Either root, or an invalid hop. We return FALSE.
*/
bool HyperEngine::is_ancestor_of(
    const Lit conflict
    , Lit thisAncestor
    , const bool thisStepRed
    , const bool onlyIrred
    , const Lit lookingForAncestor
) {
    propStats.otfHyperTime += 1;
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "is_ancestor_of."
    << "conflict: " << conflict
    << " thisAncestor: " << thisAncestor
    << " thisStepRed: " << thisStepRed
    << " onlyIrred: " << onlyIrred
    << " lookingForAncestor: " << lookingForAncestor << endl;
    #endif

    //Was propagated at level 0 -- clauseCleaner will remove the clause
    if (lookingForAncestor == lit_Undef)
        return false;

    if (lookingForAncestor == thisAncestor) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Last position inside prop queue is not saved during propFull" << endl
        << "This may be the same exact binary clause -- not removing" << endl;
        #endif
        return false;
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Looking for ancestor of " << conflict << " : " << lookingForAncestor << endl;
    cout << "This step based on redundant cl? " << (thisStepRed ? "yes" : "false") << endl;
    cout << "Only irred is acceptable?" << (onlyIrred ? "yes" : "no") << endl;
    cout << "This step would be based on redundant cl?" << (thisStepRed ? "yes" : "no") << endl;
    #endif

    if (onlyIrred && thisStepRed) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "This step doesn't work -- is redundant but needs irred" << endl;
        #endif
        return false;
    }

    //This is as low as we should search -- we cannot find what we are searchig for lower than this
    const size_t bottom = depth[lookingForAncestor.var()];

    while(thisAncestor != lit_Undef
        && (!use_depth_trick || bottom <= depth[thisAncestor.var()])
    ) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Current acestor: " << thisAncestor
        << " redundant step? " << varData[thisAncestor.var()].reason.isRedStep()
        << endl;
        #endif

        if (thisAncestor == conflict) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "We are trying to step over the conflict."
            << " That would create a loop." << endl;
            #endif
            return false;
        }

        if (thisAncestor == lookingForAncestor) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Ancestor found" << endl;
            #endif
            return true;
        }

        const PropBy& data = varData[thisAncestor.var()].reason;
        if ((onlyIrred && data.isRedStep())
            || data.getHyperbinNotAdded()
        ) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Wrong kind of hop would be needed" << endl;
            #endif
            return false;  //reached would-be redundant hop (but this is irred)
        }

        thisAncestor = data.getAncestor();
        propStats.otfHyperTime += 1;
    }

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Exit, reached root" << endl;
    #endif

    return false;
}

void HyperEngine::add_hyper_bin(const Lit p, const Clause& cl)
{
    assert(value(p.var()) == l_Undef);

    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Enqueing " << p
    << " with ancestor clause: " << cl
    << endl;
     #endif

    currAncestors.clear();
    size_t i = 0;
    for (Clause::const_iterator
        it = cl.begin(), end = cl.end()
        ; it != end
        ; ++it, i++
    ) {
        if (*it != p) {
            assert(value(*it) == l_False);
            if (varData[it->var()].level != 0)
                currAncestors.push_back(~*it);
        }
    }

    add_hyper_bin(p);
}

//Analyze why did we fail at decision level 1
Lit HyperEngine::analyzeFail(const PropBy propBy)
{
    //Clear out the datastructs we will be usin
    currAncestors.clear();

    //First, we set the ancestors, based on the clause
    //Each literal in the clause is an ancestor. So just 'push' them inside the
    //'currAncestors' variable
    switch(propBy.getType()) {
        case binary_t: {
            const Lit lit = ~propBy.lit2();
            if (varData[lit.var()].level != 0)
                currAncestors.push_back(lit);

            if (varData[failBinLit.var()].level != 0)
                currAncestors.push_back(~failBinLit);

            break;
        }

        case clause_t: {
            const uint32_t offset = propBy.get_offset();
            const Clause& cl = *cl_alloc.ptr(offset);
            for(size_t i = 0; i < cl.size(); i++) {
                if (varData[cl[i].var()].level != 0)
                    currAncestors.push_back(~cl[i]);
            }
            break;
        }

        case xor_t:
        case bnn_t:
        case null_clause_t:
            assert(false);
            break;
    }

    Lit foundLit = deepest_common_ancestor();

    return foundLit;
}

Lit HyperEngine::deepest_common_ancestor()
{
    //Then, we go back on each ancestor recursively, and exit on the first one
    //that unifies ALL the previous ancestors. That is the lowest common ancestor
    assert(toClear.empty());
    Lit foundLit = lit_Undef;
    while(foundLit == lit_Undef) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "LEVEL analyzeFail" << endl;
        #endif
        size_t num_lit_undef = 0;
        for (vector<Lit>::iterator
            it = currAncestors.begin(), end = currAncestors.end()
            ; it != end
            ; ++it
        ) {
            propStats.otfHyperTime += 1;

            //We have reached the top of the graph, the other 'threads' that
            //are still stepping back will find which literal is the lowest
            //common ancestor
            if (*it == lit_Undef) {
                #ifdef VERBOSE_DEBUG_FULLPROP
                cout << "seen lit_Undef" << endl;
                #endif
                num_lit_undef++;
                assert(num_lit_undef != currAncestors.size());
                continue;
            }

            //Increase path count
            seen[it->toInt()]++;

            //Visited counter has to be cleared later, so add it to the
            //to-be-cleared set
            if (seen[it->toInt()] == 1)
                toClear.push_back(*it);

            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "seen " << *it << " : " << seen[it->toInt()] << endl;
            #endif

            //Is this point where all the 'threads' that are stepping backwards
            //reach each other? If so, we have found what we were looking for!
            //We can exit, and return 'foundLit'
            if (seen[it->toInt()] == currAncestors.size()) {
                foundLit = *it;
                break;
            }

            //Update ancestor to its own ancestor, i.e. step up this 'thread'
            *it = varData[it->var()].reason.getAncestor();
        }
    }
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "END" << endl;
    #endif
    assert(foundLit != lit_Undef);

    //Clear nodes we have visited
    propStats.otfHyperTime += toClear.size()/2;
    for(const Lit lit: toClear) {
        seen[lit.toInt()] = 0;
    }
    toClear.clear();

    return foundLit;
}

void HyperEngine::remove_bin_clause(Lit lit, const int32_t ID)
{
    //The binary clause we should remove
    const BinaryClause clauseToRemove(
        ~varData[lit.var()].reason.getAncestor(),
        lit,
        varData[lit.var()].reason.isRedStep(),
        ID);

    //We now remove the clause
    //If it's hyper-bin, then we remove the to-be-added hyper-binary clause
    //However, if the hyper-bin was never added because only 1 literal was unbound at level 0 (i.e. through
    //clause cleaning, the clause would have been 2-long), then we don't do anything.
    if (!varData[lit.var()].reason.getHyperbin()) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Normal removing clause " << clauseToRemove << endl;
        #endif
        propStats.otfHyperTime += 2;
        uselessBin.insert(clauseToRemove);
    } else if (!varData[lit.var()].reason.getHyperbinNotAdded()) {
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Removing hyper-bin clause " << clauseToRemove << endl;
        #endif
        propStats.otfHyperTime += needToAddBinClause.size()/4;
        std::set<BinaryClause>::iterator it = needToAddBinClause.find(clauseToRemove);

        //In case this is called after a backtrack to decisionLevel 1
        //then in fact we might have already cleaned the
        //'needToAddBinClause'. When called from probing, the IF below
        //must ALWAYS be true
        if (it != needToAddBinClause.end()) {
            propStats.otfHyperTime += 2;
            needToAddBinClause.erase(it);
        }
        //This will subsume the clause later, so don't remove it
    }
}

PropResult HyperEngine::prop_bin_with_ancestor_info(
    const Lit p
    , const Watched* k
    , PropBy& confl
) {
    const Lit lit = k->lit2();
    const lbool val = value(lit);
    if (val == l_Undef) {
        //Never propagated before
        enqueue_with_acestor_info(lit, p, k->red(), k->get_ID());
        return PROP_SOMETHING;

    } else if (val == l_False) {
        //Conflict
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from " << p << " , " << lit << endl;
        #endif

        failBinLit = lit;
        confl = PropBy(~p, k->red(), k->get_ID());
        return PROP_FAIL;

    } else if (varData[lit.var()].level != 0 && perform_transitive_reduction) {
        //Propaged already
        assert(val == l_True);

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Lit " << p << " also wants to propagate " << lit << endl;
        #endif
        Lit remove = remove_which_bin_due_to_trans_red(lit, p, k->red());

        //Remove this one
        if (remove == p) {
            const Lit origAnc = varData[lit.var()].reason.getAncestor();
            const int32_t origID = varData[lit.var()].reason.getID();
            assert(origAnc != lit_Undef);
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "ID of k: " << k->get_ID() << " ID of orig: " << origID << " removing latter, origAnc: " << origAnc << endl;
            #endif

            remove_bin_clause(lit, origID);

            //Update data indicating what lead to lit
            varData[lit.var()].reason = PropBy(~p, k->red(), false, false, k->get_ID());
            assert(varData[p.var()].level != 0);
            depth[lit.var()] = depth[p.var()] + 1;
            //NOTE: we don't update the levels of other literals... :S

            //for correctness, we would need this, but that would need re-writing of history :S
            //if (!onlyIrred) return PropBy();

        } else if (remove != lit_Undef) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removing this bin clause, ID: " << k->get_ID() << endl;
            #endif
            propStats.otfHyperTime += 2;
            uselessBin.insert(BinaryClause(~p, lit, k->red(), k->get_ID()));
        }
    }

    return PROP_NOTHING;
}


PropResult HyperEngine::prop_normal_cl_with_ancestor_info(
    Watched* i
    , Watched*& j
    , const Lit p
    , PropBy& confl
) {
    //Blocked literal is satisfied, so clause is satisfied
    if (value(i->getBlockedLit()) == l_True) {
        *j++ = *i;
        return PROP_NOTHING;
    }

    //Dereference pointer
    propStats.bogoProps += 4;
    const ClOffset offset = i->get_offset();
    Clause& c = *cl_alloc.ptr(offset);

    PropResult ret = prop_normal_helper<true>(c, offset, j, p);
    if (ret != PROP_TODO)
        return ret;

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        return handle_normal_prop_fail<true>(c, offset, confl);
    }

    add_hyper_bin(c[0], c);

    return PROP_SOMETHING;
}

size_t HyperEngine::mem_used() const
{
    size_t mem = 0;
    mem += PropEngine::mem_used();
    mem += currAncestors.capacity()*sizeof(Lit);

    return mem;
}

void HyperEngine::enqueue_with_acestor_info(
    const Lit p
    , const Lit ancestor
    , const bool redStep
    , const int32_t ID
) {
    //only called at decision level 1 during solving OR
    //during intree probing
    enqueue<true>(p, decisionLevel(), PropBy(~ancestor, redStep, false, false, ID));

    assert(varData[ancestor.var()].level != 0);

    if (use_depth_trick) {
        depth[p.var()] = depth[ancestor.var()] + 1;
    } else {
        depth[p.var()] = 0;
    }
    #if defined(DEBUG_DEPTH) || defined(VERBOSE_DEBUG_FULLPROP)
    cout
    << "Enqueued " << std::setw(6) << (p)
    << " by " << std::setw(6) << (~ancestor)
    << " ID: " << ID
    << " at depth " << std::setw(4) << depth[p.var()]
    << " at dec level: " << decisionLevel()
    << endl;
    #endif
}
