/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2009, Niklas Sorensson
Copyright (c) 2009-2012, Mate Soos

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include "cmsat/DataSync.h"
#include "cmsat/Subsumer.h"
#include "cmsat/VarReplacer.h"
#include "cmsat/XorSubsumer.h"
#include <iomanip>

using namespace CMSat;

DataSync::DataSync(Solver& _solver, SharedData* _sharedData) :
    lastSyncConf(0)
    , sentUnitData(0)
    , recvUnitData(0)
    , sharedData(_sharedData)
    , solver(_solver)
{}

void DataSync::newVar()
{
    syncFinish.push(0);
    syncFinish.push(0);
    seen.push(false);
    seen.push(false);
}

bool DataSync::syncData()
{
    if (sharedData == NULL
        || lastSyncConf + SYNC_EVERY_CONFL >= solver.conflicts) return true;

    assert(sharedData != NULL);
    assert(solver.decisionLevel() == 0);

    bool ok;
    #pragma omp critical (unitData)
    ok = shareUnitData();
    if (!ok) return false;

    #pragma omp critical (binData)
    ok = shareBinData();
    if (!ok) return false;

    lastSyncConf = solver.conflicts;

    return true;
}

bool DataSync::shareBinData()
{
    uint32_t oldRecvBinData = recvBinData;
    uint32_t oldSentBinData = sentBinData;

    SharedData& shared = *sharedData;
    if (shared.bins.size() != solver.nVars()*2)
        shared.bins.resize(solver.nVars()*2);

    for (uint32_t wsLit = 0; wsLit < solver.nVars()*2; wsLit++) {
        Lit lit1 = ~Lit::toLit(wsLit);
        lit1 = solver.varReplacer->getReplaceTable()[lit1.var()] ^ lit1.sign();
        if (solver.subsumer->getVarElimed()[lit1.var()]
            || solver.xorSubsumer->getVarElimed()[lit1.var()]
            || solver.value(lit1.var()) != l_Undef
            ) continue;

        vector<Lit>& bins = shared.bins[wsLit];
        vec<Watched>& ws = solver.watches[wsLit];

        if (bins.size() > syncFinish[wsLit]
            && !syncBinFromOthers(lit1, bins, syncFinish[wsLit], ws)) return false;
    }

    syncBinToOthers();

    if (solver.conf.verbosity >= 3) {
        std::cout << "c got bins " << std::setw(10) << (recvBinData - oldRecvBinData)
        << std::setw(10) << " sent bins " << (sentBinData - oldSentBinData) << std::endl;
    }

    return true;
}

bool DataSync::syncBinFromOthers(const Lit lit, const vector<Lit>& bins, uint32_t& finished, vec<Watched>& ws)
{
    assert(solver.varReplacer->getReplaceTable()[lit.var()].var() == lit.var());
    assert(solver.subsumer->getVarElimed()[lit.var()] == false);
    assert(solver.xorSubsumer->getVarElimed()[lit.var()] == false);

    vec<Lit> addedToSeen;
    for (vec<Watched>::iterator it = ws.getData(), end = ws.getDataEnd(); it != end; it++) {
        if (it->isBinary()) {
            addedToSeen.push(it->getOtherLit());
            seen[it->getOtherLit().toInt()] = true;
        }
    }

    vec<Lit> lits(2);
    for (uint32_t i = finished; i < bins.size(); i++) {
        if (!seen[bins[i].toInt()]) {
            Lit otherLit = bins[i];
            otherLit = solver.varReplacer->getReplaceTable()[otherLit.var()] ^ otherLit.sign();
            if (solver.subsumer->getVarElimed()[otherLit.var()]
                || solver.xorSubsumer->getVarElimed()[otherLit.var()]
                || solver.value(otherLit.var()) != l_Undef
                ) continue;

            recvBinData++;
            lits[0] = lit;
            lits[1] = otherLit;
            solver.addClauseInt(lits, true, 2, 0, true);
            lits.clear();
            lits.growTo(2);
            if (!solver.ok) goto end;
        }
    }
    finished = bins.size();

    end:
    for (uint32_t i = 0; i < addedToSeen.size(); i++)
        seen[addedToSeen[i].toInt()] = false;

    return solver.ok;
}

void DataSync::syncBinToOthers()
{
    for(vector<std::pair<Lit, Lit> >::const_iterator it = newBinClauses.begin(), end = newBinClauses.end(); it != end; it++) {
        addOneBinToOthers(it->first, it->second);
    }

    newBinClauses.clear();
}

void DataSync::addOneBinToOthers(const Lit lit1, const Lit lit2)
{
    assert(lit1.toInt() < lit2.toInt());

    vector<Lit>& bins = sharedData->bins[(~lit1).toInt()];
    for (vector<Lit>::const_iterator it = bins.begin(), end = bins.end(); it != end; it++) {
        if (*it == lit2) return;
    }

    bins.push_back(lit2);
    sentBinData++;
}

bool DataSync::shareUnitData()
{
    uint32_t thisGotUnitData = 0;
    uint32_t thisSentUnitData = 0;

    SharedData& shared = *sharedData;
    shared.value.growTo(solver.nVars(), l_Undef);
    for (uint32_t var = 0; var < solver.nVars(); var++) {
        Lit thisLit = Lit(var, false);
        thisLit = solver.varReplacer->getReplaceTable()[thisLit.var()] ^ thisLit.sign();
        const lbool thisVal = solver.value(thisLit);
        const lbool otherVal = shared.value[var];

        if (thisVal == l_Undef && otherVal == l_Undef) continue;
        if (thisVal != l_Undef && otherVal != l_Undef) {
            if (thisVal != otherVal) {
                solver.ok = false;
                return false;
            } else {
                continue;
            }
        }

        if (otherVal != l_Undef) {
            assert(thisVal == l_Undef);
            Lit litToEnqueue = thisLit ^ (otherVal == l_False);
            if (solver.subsumer->getVarElimed()[litToEnqueue.var()]
                || solver.xorSubsumer->getVarElimed()[litToEnqueue.var()]
                ) continue;

            solver.uncheckedEnqueue(litToEnqueue);
            solver.ok = solver.propagate<false>().isNULL();
            if (!solver.ok) return false;
            thisGotUnitData++;
            continue;
        }

        if (thisVal != l_Undef) {
            assert(otherVal == l_Undef);
            shared.value[var] = thisVal;
            thisSentUnitData++;
            continue;
        }
    }

    if (solver.conf.verbosity >= 3 && (thisGotUnitData > 0 || thisSentUnitData > 0)) {
        std::cout << "c got units " << std::setw(8) << thisGotUnitData
        << " sent units " << std::setw(8) << thisSentUnitData << std::endl;
    }

    recvUnitData += thisGotUnitData;
    sentUnitData += thisSentUnitData;

    return true;
}
