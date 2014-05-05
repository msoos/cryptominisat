#include "cnf.h"
#include "vardata.h"
#include "solvertypes.h"
#include "clauseallocator.h"
#include "watchalgos.h"
#include "varupdatehelper.h"

using namespace CMSat;

void CNF::new_var(const bool bva, const Var orig_outer)
{
    if (nVars() >= 1ULL<<28) {
        cout << "ERROR! Variable requested is far too large" << endl;
        std::exit(-1);
    }

    minNumVars++;
    enlarge_minimal_datastructs();
    if (conf.doCache) {
        implCache.new_var();
    }
    if (conf.doStamp) {
        stamp.new_var();
    }

    if (orig_outer == std::numeric_limits<Var>::max()) {
        enlarge_nonminimial_datastructs();

        Var minVar = nVars()-1;
        Var maxVar = nVarsOuter()-1;
        interToOuterMain.push_back(maxVar);
        const Var x = interToOuterMain[minVar];
        interToOuterMain[minVar] = maxVar;
        interToOuterMain[maxVar] = x;

        outerToInterMain.push_back(maxVar);
        outerToInterMain[maxVar] = minVar;
        outerToInterMain[x] = maxVar;

        swapVars(nVarsOuter()-1);
        varData[nVars()-1].is_bva = bva;
        if (bva) {
            num_bva_vars ++;
        } else {
            outer_to_with_bva_map.push_back(nVarsOuter() - 1);
        }
    } else {
        assert(orig_outer < nVarsOuter());

        const Var minVar = nVars()-1;
        Var k = interToOuterMain[minVar];
        Var z = outerToInterMain[orig_outer];
        interToOuterMain[minVar] = orig_outer;
        interToOuterMain[z] = k;

        outerToInterMain[k] = z;
        outerToInterMain[orig_outer] = minVar;

        swapVars(z);
    }

    #ifdef MORE_DEBUG
    test_reflectivity_of_renumbering();
    #endif
}

void CNF::new_vars(size_t n)
{
    if (nVars() + n >= 1ULL<<28) {
        cout << "ERROR! Variable requested is far too large" << endl;
        std::exit(-1);
    }

    minNumVars += n;
    enlarge_minimal_datastructs(n);
    if (conf.doCache) {
        implCache.new_vars(n);
    }
    if (conf.doStamp) {
        stamp.new_vars(n);
    }

    enlarge_nonminimial_datastructs(n);

    interToOuterMain.reserve(interToOuterMain.size() + n);
    outerToInterMain.reserve(outerToInterMain.size() + n);
    outer_to_with_bva_map.reserve(outer_to_with_bva_map.size() + n);
    for(int i = n-1; i >= 0; i--) {
        Var minVar = nVars()-i-1;
        Var maxVar = nVarsOuter()-i-1;
        //cout << "nVars(): " << nVars() << endl;
        //cout << "N: " << n << " i: " << i << " minVar: " << minVar << " maxVar: " << maxVar << endl;

        interToOuterMain.push_back(maxVar);
        const Var x = interToOuterMain[minVar];
        interToOuterMain[minVar] = maxVar;
        interToOuterMain[maxVar] = x;

        outerToInterMain.push_back(maxVar);
        outerToInterMain[maxVar] = minVar;
        outerToInterMain[x] = maxVar;

        swapVars(nVarsOuter()-i-1);
        varData[nVars()-i-1].is_bva = false;
        outer_to_with_bva_map.push_back(nVarsOuter()-i-1);
    }

    #ifdef MORE_DEBUG
    test_reflectivity_of_renumbering();
    #endif
}

void CNF::swapVars(const Var which)
{
    std::swap(assigns[nVars()-1], assigns[which]);
    std::swap(varData[nVars()-1], varData[which]);

    #ifdef STATS_NEEDED
    std::swap(varDataLT[nVars()-1], varDataLT[which]);
    #endif
}

void CNF::enlarge_nonminimial_datastructs(size_t n)
{
    assigns.resize(assigns.size() + n, l_Undef);
    varData.resize(varData.size() + n, VarData());
    #ifdef STATS_NEEDED
    varDataLT.resize(varDataLT.size() + n, VarData());
    #endif
}

void CNF::enlarge_minimal_datastructs(size_t n)
{
    watches.resize(nVars()*2);
    seen.resize(seen.size() + 2*n, 0);
    seen2.resize(seen2.size() + 2*n,0);
}

void CNF::saveVarMem()
{
    //never resize varData --> contains info about what is replaced/etc.
    //never resize assigns --> contains 0-level assigns
    //never resize interToOuterMain, outerToInterMain

    watches.resize(nVars()*2);
    watches.consolidate();
    implCache.saveVarMems(nVars());
    stamp.saveVarMem(nVars());

    seen.resize(nVars()*2);
    seen.shrink_to_fit();
    seen2.resize(nVars()*2);
    seen2.shrink_to_fit();
}

//Test for reflectivity of interToOuterMain & outerToInterMain
void CNF::test_reflectivity_of_renumbering() const
{
    vector<Var> test(nVarsOuter());
    for(size_t i = 0; i  < nVarsOuter(); i++) {
        test[i] = i;
    }
    updateArrayRev(test, interToOuterMain);
    #ifdef DEBUG_RENUMBER
    for(size_t i = 0; i < nVarsOuter(); i++) {
        cout << i << ": "
        << std::setw(2) << test[i] << ", "
        << std::setw(2) << outerToInterMain[i]
        << endl;
    }
    #endif

    for(size_t i = 0; i < nVarsOuter(); i++) {
        assert(test[i] == outerToInterMain[i]);
    }
    #ifdef DEBUG_RENUMBR
    cout << "Passed test" << endl;
    #endif
}

void CNF::updateVars(
    const vector<Var>& outerToInter
    , const vector<Var>& interToOuter
) {
    updateArray(interToOuterMain, interToOuter);
    updateArrayMapCopy(outerToInterMain, outerToInter);
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
        , stats_line_percent(mem, totalMem)
        , "%"
    );

    return mem;
}

bool CNF::redundant(const Watched& ws) const
{
    return (   (ws.isBinary() && ws.red())
            || (ws.isTri()   && ws.red())
            || (ws.isClause()
                && clAllocator.getPointer(ws.getOffset())->red()
                )
    );
}

bool CNF::redundant_or_removed(const Watched& ws) const
{
    if (ws.isBinary() || ws.isTri()) {
        return ws.red();
    }

   assert(ws.isClause());
   const Clause* cl = clAllocator.getPointer(ws.getOffset());
   return cl->red() || cl->getRemoved();
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

string CNF::watches_to_string(const Lit lit, watch_subarray_const ws) const
{
    std::stringstream ss;
    for(Watched w: ws) {
        ss << watched_to_string(lit, w) << " --  ";
    }
    return ss.str();
}

string CNF::watched_to_string(Lit otherLit, const Watched& ws) const
{
    std::stringstream ss;
    switch(ws.getType()) {
        case watch_binary_t:
            ss << otherLit << ", " << ws.lit2();
            if (ws.red()) {
                ss << "(red)";
            }
            break;

        case CMSat::watch_tertiary_t:
            ss << otherLit << ", " << ws.lit2() << ", " << ws.lit3();
            if (ws.red()) {
                ss << "(red)";
            }
            break;

        case watch_clause_t: {
            const Clause* cl = clAllocator.getPointer(ws.getOffset());
            for(size_t i = 0; i < cl->size(); i++) {
                ss << (*cl)[i];
                if (i + 1 < cl->size())
                    ss << ", ";
            }
            if (cl->red()) {
                ss << "(red)";
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

size_t CNF::get_renumber_mem() const
{
    size_t mem = 0;
    mem += interToOuterMain.capacity()*sizeof(Var);
    mem += outerToInterMain.capacity()*sizeof(Var);
    return mem;
}


vector<lbool> CNF::map_back_to_without_bva(const vector<lbool>& val) const
{
    vector<lbool> ret;
    assert(val.size() == nVarsOuter());
    ret.reserve(nVarsOutside());
    for(size_t i = 0; i < nVarsOuter(); i++) {
        if (!varData[map_outer_to_inter(i)].is_bva) {
            ret.push_back(val[i]);
        }
    }
    assert(ret.size() == nVarsOutside());
    return ret;
}

vector<Var> CNF::build_outer_to_without_bva_map() const
{
    vector<Var> ret;
    size_t at = 0;
    for(size_t i = 0; i < nVarsOuter(); i++) {
        if (!varData[map_outer_to_inter(i)].is_bva) {
            ret.push_back(at);
            at++;
        } else {
            ret.push_back(var_Undef);
        }
    }

    return ret;
}
