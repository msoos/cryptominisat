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
    prop_stats.otfHyperPropCalled++;

    PropBy confl;
    assert(uselessBin.empty());

    //The toplevel decision has to be set specifically
    //If we came here as part of a backtrack to decision level 1, then
    //this is already set, and there is no need to set it
    if (trail.size() - trail_lim.back() == 1) {
        //Set up root node
        Lit root = trail[qhead].lit;
        var_data[root.var()].reason = PropBy(~lit_Undef, false, false, false, 0);
    }

    uint32_t nlBinQHead = qhead;
    uint32_t lBinQHead = qhead;

    need_to_add_bin_clause.clear();
    PropResult ret = PROP_NOTHING;
    start:

    //Early-abort if too much time was used (from prober)
    if (prop_stats.otf_hyper_time + prop_stats.bogo_props > timeout) {
        timedOutPropagateFull = true;
        return lit_Undef;
    }

    //Propagate binary irred
    while (nlBinQHead < trail.size()) {
        const Lit p = trail[nlBinQHead++].lit;
        watch_subarray_const ws = watches[~p];
        prop_stats.bogo_props += 1;
        for(const Watched *k = ws.begin(), *end = ws.end()
            ; k != end
            ; k++
        ) {
            //If something other than irred binary, skip
            if (!k->is_bin() || k->red()) continue;
            ret = prop_bin_with_ancestor_info(p, k, confl);
            if (ret == PROP_FAIL)
                return analyzeFail(confl);

        }
        prop_stats.bogo_props += ws.size()*4;
    }

    //Propagate binary redundant
    ret = PROP_NOTHING;
    while (lBinQHead < trail.size()) {
        const Lit p = trail[lBinQHead].lit;
        watch_subarray_const ws = watches[~p];
        prop_stats.bogo_props += 1;
        size_t done = 0;

        for(const Watched *k = ws.begin(), *end = ws.end(); k != end; k++, done++) {

            //If something other than redundant binary, skip
            if (!k->is_bin() || !k->red())
                continue;

            ret = prop_bin_with_ancestor_info(p, k, confl);
            if (ret == PROP_FAIL) {
                return analyzeFail(confl);
            } else if (ret == PROP_SOMETHING) {
                prop_stats.bogo_props += done*4;
                goto start;
            } else {
                assert(ret == PROP_NOTHING);
            }
        }
        lBinQHead++;
        prop_stats.bogo_props += done*4;
    }

    ret = PROP_NOTHING;
    while (qhead < trail.size()) {
        const Lit p = trail[qhead].lit;
        watch_subarray ws = watches[~p];
        prop_stats.bogo_props += 1;

        Watched* i = ws.begin();
        Watched* j = ws.begin();
        Watched* end = ws.end();
        for(; i != end; i++) {
            if (i->is_bin()) {
                *j++ = *i;
                continue;
            }

            if (i->is_clause()) {
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
        prop_stats.bogo_props += ws.size()*4;
        while(i != end)
            *j++ = *i++;
        ws.shrink_(end-j);

        if (ret == PROP_FAIL) {
            return analyzeFail(confl);
        } else if (ret == PROP_SOMETHING) {
            prop_stats.bogo_props += ws.size()*4;
            goto start;
        }

        qhead++;
        prop_stats.bogo_props += ws.size()*4;
    }

    return lit_Undef;
}

//Add binary clause to deepest common ancestor
void HyperEngine::add_hyper_bin(const Lit p, const Clause* cl)
{
    prop_stats.otf_hyper_time += 2;

    //FRAT: level-0 units and the ID of the source clause, hinted last
    const auto src_cl_hints = [&]() {
        if (cl) {
            for(const Lit l: *cl) {
                if (var_data[l.var()].level == 0) {
                    assert(unit_cl_IDs[l.var()] != 0);
                    *frat << unit_cl_IDs[l.var()];
                }
            }
            *frat << cl->stats.id;
        }
    };

    Lit deepestAncestor = lit_Undef;
    bool hyperBinNotAdded = true;
    const int32_t ID = ++clause_id;
    if (curr_ancestors.size() > 1) {
        if (frat->enabled()) tmp_orig_ancestors = curr_ancestors;
        deepestAncestor = deepest_common_ancestor();

        need_to_add_bin_clause.insert(BinaryClause(p, ~deepestAncestor, true, ID));
        if (frat->enabled()) {
            //hints: per ancestor, its bin chain down from the common
            //ancestor, then the source clause
            *frat << add << ID << p << ~deepestAncestor << fratchain;
            for(const Lit a: tmp_orig_ancestors) {
                tmp_anc_chain.clear();
                Lit x = a;
                while (x != deepestAncestor) {
                    const PropBy& r = var_data[x.var()].reason;
                    tmp_anc_chain.push_back(r.get_id());
                    x = r.get_ancestor();
                }
                for(auto it = tmp_anc_chain.rbegin(); it != tmp_anc_chain.rend(); ++it)
                    *frat << *it;
            }
            src_cl_hints();
            *frat << fin;
        }

        hyperBinNotAdded = false;
    } else {
        //0-level propagation is NEVER made by propFull
        assert(curr_ancestors.size() > 0);

        deepestAncestor = curr_ancestors[0];
        hyperBinNotAdded = true;
        if (frat->enabled()) {
            //materialize the never-attached bin: it serves as a reason, so
            //hint chains may go through its ID. Deleted at the end of intree.
            *frat << add << ID << p << ~deepestAncestor << fratchain;
            src_cl_hints();
            *frat << fin;
            ghost_hyper_bins.push_back({ID, p, ~deepestAncestor});
        }
    }

    enqueue_with_acestor_info(p, deepestAncestor, true, ID);
    var_data[p.var()].reason.setHyperbin(true);
    var_data[p.var()].reason.setHyperbinNotAdded(hyperBinNotAdded);
}

/**
We can try both ways: either binary clause can be removed.
Try to remove one, then the other
Return which one is to be removed
*/
Lit HyperEngine::remove_which_bin_due_to_trans_red(
    Lit conflict
    , Lit this_ancestor
    , bool thisStepRed
) {
    prop_stats.otf_hyper_time += 1;
    const PropBy& data = var_data[conflict.var()].reason;

    bool onlyIrred = !data.is_red_step();
    Lit looking_for_ancestor = data.get_ancestor();

    if (this_ancestor == lit_Undef || looking_for_ancestor == lit_Undef)
        return lit_Undef;

    prop_stats.otf_hyper_time += 1;
    bool second_is_deeper = false;
    bool ambivalent = true;
    if (use_depth_trick) {
        ambivalent = depth[this_ancestor.var()] == depth[looking_for_ancestor.var()];
        if (depth[this_ancestor.var()] < depth[looking_for_ancestor.var()]) {
            second_is_deeper = true;
        }
    }
    #ifdef DEBUG_DEPTH
    cout
    << "1st: " << std::setw(6) << this_ancestor
    << " depth: " << std::setw(4) << depth[this_ancestor.var()]
    << "  2nd: " << std::setw(6) << looking_for_ancestor
    << " depth: " << std::setw(4) << depth[looking_for_ancestor.var()]
    ;
    #endif


    if ((ambivalent || !second_is_deeper) &&
        is_ancestor_of(
        conflict
        , this_ancestor
        , thisStepRed
        , onlyIrred
        , looking_for_ancestor
        )
    ) {
        #ifdef DEBUG_DEPTH
        cout << " -- OK" << endl;
        #endif
        //assert(ambivalent || !second_is_deeper);
        return this_ancestor;
    }

    onlyIrred = !thisStepRed;
    thisStepRed = data.is_red_step();
    std::swap(looking_for_ancestor, this_ancestor);
    if ((ambivalent || second_is_deeper) &&
        is_ancestor_of(
        conflict
        , this_ancestor
        , thisStepRed
        , onlyIrred
        , looking_for_ancestor
        )
    ) {
        #ifdef DEBUG_DEPTH
        cout << " -- OK" << endl;
        #endif
        //assert(ambivalent || second_is_deeper);
        return this_ancestor;
    }

    #ifdef DEBUG_DEPTH
    cout << " -- NOTK" << endl;
    #endif

    return lit_Undef;
}

/**
hop backwards from this_ancestor until:
1) we reach ancestor of 'conflict' -- at this point, we return TRUE
2) we reach an invalid point. Either root, or an invalid hop. We return FALSE.
*/
bool HyperEngine::is_ancestor_of(
    const Lit conflict
    , Lit this_ancestor
    , const bool thisStepRed
    , const bool onlyIrred
    , const Lit looking_for_ancestor
) {
    prop_stats.otf_hyper_time += 1;

    //Was propagated at level 0 -- clause_cleaner will remove the clause
    if (looking_for_ancestor == lit_Undef)
        return false;

    if (looking_for_ancestor == this_ancestor) {
        return false;
    }


    if (onlyIrred && thisStepRed) {
        return false;
    }

    //This is as low as we should search -- we cannot find what we are searchig for lower than this
    const size_t bottom = depth[looking_for_ancestor.var()];

    while(this_ancestor != lit_Undef
        && (!use_depth_trick || bottom <= depth[this_ancestor.var()])
    ) {

        if (this_ancestor == conflict) {
            return false;
        }

        if (this_ancestor == looking_for_ancestor) {
            return true;
        }

        const PropBy& data = var_data[this_ancestor.var()].reason;
        if ((onlyIrred && data.is_red_step())
            || data.get_hyperbin_not_added()
        ) {
            return false;  //reached would-be redundant hop (but this is irred)
        }

        this_ancestor = data.get_ancestor();
        prop_stats.otf_hyper_time += 1;
    }


    return false;
}

void HyperEngine::add_hyper_bin(const Lit p, const Clause& cl)
{
    assert(value(p.var()) == l_Undef);


    curr_ancestors.clear();
    for (const Lit lit : cl) {
        if (lit != p) {
            assert(value(lit) == l_False);
            if (var_data[lit.var()].level != 0)
                curr_ancestors.push_back(~lit);
        }
    }

    add_hyper_bin(p, &cl);
}

//Analyze why did we fail at decision level 1
Lit HyperEngine::analyzeFail(const PropBy propBy)
{
    //Clear out the datastructs we will be usin
    curr_ancestors.clear();

    //First, we set the ancestors, based on the clause
    //Each literal in the clause is an ancestor. So just 'push' them inside the
    //'curr_ancestors' variable
    switch(propBy.get_type()) {
        case binary_t: {
            const Lit lit = ~propBy.lit2();
            if (var_data[lit.var()].level != 0)
                curr_ancestors.push_back(lit);

            if (var_data[fail_bin_lit.var()].level != 0)
                curr_ancestors.push_back(~fail_bin_lit);

            break;
        }

        case clause_t: {
            const uint32_t offset = propBy.get_offset();
            const Clause& cl = *cl_alloc.ptr(offset);
            for(size_t i = 0; i < cl.size(); i++) {
                if (var_data[cl[i].var()].level != 0)
                    curr_ancestors.push_back(~cl[i]);
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
    assert(to_clear.empty());
    Lit foundLit = lit_Undef;
    while(foundLit == lit_Undef) {
        size_t num_lit_undef = 0;
        for (auto it = curr_ancestors.begin(), end = curr_ancestors.end(); it != end; ++it) {
            prop_stats.otf_hyper_time += 1;

            //We have reached the top of the graph, the other 'threads' that
            //are still stepping back will find which literal is the lowest
            //common ancestor
            if (*it == lit_Undef) {
                num_lit_undef++;
                assert(num_lit_undef != curr_ancestors.size());
                continue;
            }

            //Increase path count
            seen[it->toInt()]++;

            //Visited counter has to be cleared later, so add it to the
            //to-be-cleared set
            if (seen[it->toInt()] == 1)
                to_clear.push_back(*it);


            //Is this point where all the 'threads' that are stepping backwards
            //reach each other? If so, we have found what we were looking for!
            //We can exit, and return 'foundLit'
            if (seen[it->toInt()] == curr_ancestors.size()) {
                foundLit = *it;
                break;
            }

            //Update ancestor to its own ancestor, i.e. step up this 'thread'
            *it = var_data[it->var()].reason.get_ancestor();
        }
    }
    assert(foundLit != lit_Undef);

    //Clear nodes we have visited
    prop_stats.otf_hyper_time += to_clear.size()/2;
    for(const Lit lit: to_clear) {
        seen[lit.toInt()] = 0;
    }
    to_clear.clear();

    return foundLit;
}

void HyperEngine::remove_bin_clause(Lit lit, const int32_t ID)
{
    //The binary clause we should remove
    const BinaryClause clauseToRemove(
        ~var_data[lit.var()].reason.get_ancestor(),
        lit,
        var_data[lit.var()].reason.is_red_step(),
        ID);

    //We now remove the clause
    //If it's hyper-bin, then we remove the to-be-added hyper-binary clause
    //However, if the hyper-bin was never added because only 1 literal was unbound at level 0 (i.e. through
    //clause cleaning, the clause would have been 2-long), then we don't do anything.
    if (!var_data[lit.var()].reason.get_hyperbin()) {
        prop_stats.otf_hyper_time += 2;
        uselessBin.insert(clauseToRemove);
    } else if (!var_data[lit.var()].reason.get_hyperbin_not_added()) {
        prop_stats.otf_hyper_time += need_to_add_bin_clause.size()/4;
        std::set<BinaryClause>::iterator it = need_to_add_bin_clause.find(clauseToRemove);

        //In case this is called after a backtrack to decision_level 1
        //then in fact we might have already cleaned the
        //'need_to_add_bin_clause'. When called from probing, the IF below
        //must ALWAYS be true
        if (it != need_to_add_bin_clause.end()) {
            prop_stats.otf_hyper_time += 2;
            //FRAT: its add was emitted at creation, delete it
            *frat << del << it->get_id() << it->get_lit1() << it->get_lit2() << fin;
            need_to_add_bin_clause.erase(it);
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
        enqueue_with_acestor_info(lit, p, k->red(), k->get_id());
        return PROP_SOMETHING;

    } else if (val == l_False) {
        //Conflict

        fail_bin_lit = lit;
        confl = PropBy(~p, k->red(), k->get_id());
        return PROP_FAIL;

    } else if (var_data[lit.var()].level != 0 && perform_transitive_reduction) {
        //Propaged already
        assert(val == l_True);

        Lit remove = remove_which_bin_due_to_trans_red(lit, p, k->red());

        //Remove this one
        if (remove == p) {
            const Lit origAnc = var_data[lit.var()].reason.get_ancestor();
            const int32_t origID = var_data[lit.var()].reason.get_id();
            assert(origAnc != lit_Undef);

            remove_bin_clause(lit, origID);

            //Update data indicating what lead to lit
            var_data[lit.var()].reason = PropBy(~p, k->red(), false, false, k->get_id());
            assert(var_data[p.var()].level != 0);
            depth[lit.var()] = depth[p.var()] + 1;
            //NOTE: we don't update the levels of other literals... :S

            //for correctness, we would need this, but that would need re-writing of history :S
            //if (!onlyIrred) return PropBy();

        } else if (remove != lit_Undef) {
            prop_stats.otf_hyper_time += 2;
            uselessBin.insert(BinaryClause(~p, lit, k->red(), k->get_id()));
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
    if (value(i->get_blocked_lit()) == l_True) {
        *j++ = *i;
        return PROP_NOTHING;
    }

    //Dereference pointer
    prop_stats.bogo_props += 4;
    const ClOffset offset = i->get_offset();
    Clause& c = *cl_alloc.ptr(offset);

    PropResult ret = find_new_watch<true>(c, offset, j, p);
    if (ret != PROP_TODO)
        return ret;

    // Did not find watch -- clause is unit under assignment:
    *j++ = *i;
    if (value(c[0]) == l_False) {
        return handle_long_cl_conflict<true>(c, offset, confl);
    }

    add_hyper_bin(c[0], c);

    return PROP_SOMETHING;
}

size_t HyperEngine::mem_used() const
{
    size_t mem = 0;
    mem += PropEngine::mem_used();
    mem += curr_ancestors.capacity()*sizeof(Lit);

    return mem;
}

void HyperEngine::enqueue_with_acestor_info(
    const Lit p
    , const Lit ancestor
    , const bool red_step
    , const int32_t ID
) {
    //only called at decision level 1 during solving OR
    //during intree probing
    enqueue<true>(p, decision_level(), PropBy(~ancestor, red_step, false, false, ID));

    assert(var_data[ancestor.var()].level != 0);

    if (use_depth_trick) {
        depth[p.var()] = depth[ancestor.var()] + 1;
    } else {
        depth[p.var()] = 0;
    }
    #ifdef DEBUG_DEPTH
    cout
    << "Enqueued " << std::setw(6) << (p)
    << " by " << std::setw(6) << (~ancestor)
    << " ID: " << ID
    << " at depth " << std::setw(4) << depth[p.var()]
    << " at dec level: " << decision_level()
    << endl;
    #endif
}
