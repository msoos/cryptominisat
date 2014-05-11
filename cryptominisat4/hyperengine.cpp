/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2013, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#include "hyperengine.h"
#include "clauseallocator.h"

using namespace CMSat;

HyperEngine::HyperEngine(const SolverConf& _conf, bool* _needToInterrupt) :
    PropEngine(_conf, _needToInterrupt)
    , stampingTime(0)
{
}

Lit HyperEngine::propagateFullBFS(const uint64_t timeout)
{
    timedOutPropagateFull = false;
    propStats.otfHyperPropCalled++;
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Prop full BFS started" << endl;
    #endif

    PropBy confl;

    //Assert startup: only 1 enqueued, uselessBin is empty
    assert(uselessBin.empty());
    assert(decisionLevel() == 1);

    //The toplevel decision has to be set specifically
    //If we came here as part of a backtrack to decision level 1, then
    //this is already set, and there is no need to set it
    if (trail.size() - trail_lim.back() == 1) {
        //Set up root node
        Lit root = trail[qhead];
        varData[root.var()].reason = PropBy(~lit_Undef, false, false, false);
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
        const Lit p = trail[nlBinQHead++];
        watch_subarray_const ws = watches[(~p).toInt()];
        propStats.bogoProps += 1;
        for(watch_subarray_const::const_iterator
            k = ws.begin(), end = ws.end()
            ; k != end
            ; k++
        ) {

            //If something other than irred binary, skip
            if (!k->isBinary() || k->red())
                continue;

            ret = propBin(p, k, confl);
            if (ret == PROP_FAIL)
                return analyzeFail(confl);

        }
        propStats.bogoProps += ws.size()*4;
    }

    //Propagate binary redundant
    ret = PROP_NOTHING;
    while (lBinQHead < trail.size()) {
        const Lit p = trail[lBinQHead];
        watch_subarray_const ws = watches[(~p).toInt()];
        propStats.bogoProps += 1;
        size_t done = 0;

        for(watch_subarray::const_iterator k = ws.begin(), end = ws.end(); k != end; k++, done++) {

            //If something other than redundant binary, skip
            if (!k->isBinary() || !k->red())
                continue;

            ret = propBin(p, k, confl);
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
        const Lit p = trail[qhead];
        watch_subarray ws = watches[(~p).toInt()];
        propStats.bogoProps += 1;

        watch_subarray::iterator i = ws.begin();
        watch_subarray::iterator j = ws.begin();
        watch_subarray_const::const_iterator end = ws.end();
        for(; i != end; i++) {
            if (i->isBinary()) {
                *j++ = *i;
                continue;
            }

            if (i->isTri()) {
                *j++ = *i;
                ret = propTriClauseComplex(i, p, confl);
                if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                    i++;
                    break;
                } else {
                    assert(ret == PROP_NOTHING);
                    continue;
                }
            }

            if (i->isClause()) {
                ret = propNormalClauseComplex(i, j, p, confl);
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

Lit HyperEngine::prop_red_bin_dfs(
    const StampType stampType
    , PropBy& confl
    , Lit& root
    , bool& restart
) {
    propStats.bogoProps += 1;

    const Lit p = toPropRedBin.top();
    watch_subarray_const ws = watches[(~p).toInt()];
    size_t done = 0;
    for(watch_subarray::const_iterator
        k = ws.begin(), end = ws.end()
        ; k != end
        ; k++, done++
    ) {
        propStats.bogoProps += 1;

        //If something other than redundant binary, skip
        if (!k->isBinary() || !k->red())
            continue;

        PropResult ret = propBin(p, k, confl);
        switch(ret) {
            case PROP_FAIL:
                closeAllTimestamps(stampType);
                return analyzeFail(confl);

            case PROP_SOMETHING:
                propStats.bogoProps += 8;
                stampingTime++;
                stamp.tstamp[trail.back().toInt()].start[stampType] = stampingTime;

                //No need to set it. Old setting is either the same or better than lit_Undef
                //timestamp[trail.back().toInt()].dominator[stampType] = lit_Undef;

                //Root for literals propagated afterwards will be this literal
                root = trail.back();

                #ifdef DEBUG_STAMPING
                cout
                << "From " << p << " enqueued " << trail.back()
                << " for stampingTime " << stampingTime
                << endl;
                #endif

                toPropNorm.push(trail.back());
                toPropBin.push(trail.back());
                toPropRedBin.push(trail.back());
                propStats.bogoProps += done*4;
                restart = true;
                return lit_Undef;

            case PROP_NOTHING:
                break;

            default:
                assert(false);
                break;
        }
    }

    //Finished with this literal of this type
    propStats.bogoProps += ws.size()*4;
    toPropRedBin.pop();

    return lit_Undef;
}

Lit HyperEngine::prop_norm_bin_dfs(
    StampType stampType
    , PropBy& confl
    , const Lit root
    , bool& restart
) {
    const Lit p = toPropBin.top();
    watch_subarray_const ws = watches[(~p).toInt()];
    size_t done = 0;
    for(watch_subarray::const_iterator
        k = ws.begin(), end = ws.end()
        ; k != end
        ; k++, done++
    ) {
        propStats.bogoProps += 1;
        //Pre-fetch long clause
        if (k->isClause()) {
            if (value(k->getBlockedLit()) != l_True) {
                const ClOffset offset = k->getOffset();
                __builtin_prefetch(clAllocator.getPointer(offset));
            }

            continue;
        } //end CLAUSE

        //If something other than binary, skip
        if (!k->isBinary())
            continue;

        //If stamping only irred, go over red binaries
        if (stampType == STAMP_IRRED
            && k->red()
        ) {
            continue;
        }

        PropResult ret = propBin(p, k, confl);
        switch(ret) {
            case PROP_FAIL:
                closeAllTimestamps(stampType);
                return analyzeFail(confl);

            case PROP_SOMETHING:
                propStats.bogoProps += 8;
                stampingTime++;
                stamp.tstamp[trail.back().toInt()].start[stampType] = stampingTime;
                stamp.tstamp[trail.back().toInt()].dominator[stampType] = root;
                #ifdef DEBUG_STAMPING
                cout
                << "From " << p << " enqueued " << trail.back()
                << " for stampingTime " << stampingTime
                << endl;
                #endif

                toPropNorm.push(trail.back());
                toPropBin.push(trail.back());
                if (stampType == STAMP_IRRED) toPropRedBin.push(trail.back());
                propStats.bogoProps += done*4;
                restart = true;
                return lit_Undef;

            case PROP_NOTHING:
                break;
            default:
                assert(false);
                break;
        }
    }

    //Finished with this literal
    propStats.bogoProps += ws.size()*4;
    toPropBin.pop();
    stampingTime++;
    stamp.tstamp[p.toInt()].end[stampType] = stampingTime;
    #ifdef DEBUG_STAMPING
    cout
    << "End time for " << p
    << " is " << stampingTime
    << endl;
    #endif

    return lit_Undef;
}

Lit HyperEngine::prop_norm_cl_dfs(
    StampType stampType
    , PropBy& confl
    , Lit& root
    , bool& restart
) {
    PropResult ret = PROP_NOTHING;
    const Lit p = toPropNorm.top();
    watch_subarray ws = watches[(~p).toInt()];
    propStats.bogoProps += 1;

    watch_subarray::iterator i = ws.begin();
    watch_subarray::iterator j = ws.begin();
    watch_subarray_const::const_iterator end = ws.end();
    for(; i != end; i++) {
        propStats.bogoProps += 1;
        if (i->isBinary()) {
            *j++ = *i;
            continue;
        }

        if (i->isTri()) {
            *j++ = *i;
            ret = propTriClauseComplex(i, p, confl);
            if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                i++;
                break;
            } else {
                assert(ret == PROP_NOTHING);
                continue;
            }
        }

        if (i->isClause()) {
            ret = propNormalClauseComplex(i, j, p, confl);
            if (ret == PROP_SOMETHING || ret == PROP_FAIL) {
                i++;
                break;
            } else {
                assert(ret == PROP_NOTHING);
                continue;
            }
        }
    }
    while(i != end)
        *j++ = *i++;
    ws.shrink_(end-j);

    switch(ret) {
        case PROP_FAIL:
            closeAllTimestamps(stampType);
            return analyzeFail(confl);

        case PROP_SOMETHING:
            propStats.bogoProps += 8;
            stampingTime++;
            #ifdef DEBUG_STAMPING
            cout
            << "From (long-reduced) " << p << " enqueued << " << trail.back()
            << " for stampingTime " << stampingTime
            << endl;
            #endif
            stamp.tstamp[trail.back().toInt()].start[stampType] = stampingTime;
            if (stampType == STAMP_IRRED) {
                //Root for literals propagated afterwards will be this literal
                root = trail.back();
                toPropRedBin.push(trail.back());
            }

            toPropNorm.push(trail.back());
            toPropBin.push(trail.back());
            propStats.bogoProps += ws.size()*8;
            restart = true;
            return lit_Undef;

        case PROP_NOTHING:
            break;

        default:
            assert(false);
            break;
    }

    //Finished with this literal
    propStats.bogoProps += ws.size()*8;
    toPropNorm.pop();

    return lit_Undef;
}

bool HyperEngine::need_early_abort_dfs(
    StampType stampType
    , const size_t timeout
) {
    //Early-abort if too much time was used (from prober)
    if (propStats.otfHyperTime + propStats.bogoProps > timeout) {
        closeAllTimestamps(stampType);
        timedOutPropagateFull = true;
        return true;
    }
    return false;
}

Lit HyperEngine::propagateFullDFS(
    const StampType stampType
    , const uint64_t timeout
) {
    timedOutPropagateFull = false;
    propStats.otfHyperPropCalled++;
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "Prop full started" << endl;
    #endif

    PropBy confl;

    //Assert startup: only 1 enqueued, uselessBin is empty
    assert(uselessBin.empty());
    assert(decisionLevel() == 1);

    //The toplevel decision has to be set specifically
    //If we came here as part of a backtrack to decision level 1, then
    //this is already set, and there is no need to set it
    if (trail.size() - trail_lim.back() == 1) {
        //Set up root node
        Lit root = trail[qhead];
        varData[root.var()].reason = PropBy(~lit_Undef, false, false, false);
    }

    //Set up stacks
    const size_t origTrailSize = trail.size();
    toPropBin.clear();
    toPropRedBin.clear();
    toPropNorm.clear();

    Lit root = trail.back();
    toPropBin.push(root);
    toPropNorm.push(root);
    if (stampType == STAMP_RED)
        toPropRedBin.push(root);

    //Setup
    needToAddBinClause.clear();
    stampingTime++;
    stamp.tstamp[root.toInt()].start[stampType] = stampingTime;

    #ifdef DEBUG_STAMPING
    cout
    << "Top-enqueued << " << trail.back()
    << " for stampingTime " << stampingTime
    << endl;
    #endif

    while(true) {
        propStats.bogoProps += 3;
        if (need_early_abort_dfs(stampType, timeout))
            return lit_Undef;

        //Propagate binary irred
        bool restart = false;
        while (!toPropBin.empty()) {
            Lit ret = prop_norm_bin_dfs(stampType, confl, root, restart);
            if (ret != lit_Undef)
                return ret;
            if (restart)
                break;
        }
        if (restart)
            continue;

        if (stampType == STAMP_IRRED) {
            while (!toPropRedBin.empty()) {
                Lit ret = prop_red_bin_dfs(stampType, confl, root, restart);
                if (ret != lit_Undef)
                    return ret;
                if (restart)
                    break;
            }
        }
        if (restart)
            continue;

        while (!toPropNorm.empty()) {
            Lit ret = prop_norm_cl_dfs(stampType, confl, root, restart);
            if (ret != lit_Undef)
                return ret;
            if (restart)
                break;

            qhead++;
        }
        if (restart)
            continue;

        //Nothing more to propagate
        break;
    }

    stamp.tstamp[trail.back().toInt()].numDom[stampType] = trail.size() - origTrailSize;

    return lit_Undef;
}


void HyperEngine::closeAllTimestamps(const StampType stampType)
{
    while(!toPropBin.empty())
    {
        stampingTime++;
        stamp.tstamp[toPropBin.top().toInt()].end[stampType] = stampingTime;
        #ifdef DEBUG_STAMPING
        cout
        << "End time for " << toPropBin.top()
        << " is " << stampingTime
        << " (due to failure, closing all nodes)"
        << endl;
        #endif

        toPropBin.pop();
    }
}


//Add binary clause to deepest common ancestor
void HyperEngine::addHyperBin(const Lit p)
{
    propStats.otfHyperTime += 2;

    Lit deepestAncestor = lit_Undef;
    bool hyperBinNotAdded = true;
    if (currAncestors.size() > 1) {
        deepestAncestor = deepestCommonAcestor();

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Adding hyper-bin clause: " << p << " , " << ~deepestAncestor << endl;
        #endif
        needToAddBinClause.insert(BinaryClause(p, ~deepestAncestor, true));
        *drup << p << (~deepestAncestor) << fin;

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

    enqueueComplex(p, deepestAncestor, true);
    varData[p.var()].reason.setHyperbin(true);
    varData[p.var()].reason.setHyperbinNotAdded(hyperBinNotAdded);
}

/**
We can try both ways: either binary clause can be removed.
Try to remove one, then the other
Return which one is to be removed
*/
Lit HyperEngine::removeWhich(
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
    bool ambivalent = varData[thisAncestor.var()].depth == varData[lookingForAncestor.var()].depth;
    if (varData[thisAncestor.var()].depth < varData[lookingForAncestor.var()].depth) {
        second_is_deeper = true;
    }
    #ifdef DEBUG_DEPTH
    cout
    << "1st: " << std::setw(6) << thisAncestor
    << " depth: " << std::setw(4) << varData[thisAncestor.var()].depth
    << "  2nd: " << std::setw(6) << lookingForAncestor
    << " depth: " << std::setw(4) << varData[lookingForAncestor.var()].depth
    ;
    #endif


    if ((ambivalent || !second_is_deeper) &&
        isAncestorOf(
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
        isAncestorOf(
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
bool HyperEngine::isAncestorOf(
    const Lit conflict
    , Lit thisAncestor
    , const bool thisStepRed
    , const bool onlyIrred
    , const Lit lookingForAncestor
) {
    propStats.otfHyperTime += 1;
    #ifdef VERBOSE_DEBUG_FULLPROP
    cout << "isAncestorOf."
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
    const size_t bottom = varData[lookingForAncestor.var()].depth;

    while(thisAncestor != lit_Undef
        && bottom <= varData[thisAncestor.var()].depth
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

void HyperEngine::addHyperBin(const Lit lit1, const Lit lit2, const Lit lit3)
{
    assert(value(lit1.var()) == l_Undef);

    #ifdef VERBOSE_DEBUG_FULLPROP
    print_trail();
    cout << "Enqueing " << lit1
    << " with ancestor 3-long clause: " << lit1 << " , "
    << lit2 << " (lev:" << varData[lit2.var()].level << ") "
    << lit3 << " (lev:" << varData[lit3.var()].level << ") "
    << endl;
    #endif

    assert(value(lit2) == l_False);
    assert(value(lit3) == l_False);

    currAncestors.clear();
    if (varData[lit2.var()].level != 0) {
        currAncestors.push_back(~lit2);
    }

    if (varData[lit3.var()].level != 0)
        currAncestors.push_back(~lit3);

    addHyperBin(lit1);
}

void HyperEngine::addHyperBin(const Lit p, const Clause& cl)
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
        ; it++, i++
    ) {
        if (*it != p) {
            assert(value(*it) == l_False);
            if (varData[it->var()].level != 0)
                currAncestors.push_back(~*it);
        }
    }

    addHyperBin(p);
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
        case tertiary_t : {
            const Lit lit = ~propBy.lit3();
            if (varData[lit.var()].level != 0)
                currAncestors.push_back(lit);
            //intentionally falling through here
            //i.e. there is no 'break' here for a reason
        }
        case binary_t: {
            const Lit lit = ~propBy.lit2();
            if (varData[lit.var()].level != 0)
                currAncestors.push_back(lit);

            if (varData[failBinLit.var()].level != 0)
                currAncestors.push_back(~failBinLit);

            break;
        }

        case clause_t: {
            const uint32_t offset = propBy.getClause();
            const Clause& cl = *clAllocator.getPointer(offset);
            for(size_t i = 0; i < cl.size(); i++) {
                if (varData[cl[i].var()].level != 0)
                    currAncestors.push_back(~cl[i]);
            }
            break;
        }

        case null_clause_t:
            assert(false);
            break;
    }

    Lit foundLit = deepestCommonAcestor();

    return foundLit;
}

Lit HyperEngine::deepestCommonAcestor()
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
            ; it++
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

void HyperEngine::remove_bin_clause(Lit lit)
{
    //The binary clause we should remove
    const BinaryClause clauseToRemove(
        ~varData[lit.var()].reason.getAncestor()
        , lit
        , varData[lit.var()].reason.isRedStep()
    );

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

PropResult HyperEngine::propBin(
    const Lit p
    , watch_subarray::const_iterator k
    , PropBy& confl
) {
    const Lit lit = k->lit2();
    const lbool val = value(lit);
    if (val == l_Undef) {
        #ifdef STATS_NEEDED
        if (k->red())
            propStats.propsBinRed++;
        else
            propStats.propsBinIrred++;
        #endif

        //Never propagated before
        enqueueComplex(lit, p, k->red());
        return PROP_SOMETHING;

    } else if (val == l_False) {
        //Conflict
        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Conflict from " << p << " , " << lit << endl;
        #endif //VERBOSE_DEBUG_FULLPROP

        //Update stats
        if (k->red())
            lastConflictCausedBy = ConflCausedBy::binred;
        else
            lastConflictCausedBy = ConflCausedBy::binirred;

        failBinLit = lit;
        confl = PropBy(~p);
        return PROP_FAIL;

    } else if (varData[lit.var()].level != 0) {
        //Propaged already
        assert(val == l_True);

        #ifdef VERBOSE_DEBUG_FULLPROP
        cout << "Lit " << p << " also wants to propagate " << lit << endl;
        #endif
        Lit remove = removeWhich(lit, p, k->red());

        //Remove this one
        if (remove == p) {
            Lit origAnc = varData[lit.var()].reason.getAncestor();
            assert(origAnc != lit_Undef);

            remove_bin_clause(lit);

            //Update data indicating what lead to lit
            varData[lit.var()].reason = PropBy(~p, k->red(), false, false);
            assert(varData[p.var()].level != 0);
            varData[lit.var()].depth = varData[p.var()].depth + 1;
            //NOTE: we don't update the levels of other literals... :S

            //for correctness, we would need this, but that would need re-writing of history :S
            //if (!onlyIrred) return PropBy();

        } else if (remove != lit_Undef) {
            #ifdef VERBOSE_DEBUG_FULLPROP
            cout << "Removing this bin clause" << endl;
            #endif
            propStats.otfHyperTime += 2;
            uselessBin.insert(BinaryClause(~p, lit, k->red()));
        }
    }

    return PROP_NOTHING;
}


PropResult HyperEngine::propNormalClauseComplex(
    watch_subarray_const::const_iterator i
    , watch_subarray::iterator &j
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
    const ClOffset offset = i->getOffset();
    Clause& c = *clAllocator.getPointer(offset);

    PropResult ret = prop_normal_helper(c, offset, j, p);
    if (ret != PROP_TODO)
        return ret;

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        return handle_normal_prop_fail(c, offset, confl);
    }

    //Update stats
    c.stats.propagations_made++;
    c.stats.sum_of_branch_depth_propagation += decisionLevel() + 1;
    #ifdef STATS_NEEDED
    if (c.red())
        propStats.propsLongRed++;
    else
        propStats.propsLongIrred++;
    #endif

    addHyperBin(c[0], c);

    return PROP_SOMETHING;
}

PropResult HyperEngine::propTriClauseComplex(
    watch_subarray_const::const_iterator i
    , const Lit lit1
    , PropBy& confl
) {
    const Lit lit2 = i->lit2();
    lbool val2 = value(lit2);

    //literal is already satisfied, nothing to do
    if (val2 == l_True)
        return PROP_NOTHING;

    const Lit lit3 = i->lit3();
    lbool val3 = value(lit3);

    //literal is already satisfied, nothing to do
    if (val3 == l_True)
        return PROP_NOTHING;

    if (val2 == l_False && val3 == l_False) {
        return handle_prop_tri_fail(i, lit1, confl);
    }

    if (val2 == l_Undef && val3 == l_False) {
        return propTriHelperComplex(lit2, ~lit1, lit3, i->red());
    }

    if (val3 == l_Undef && val2 == l_False) {
        return propTriHelperComplex(lit3, ~lit1,  lit2, i->red());
    }

    return PROP_NOTHING;
}

PropResult HyperEngine::propTriHelperComplex(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
) {
    #ifdef STATS_NEEDED
    if (red)
        propStats.propsTriRed++;
    else
        propStats.propsTriIrred++;
    #endif

    //Not simple
    addHyperBin(lit1, lit2, lit3);
    return PROP_SOMETHING;
}

size_t HyperEngine::print_stamp_mem(size_t totalMem) const
{
    size_t mem = 0;
    mem += toPropNorm.capacity()*sizeof(Lit);
    mem += toPropBin.capacity()*sizeof(Lit);
    mem += toPropRedBin.capacity()*sizeof(Lit);
    mem += currAncestors.capacity()*sizeof(Lit);
    mem += stamp.memUsed();
    printStatsLine("c Mem for stamps"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );

    return mem;
}

void HyperEngine::enqueueComplex(
    const Lit p
    , const Lit ancestor
    , const bool redStep
) {
    enqueue(p, PropBy(~ancestor, redStep, false, false));

    assert(varData[ancestor.var()].level != 0);

    varData[p.var()].depth = varData[ancestor.var()].depth + 1;
    #if defined(DEBUG_DEPTH) || defined(VERBOSE_DEBUG_FULLPROP)
    cout
    << "Enqueued "
    << std::setw(6) << (p)
    << " by " << std::setw(6) << (~ancestor)
    << " at depth " << std::setw(4) << varData[p.var()].depth
    << " at dec level: " << decisionLevel()
    << endl;
    #endif
}