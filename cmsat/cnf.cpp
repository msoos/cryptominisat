#include "cnf.h"
#include "vardata.h"
#include "solvertypes.h"
#include "clauseallocator.h"
#include "watchalgos.h"

using namespace CMSat;

void CNF::newVar(bool bva)
{
    if (nVars() >= 1ULL<<28) {
        cout << "ERROR! Variable requested is far too large" << endl;
        exit(-1);
    }

    minNumVars++;
    watches.resize(nVars()*2);
    if (conf.doCache) {
        implCache.addNew();
    }
    if (conf.doStamp) {
        stamp.newVar();
    }

    assigns.push_back(l_Undef);
    std::swap(assigns[nVars()-1], assigns.back());
    varData.push_back(VarData());
    std::swap(varData[nVars()-1], varData.back());
    varData[nVars()-1].is_bva = bva;
    #ifdef STATS_NEEDED
    varDataLT.push_back(VarData());
    std::swap(varDataLT[nVars()-1], varDataLT.back());
    #endif

    seen      .push_back(0);
    seen      .push_back(0);
    seen2     .push_back(0);
    seen2     .push_back(0);

    Var minVar = nVars()-1;
    Var maxVar = nVarsReal()-1;
    interToOuterMain.push_back(maxVar);
    const Var x = interToOuterMain[minVar];
    interToOuterMain[minVar] = maxVar;
    interToOuterMain[maxVar] = x;

    outerToInterMain.push_back(maxVar);
    outerToInterMain[maxVar] = minVar;
    outerToInterMain[x] = maxVar;
}

void CNF::saveVarMem()
{
    watches.resize(nVars()*2);
    watches.consolidate();
    implCache.saveVarMems(nVars());
    stamp.saveVarMem(nVars());

    seen.resize(nVars()*2);
    seen.shrink_to_fit();
    seen2.resize(nVars()*2);
    seen2.shrink_to_fit();
}

void CNF::test_reflectivity_of_renumbering() const
{
    //Test for reflectivity of interToOuterMain & outerToInterMain
    #ifdef NDEBUG
    return;
    #endif

    vector<Var> test(nVarsReal());
    for(size_t i = 0; i  < nVarsReal(); i++) {
        test[i] = i;
    }
    updateArrayRev(test, interToOuterMain);
    #ifdef DEBUG_RENUMBER
    for(size_t i = 0; i < nVarsReal(); i++) {
        cout << i << ": "
        << std::setw(2) << test[i] << ", "
        << std::setw(2) << outerToInterMain[i]
        << endl;
    }
    #endif
    for(size_t i = 0; i < nVarsReal(); i++) {
        assert(test[i] == outerToInterMain[i]);
    }
    #ifdef DEBUG_RENUMBR
    cout << "Passed test" << endl;
    #endif
}

size_t CNF::print_mem_used_longclauses(const size_t totalMem) const
{
    size_t mem = 0;
    mem += clAllocator.memUsed();
    mem += longIrredCls.capacity()*sizeof(ClOffset);
    mem += longRedCls.capacity()*sizeof(ClOffset);
    printStatsLine("c Mem for longclauses"
        , mem/(1024UL*1024UL)
        , "MB"
        , (double)mem/(double)totalMem*100.0
        , "%"
    );

    return mem;
}

bool CNF::redundant(const Watched& ws) const
{
    return ((ws.isBinary() || ws.isTri()) && ws.red())
            || (ws.isClause() && clAllocator.getPointer(ws.getOffset())->red()
    );
}

size_t CNF::cl_size(const Watched& ws) const
{
    switch(ws.getType()) {
        case watch_binary_t:
            return 2;
            break;

        case CMSat::watch_tertiary_t:
            return 3;
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator.getPointer(ws.getOffset());
            return cl->size();
            break;
        }

        default:
            assert(false);
            return 0;
            break;
    }
}

string CNF::watched_to_string(Lit otherLit, const Watched& ws) const
{
    std::stringstream ss;
    switch(ws.getType()) {
        case watch_binary_t:
            ss << otherLit << ", " << ws.lit2();
            break;

        case CMSat::watch_tertiary_t:
            ss << otherLit << ", " << ws.lit2() << ", " << ws.lit3();
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator.getPointer(ws.getOffset());
            for(size_t i = 0; i < cl->size(); i++) {
                ss << (*cl)[i];
                if (i < cl->size()-1)
                    ss << ", ";
            }
            break;
        }

        default:
            assert(false);
            break;
    }

    return ss.str();
}

bool ClauseSizeSorter::operator () (const ClOffset x, const ClOffset y)
{
    Clause* cl1 = clAllocator.getPointer(x);
    Clause* cl2 = clAllocator.getPointer(y);
    return (cl1->size() < cl2->size());
}

void CNF::remove_tri_but_lit1(
    const Lit lit1
    , const Lit lit2
    , const Lit lit3
    , const bool red
    , int64_t& timeAvailable
) {
    //Remove tri
    Lit lits[3];
    lits[0] = lit1;
    lits[1] = lit2;
    lits[2] = lit3;
    std::sort(lits, lits+3);
    timeAvailable -= watches[lits[0].toInt()].size();
    timeAvailable -= watches[lits[1].toInt()].size();
    timeAvailable -= watches[lits[2].toInt()].size();
    removeTriAllButOne(watches, lit1, lits, red);

    //Update stats for tri
    if (red) {
        binTri.redTris--;
    } else {
        binTri.irredTris--;
    }
}

