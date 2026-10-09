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

#include "datasync.h"
#include "varreplacer.h"
#include "solver.h"
#include "shareddata.h"

#include <iostream>
#include <iomanip>


using namespace CMSat;

DataSync::DataSync(Solver* _solver, SharedData* _sharedData) :
    solver(_solver)
    , shared_data(_sharedData)
    , seen(solver->seen)
    , to_clear(solver->to_clear)
{
}

void DataSync::set_shared_data(SharedData* _sharedData)
{
    shared_data = _sharedData;
    thread_id = _sharedData->cur_thread_id++;
}

void DataSync::new_var(const bool bva)
{
    if (!enabled())
        return;

    if (!bva) {
        sync_finish.push_back(0);
        sync_finish.push_back(0);
    }
    assert(solver->nVarsOuter()*2 == sync_finish.size());
}

void DataSync::new_vars(size_t n)
{
    if (!enabled())
        return;

    sync_finish.insert(sync_finish.end(), 2*n, 0);
    assert(solver->nVarsOuter()*2 == sync_finish.size());
}

void DataSync::save_on_var_memory()
{
}

void DataSync::update_vars(
    [[maybe_unused]] const vector<uint32_t>&  outer_to_inter
    , [[maybe_unused]] const vector<uint32_t>& inter_to_outer
) {
}

bool DataSync::syncData()
{
    if (!enabled()
        || lastSyncConf + solver->conf.sync_every_confl >= solver->sum_conflicts
    ) {
        return true;
    }

    assert(shared_data != nullptr);
    assert(solver->decision_level() == 0);

    //SEND data
    bool ok;
    shared_data->unit_mutex.lock();
    ok = shareUnitData();
    shared_data->unit_mutex.unlock();
    if (!ok) {
        return false;
    }
    solver->ok = solver->propagate<false>().isnullptr();
    if (!solver->ok) {
        return false;
    }

    //RECEIVE data
    shared_data->bin_mutex.lock();
    extend_bins_if_needed();
    clear_set_binary_values();
    ok = shareBinData();
    shared_data->bin_mutex.unlock();
    if (!ok) {
        return false;
    }

    lastSyncConf = solver->sum_conflicts;

    return true;
}

bool DataSync::shareUnitData()
{
    assert(solver->okay());
    assert(!solver->frat->enabled());

    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *shared_data;
    if (shared.value.size() < solver->nVarsOuter()) {
        shared.value.insert(
            shared.value.end(),
            solver->nVarsOuter()-shared.value.size(), l_Undef);
    }
    for (uint32_t var = 0; var < solver->nVarsOuter(); var++) {
        Lit thisLit = Lit(var, false);
        thisLit = solver->var_replacer->get_lit_replaced_with_outer(thisLit);
        thisLit = solver->map_outer_to_inter(thisLit);
        const lbool this_val = solver->value(thisLit);
        const lbool other_val = shared.value[var];

        if (this_val == l_Undef && other_val == l_Undef) {
            continue;
        }

        if (this_val != l_Undef && other_val != l_Undef) {
            if (this_val != other_val) {
                solver->ok = false;
                return false;
            } else {
                continue;
            }
        }

        if (other_val != l_Undef) {
            assert(this_val == l_Undef);
            Lit litToEnqueue = thisLit ^ (other_val == l_False);
            if (solver->var_data[litToEnqueue.var()].removed != Removed::none) {
                continue;
            }

            solver->enqueue<false>(litToEnqueue);

            thisGotUnitData++;
            continue;
        }

        if (this_val != l_Undef) {
            assert(other_val == l_Undef);
            shared.value[var] = this_val;
            thisSentUnitData++;
            continue;
        }
    }
    stats.recvUnitData += thisGotUnitData;
    stats.sentUnitData += thisSentUnitData;

    if (solver->conf.verbosity >= 1) {
        cout
        << solver->conf.prefix << "[sync " << thread_id << "  ]"
        << " got units " << thisGotUnitData
        << " (total: " << stats.recvUnitData << ")"
        << " sent units " << thisSentUnitData
        << " (total: " << stats.sentUnitData << ")"
        << endl;
    }

    return true;
}

void CMSat::DataSync::signal_new_long_clause(const vector<Lit>& cl)
{
    if (!enabled()) return;
    assert(thread_id != -1);
    if (cl.size() == 2) signal_new_bin_clause(cl[0], cl[1]);
}

bool DataSync::syncBinFromOthers()
{
    for (uint32_t ws_lit = 0; ws_lit < shared_data->bins.size(); ws_lit++) {
        if (shared_data->bins[ws_lit].data == nullptr) {
            continue;
        }

        Lit lit1 = Lit::toLit(ws_lit);
        lit1 = solver->var_replacer->get_lit_replaced_with_outer(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->var_data[lit1.var()].removed != Removed::none
            || solver->value(lit1.var()) != l_Undef
        ) {
            continue;
        }

        vector<Lit>& bins = *shared_data->bins[ws_lit].data;
        watch_subarray ws = solver->watches[lit1];

        assert(sync_finish.size() > ws_lit);
        if (bins.size() > sync_finish[ws_lit]
            && !syncBinFromOthers(lit1, bins, sync_finish[ws_lit], ws)
        ) {
            return false;
        }
    }

    return true;
}

bool DataSync::syncBinFromOthers(
    const Lit lit
    , const vector<Lit>& bins
    , uint32_t& finished
    , watch_subarray ws
) {
    assert(solver->var_replacer->get_lit_replaced_with(lit) == lit);
    assert(solver->var_data[lit.var()].removed == Removed::none);

    assert(to_clear.empty());
    for (const Watched& w: ws) {
        if (w.is_bin()) {
            to_clear.push_back(w.lit2());
            assert(seen.size() > w.lit2().toInt());
            seen[w.lit2().toInt()] = true;
        }
    }

    vector<Lit> lits(2);
    for (uint32_t i = finished; i < bins.size(); i++) {
        Lit other_lit = bins[i];
        other_lit = solver->var_replacer->get_lit_replaced_with_outer(other_lit);
        other_lit = solver->map_outer_to_inter(other_lit);
        if (solver->var_data[other_lit.var()].removed != Removed::none
            || solver->value(other_lit) != l_Undef
        ) {
            continue;
        }
        assert(seen.size() > other_lit.toInt());
        if (!seen[other_lit.toInt()]) {
            stats.recvBinData++;
            lits[0] = lit;
            lits[1] = other_lit;

            //Don't add FRAT: it would add to the thread data, too
            solver->add_clause_int(lits, true, nullptr, true, nullptr, false);
            if (!solver->okay()) {
                goto end;
            }
        }
    }
    finished = bins.size();

    end:
    for (const Lit l: to_clear) {
        seen[l.toInt()] = false;
    }
    to_clear.clear();

    return solver->okay();
}

void DataSync::syncBinToOthers()
{
    for(const std::pair<Lit, Lit>& bin: newBinClauses) {
        add_bin_to_threads(bin.first, bin.second);
    }

    newBinClauses.clear();
}

bool DataSync::add_bin_to_threads(Lit lit1, Lit lit2)
{
    assert(lit1 < lit2);
    if (shared_data->bins[lit1.toInt()].data == nullptr) {
        return false;
    }

    vector<Lit>& bins = *shared_data->bins[lit1.toInt()].data;
    for (const Lit lit : bins) {
        if (lit == lit2)
            return false;
    }

    bins.push_back(lit2);
    stats.sentBinData++;
    return true;
}

void DataSync::clear_set_binary_values()
{
    for(size_t i = 0; i < solver->nVarsOuter()*2; i++) {
        Lit lit1 = Lit::toLit(i);
        lit1 = solver->var_replacer->get_lit_replaced_with_outer(lit1);
        lit1 = solver->map_outer_to_inter(lit1);
        if (solver->value(lit1) != l_Undef) {
            shared_data->bins[i].clear();
        }
    }
}

void DataSync::extend_bins_if_needed()
{
    assert(shared_data->bins.size() <= (solver->nVarsOuter())*2);
    if (shared_data->bins.size() == (solver->nVarsOuter())*2)
        return;

    shared_data->bins.resize(solver->nVarsOuter()*2);
}

bool DataSync::shareBinData()
{
    assert(solver->okay());
    uint32_t oldRecvBinData = stats.recvBinData;
    uint32_t oldSentBinData = stats.sentBinData;

    bool ok = syncBinFromOthers();
    syncBinToOthers();
    size_t mem = shared_data->calc_memory_use_bins();

    if (solver->conf.verbosity >= 1) {
        cout
        << solver->conf.prefix << "[sync " << thread_id << "  ]"
        << " got bins " << (stats.recvBinData - oldRecvBinData)
        << " (total: " << stats.recvBinData << ")"
        << " sent bins " << (stats.sentBinData - oldSentBinData)
        << " (total: " << stats.sentBinData << ")"
        << " mem use: " << mem/(1024*1024) << " M"
        << endl;
    }

    return ok;
}

void DataSync::signal_new_bin_clause(Lit lit1, Lit lit2)
{
    if (!enabled()) return;
    if (solver->var_data[lit1.var()].is_bva) return;
    if (solver->var_data[lit2.var()].is_bva) return;

    lit1 = solver->map_inter_to_outer(lit1);
    lit2 = solver->map_inter_to_outer(lit2);

    if (lit1.toInt() > lit2.toInt()) std::swap(lit1, lit2);
    newBinClauses.push_back(std::make_pair(lit1, lit2));
}
