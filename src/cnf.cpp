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

#include "cnf.h"

#include <stdexcept>

#include "vardata.h"
#include "solvertypes.h"
#include "clauseallocator.h"
#include "watchalgos.h"
#include "varupdatehelper.h"
#include "time_mem.h"

using namespace CMSat;

void CNF::new_var(
    const bool bva,
    const uint32_t orig_outer,
    const bool /*insert_varorder*/)
{
    if (nVars() >= 1ULL<<28) {
        cout << "ERROR! Variable requested is far too large" << endl;
        throw std::runtime_error("ERROR! Variable requested is far too large");
    }

    minNumVars++;
    enlarge_minimal_datastructs();
    if (orig_outer == numeric_limits<uint32_t>::max()) {
        //completely new var
        enlarge_nonminimial_datastructs();

        uint32_t minVar = nVars()-1;
        uint32_t maxVar = nVarsOuter()-1;
        interToOuterMain.push_back(maxVar);
        const uint32_t x = interToOuterMain[minVar];
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
        //Old var, re-inserted
        assert(orig_outer < nVarsOuter());

        const uint32_t minVar = nVars()-1;
        uint32_t k = interToOuterMain[minVar];
        uint32_t z = outerToInterMain[orig_outer];
        interToOuterMain[minVar] = orig_outer;
        interToOuterMain[z] = k;

        outerToInterMain[k] = z;
        outerToInterMain[orig_outer] = minVar;

        swapVars(z);
    }

    #ifdef SLOW_DEBUG
    test_reflectivity_of_renumbering();
    #endif
}

void CNF::new_vars(const size_t n)
{
    if (nVars() + n >= 1ULL<<28) {
        cout << "ERROR! Variable requested is far too large" << endl;
        std::exit(-1);
    }

    minNumVars += n;
    enlarge_minimal_datastructs(n);
    enlarge_nonminimial_datastructs(n);

    size_t inter_at = interToOuterMain.size();
    interToOuterMain.insert(interToOuterMain.end(), n, 0);

    size_t outer_at = outerToInterMain.size();
    outerToInterMain.insert(outerToInterMain.end(), n, 0);

    size_t outer_to_with_bva_at = outer_to_with_bva_map.size();
    outer_to_with_bva_map.insert(outer_to_with_bva_map.end(), n, 0);

    for(int i = n-1; i >= 0; i--) {
        const uint32_t minVar = nVars()-i-1;
        const uint32_t maxVar = nVarsOuter()-i-1;

        interToOuterMain[inter_at++] = maxVar;
        const uint32_t x = interToOuterMain[minVar];
        interToOuterMain[minVar] = maxVar;
        interToOuterMain[maxVar] = x;

        outerToInterMain[outer_at++] = maxVar;
        outerToInterMain[maxVar] = minVar;
        outerToInterMain[x] = maxVar;

        swapVars(nVarsOuter()-i-1, i);
        varData[nVars()-i-1].is_bva = false;
        outer_to_with_bva_map[outer_to_with_bva_at++] = nVarsOuter()-i-1;
    }

    #ifdef SLOW_DEBUG
    test_reflectivity_of_renumbering();
    #endif
}

void CNF::swapVars(const uint32_t which, const int off_by)
{
    std::swap(assigns[nVars()-off_by-1], assigns[which]);
    std::swap(varData[nVars()-off_by-1], varData[which]);
}

void CNF::enlarge_nonminimial_datastructs(size_t n)
{
    assigns.insert(assigns.end(), n, l_Undef);
    unit_cl_IDs.insert(unit_cl_IDs.end(), n, 0);
    varData.insert(varData.end(), n, VarData());
    depth.insert(depth.end(), n, 0);
}

void CNF::enlarge_minimal_datastructs(size_t n)
{
    watches.insert(2*n);
    gwatches.insert(2*n);
    seen.insert(seen.end(), 2*n, 0);
    seen2.insert(seen2.end(), 2*n, 0);
    permDiff.insert(permDiff.end(), 2*n, 0);
}

void CNF::save_on_var_memory()
{
    //never resize varData --> contains info about what is replaced/etc.
    //never resize assigns --> contains 0-level assigns
    //never resize interToOuterMain, outerToInterMain

    watches.resize(nVars()*2);
    watches.consolidate();
    gwatches.resize(nVars()*2);

    for(auto& l: longRedCls) {
        l.shrink_to_fit();
    }
    longIrredCls.shrink_to_fit();

    seen.resize(nVars()*2);
    seen.shrink_to_fit();
    seen2.resize(nVars()*2);
    seen2.shrink_to_fit();
    permDiff.resize(nVars()*2);
    permDiff.shrink_to_fit();
}

//Test for reflectivity of interToOuterMain & outerToInterMain
void CNF::test_reflectivity_of_renumbering() const
{
    vector<uint32_t> test(nVarsOuter());
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

inline void CNF::updateWatch(
    watch_subarray ws
    , const vector<uint32_t>& outerToInter
) {
    for(Watched *it = ws.begin(), *end = ws.end()
        ; it != end
        ; ++it
    ) {
        if (it->isBin()) {
            it->setLit2(
                getUpdatedLit(it->lit2(), outerToInter)
            );
            continue;
        }

        if (it->isBNN()) {
            continue;
        }

        assert(it->isClause());
        const Clause &cl = *cl_alloc.ptr(it->get_offset());
        Lit blocked_lit = it->getBlockedLit();
        blocked_lit = getUpdatedLit(it->getBlockedLit(), outerToInter);
        bool found = false;
        for(Lit lit: cl) {
            if (lit == blocked_lit) {
                found = true;
                break;
            }
        }
        if (!found) {
            it->setBlockedLit(cl[2]);
        } else {
            it->setBlockedLit(blocked_lit);
        }
    }
}

void CNF::updateVars(
    const vector<uint32_t>& outerToInter
    , const vector<uint32_t>& interToOuter
    , const vector<uint32_t>& interToOuter2
) {
    updateArray(varData, interToOuter);
    updateArray(assigns, interToOuter);
    updateArray(unit_cl_IDs, interToOuter);
    updateBySwap(watches, seen, interToOuter2);

    for(watch_subarray w: watches) {
        if (!w.empty())
            updateWatch(w, outerToInter);
    }

    updateArray(interToOuterMain, interToOuter);
    updateArrayMapCopy(outerToInterMain, outerToInter);
}

uint64_t CNF::mem_used_longclauses() const
{
    uint64_t mem = 0;
    mem += cl_alloc.mem_used();
    mem += longIrredCls.capacity()*sizeof(ClOffset);
    for(auto& l: longRedCls) {
        mem += l.capacity()*sizeof(ClOffset);
    }
    return mem;
}

uint64_t CNF::print_mem_used_longclauses(const size_t totalMem) const
{
    uint64_t mem = mem_used_longclauses();
    print_stats_line("c Mem for longclauses"
        , mem/(1024UL*1024UL)
        , "MB"
        , stats_line_percent(mem, totalMem)
        , "%"
    );

    return mem;
}

size_t CNF::cl_size(const Watched& ws) const
{
    switch(ws.getType()) {
        case WatchType::watch_binary_t:
            return 2;

        case WatchType::watch_clause_t: {
            const Clause* cl = cl_alloc.ptr(ws.get_offset());
            return cl->size();
        }

        default:
            assert(false);
            return 0;
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
        case WatchType::watch_binary_t:
            ss << otherLit << ", " << ws.lit2();
            if (ws.red()) {
                ss << "(red)";
            }
            break;

        case WatchType::watch_clause_t: {
            const Clause* cl = cl_alloc.ptr(ws.get_offset());
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
    Clause* cl1 = cl_alloc.ptr(x);
    Clause* cl2 = cl_alloc.ptr(y);
    return (cl1->size() < cl2->size());
}

size_t CNF::mem_used_renumberer() const
{
    size_t mem = 0;
    mem += interToOuterMain.capacity()*sizeof(uint32_t);
    mem += outerToInterMain.capacity()*sizeof(uint32_t);
    mem += outer_to_with_bva_map.capacity()*sizeof(uint32_t);
    return mem;
}

vector<uint32_t> CNF::build_outer_to_without_bva_map() const
{
    vector<uint32_t> ret;
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

vector<uint32_t> CNF::build_outer_to_without_bva_map_extended() const
{
    assert(nVarsOutside() <= nVarsOuter());
    vector<uint32_t> ret;
    size_t at = 0;
    uint32_t extra_map = nVarsOutside();
    for(size_t i = 0; i < nVarsOuter(); i++) {
        if (!varData[map_outer_to_inter(i)].is_bva) {
            ret.push_back(at);
            at++;
        } else {
            ret.push_back(extra_map);
            extra_map++;
        }
    }
    assert(extra_map == nVarsOuter());

    return ret;
}

size_t CNF::mem_used() const
{
    size_t mem = 0;
    mem += sizeof(conf);
    mem += sizeof(binTri);
    mem += seen.capacity()*sizeof(uint16_t);
    mem += seen2.capacity()*sizeof(uint8_t);
    mem += toClear.capacity()*sizeof(Lit);

    return mem;
}

void CNF::test_all_clause_attached() const
{
    test_all_clause_attached(longIrredCls);
    for(const vector<ClOffset>& l: longRedCls) {
        test_all_clause_attached(l);
    }
}

void CNF::test_all_clause_attached(const vector<ClOffset>& offsets) const
{
    for (vector<ClOffset>::const_iterator
        it = offsets.begin(), end = offsets.end()
        ; it != end
        ; ++it
    ) {
        assert(normClauseIsAttached(*it));
    }
}

bool CNF::normClauseIsAttached(const ClOffset offset) const
{
    bool attached = true;
    const Clause& cl = *cl_alloc.ptr(offset);
    assert(cl.size() > 2);

    attached &= findWCl(watches[cl[0]], offset);
    attached &= findWCl(watches[cl[1]], offset);

    if (detached_xor_clauses && cl._xor_is_detached) {
        //We expect this NOT to be attached, actually.
        if (attached) {
            cout
            << "Failed. XOR-representing clause is NOT supposed to be attached"
            << " clause: " << cl
            << " _xor_is_detached: " << cl._xor_is_detached
            << " detached_xor_clauses: " << detached_xor_clauses
            << endl;
        }
        return !attached;
    }

    bool satcl = satisfied(cl);
    uint32_t num_false2 = 0;
    num_false2 += value(cl[0]) == l_False;
    num_false2 += value(cl[1]) == l_False;
    if (!satcl) {
        if (num_false2 != 0) {
            cout << "Clause failed: " << cl << endl;
            for(Lit l: cl) {
                cout << "val " << l << " : " << value(l) << endl;
            }
            for(const Watched& w: watches[cl[0]]) {
                cout << "watch " << cl[0] << endl;
                if (w.isClause() && w.get_offset() == offset) {
                    cout << "Block lit: " << w.getBlockedLit()
                    << " val: " << value(w.getBlockedLit()) << endl;
                }
            }
            for(const Watched& w: watches[cl[1]]) {
                cout << "watch " << cl[1] << endl;
                if (w.isClause() && w.get_offset() == offset) {
                    cout << "Block lit: " << w.getBlockedLit()
                    << " val: " << value(w.getBlockedLit()) << endl;
                }
            }
        }
        assert(num_false2 == 0 && "propagation was not full??");
    }

    return attached;
}

void CNF::find_all_attach() const
{
    for (size_t i = 0; i < watches.size(); i++) {
        const Lit lit = Lit::toLit(i);
        for (uint32_t i2 = 0; i2 < watches[lit].size(); i2++) {
            const Watched& w = watches[lit][i2];
            if (!w.isClause())
                continue;

            //Get clause
            Clause* cl = cl_alloc.ptr(w.get_offset());
            assert(!cl->freed());

            bool satcl = satisfied(*cl);
            if (!satcl) {
                if (value(w.getBlockedLit())  == l_True) {
                    cout << "ERROR: Clause " << *cl << " not satisfied, but its blocked lit, "
                    << w.getBlockedLit() << " is." << endl;
                }
                assert(value(w.getBlockedLit()) != l_True && "Blocked lit is satisfied but clause is NOT!!");
            }

            //Assert watch correctness
            if ((*cl)[0] != lit
                && (*cl)[1] != lit
            ) {
                std::cerr
                << "ERROR! Clause " << (*cl)
                << " not attached?"
                << endl;

                assert(false);
                std::exit(-1);
            }

            //Clause in one of the lists
            if (!find_clause(w.get_offset())) {
                std::cerr
                << "ERROR! did not find clause " << *cl
                << endl;

                assert(false);
                std::exit(1);
            }
        }
    }

    find_all_attach(longIrredCls);
    for(auto& lredcls: longRedCls) {
        find_all_attach(lredcls);
    }
}

void CNF::find_all_attach(const vector<ClOffset>& cs) const
{
    for(vector<ClOffset>::const_iterator
        it = cs.begin(), end = cs.end()
        ; it != end
        ; ++it
    ) {
        Clause& cl = *cl_alloc.ptr(*it);
        bool should_be_attached = true;
        if (detached_xor_clauses && cl._xor_is_detached) {
            should_be_attached = false;
        }
        bool ret = findWCl(watches[cl[0]], *it);
        if (ret != should_be_attached) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red()
            << " used in xor: " << cl.used_in_xor()
            << " detached xor: " << cl._xor_is_detached
            << " should be attached: " << should_be_attached
            << " )";
            if (ret) {
                cout << " doesn't have its 1st watch attached!";
            } else {
                cout << " HAS its 1st watch attached (but it should NOT)!";
            }
            cout << endl;

            assert(false);
            std::exit(-1);
        }

        ret = findWCl(watches[cl[1]], *it);
        if (ret != should_be_attached) {
            cout
            << "Clause " << cl
            << " (red: " << cl.red()
            << " used in xor: " << cl.used_in_xor()
            << " detached xor: " << cl._xor_is_detached
            << " should be attached: " << should_be_attached
            << " )";
            if (ret) {
                cout << " doesn't have its 2nd watch attached!";
            } else {
                cout << " HAS its 2nd watch attached (but it should NOT)!";
            }
            cout << endl;

            assert(false);
            std::exit(-1);
        }
    }
}


bool CNF::find_clause(const ClOffset offset) const
{
    for (uint32_t i = 0; i < longIrredCls.size(); i++) {
        if (longIrredCls[i] == offset)
            return true;
    }

    for(auto& lredcls: longRedCls) {
        for (ClOffset off: lredcls) {
            if (off == offset)
                return true;
        }
    }

    return false;
}

void CNF::check_wrong_attach() const
{
#ifdef SLOW_DEBUG
    for(auto& lredcls: longRedCls) {
        for (ClOffset offs: lredcls) {
            const Clause& cl = *cl_alloc.ptr(offs);
            for (uint32_t i = 0; i < cl.size(); i++) {
                if (i > 0)
                    assert(cl[i-1].var() != cl[i].var());
            }
        }
    }
    for(watch_subarray_const ws: watches) {
        check_watchlist(ws);
    }
#endif
}

void CNF::check_watchlist(watch_subarray_const ws) const
{
    for(const Watched& w: ws) {
        if (!w.isClause()) {
            continue;
        }

        const ClOffset offs = w.get_offset();
        const Clause& c = *cl_alloc.ptr(offs);
        Lit blockedLit = w.getBlockedLit();
        /*cout << "Clause " << c << " blocked lit:  "<< blockedLit << " val: " << value(blockedLit)
        << " blocked removed:" << !(varData[blockedLit.var()].removed == Removed::none)
        << " cl satisfied: " << satisfied(&c)
        << endl;*/
        assert(blockedLit.var() < nVars());

        if (varData[blockedLit.var()].removed == Removed::none
            //0-level FALSE --> clause cleaner removed it from clause, that's OK
            && value(blockedLit) != l_False
            && !satisfied(c)
        ) {
            bool found = false;
            for(Lit l: c) {
                if (l == blockedLit) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                cout << "Did not find non-removed blocked lit " << blockedLit
                << " val: " << value(blockedLit) << endl
                << "In clause " << c << endl;
            }
            assert(found);
        }

    }
}


uint64_t CNF::count_lits(
    const vector<ClOffset>& clause_array
    , const bool red
    , const bool allowFreed
) const {
    uint64_t lits = 0;
    for(vector<ClOffset>::const_iterator
        it = clause_array.begin(), end = clause_array.end()
        ; it != end
        ; ++it
    ) {
        const Clause& cl = *cl_alloc.ptr(*it);
        if (cl.freed()) {
            assert(allowFreed);
        } else {
            if ((cl.red() ^ red) == false) {
                lits += cl.size();
            }
        }
    }

    return lits;
}

void CNF::print_watchlist_stats() const
{
    uint64_t total_size = 0;
    uint64_t total_size_lits = 0;
    uint64_t total_cls = 0;
    uint64_t bin_cls = 0;
    uint64_t used_in_xor = 0;
    uint64_t used_in_xor_full = 0;
    for(auto const& ws: watches) {
        for(auto const& w: ws) {
            total_size+=1;
            if (w.isBin()) {
                total_size_lits+=2;
                total_cls++;
                bin_cls++;
            } else if (w.isClause()) {
                Clause* cl = cl_alloc.ptr(w.get_offset());
                assert(!cl->getRemoved());
                used_in_xor+=cl->used_in_xor();
                used_in_xor_full+=cl->used_in_xor_full();
                total_size_lits+=cl->size();
                total_cls++;
            }
        }
    }
    cout << "c [watchlist] avg watchlist size: " << float_div(total_size, watches.size());
    cout << " Avg cl size: " << float_div(total_size_lits, total_cls);
    cout << " Cls: " << total_cls;
    cout << " Total WS size: " << total_size;
    cout << " used_in_xor: " << used_in_xor;
    cout << " used_in_xor_full: " << used_in_xor_full;
    cout << " bin cl: " << bin_cls;
    cout << endl;
}

void CNF::print_all_clauses() const
{
    for(vector<ClOffset>::const_iterator
        it = longIrredCls.begin(), end = longIrredCls.end()
        ; it != end
        ; ++it
    ) {
        Clause* cl = cl_alloc.ptr(*it);
        cout
        << "Normal clause offs " << *it
        << " cl: " << *cl
        << endl;
    }


    uint32_t wsLit = 0;
    for (watch_array::const_iterator
        it = watches.begin(), end = watches.end()
        ; it != end
        ; ++it, wsLit++
    ) {
        Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;
        cout << "watches[" << lit << "]" << endl;
        for (const Watched *it2 = ws.begin(), *end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            if (it2->isBin()) {
                cout << "Binary clause part: " << lit << " , " << it2->lit2() << endl;
            } else if (it2->isClause()) {
                cout << "Normal clause offs " << it2->get_offset() << endl;
            }
        }
    }
}

bool CNF::no_marked_clauses() const
{
    for(ClOffset offset: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offset);
        assert(!cl->stats.marked_clause);
    }

    for(auto& lredcls: longRedCls) {
        for(ClOffset offset: lredcls) {
            Clause* cl = cl_alloc.ptr(offset);
            assert(!cl->stats.marked_clause);
        }
    }

    return true;
}

void CNF::add_frat(FILE* os) {
    if (frat) delete frat;
    frat = new DratFile<false>(interToOuterMain);
    frat->setFile(os);
    frat->set_sumconflicts_ptr(&sumConflicts);
    frat->set_sqlstats_ptr(sqlStats);
}

vector<uint32_t> CNF::get_outside_lit_incidence()
{
    assert(get_num_bva_vars() == 0);
    vector<uint32_t> inc;
    inc.resize(nVars()*2, 0);
    if (!okay()) return inc;

    for(uint32_t i = 0; i < nVars()*2; i++) {
        const Lit l = Lit::toLit(i);
        for(const auto& x: watches[l]) {
            if (x.isBin() &&
                !x.red() &&
                l.var() < x.lit2().var()) //don't count twice
            {
                inc[x.lit2().toInt()]++;
                inc[l.toInt()]++;
            }
        }
    }

    for(const auto& offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        for(const auto& l: *cl) {
            inc[l.toInt()]++;
        }
    }

    //Map to outer
    vector<uint32_t> inc_outer(nVarsOuter()*2, 0);
    for(uint32_t i = 0; i < inc.size(); i ++) {
        Lit outer = map_inter_to_outer(Lit::toLit(i));
        inc_outer[outer.toInt()] = inc[i];
    }

    //Map to outside
    if (get_num_bva_vars() != 0) {
        inc_outer = map_back_lits_to_without_bva(inc_outer);
    }
    return inc_outer;
}

vector<uint32_t> CNF::get_outside_var_incidence()
{
    assert(get_num_bva_vars() == 0);
    assert(okay());


    vector<uint32_t> inc;
    inc.resize(nVarsOuter(), 0);
    for(uint32_t i = 0; i < nVars()*2; i++) {
        const Lit l = Lit::toLit(i);
        for(const auto& x: watches[l]) {
            if (x.isBin() &&
                !x.red() &&
                l.var() < x.lit2().var()) //don't count twice
            {
                inc[x.lit2().var()]++;
                inc[l.var()]++;
            }
        }
    }

    for(const auto& offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        for(const auto& l: *cl) {
            inc[l.var()]++;
        }
    }

    //Map to outer
    vector<uint32_t> inc_outer(nVarsOuter(), 0);
    for(uint32_t i = 0; i < inc.size(); i ++) {
        uint32_t outer = map_inter_to_outer(i);
        inc_outer[outer] = inc[i];
    }

    //Map to outside
    if (get_num_bva_vars() != 0) {
        inc_outer = map_back_vars_to_without_bva(inc_outer);
    }
    return inc_outer;
}

vector<uint32_t> CNF::get_outside_var_incidence_also_red()
{
    vector<uint32_t> inc;
    inc.resize(nVars(), 0);
    for(uint32_t i = 0; i < nVars()*2; i++) {
        const Lit l = Lit::toLit(i);
        for(const auto& x: watches[l]) {
            if (x.isBin()) {
                inc[x.lit2().var()]++;
                inc[l.var()]++;
            }
        }
    }

    for(const auto& offs: longIrredCls) {
        Clause* cl = cl_alloc.ptr(offs);
        for(const auto& l: *cl) {
            inc[l.var()]++;
        }
    }

    for(const auto& reds: longRedCls) {
        for(const auto& offs: reds) {
            Clause* cl = cl_alloc.ptr(offs);
            for(const auto& l: *cl) {
                inc[l.var()]++;
            }
        }
    }

    //Map to outer
    vector<uint32_t> inc_outer(nVarsOuter(), 0);
    for(uint32_t i = 0; i < inc.size(); i ++) {
        uint32_t outer = map_inter_to_outer(i);
        inc_outer[outer] = inc[i];
    }

    //Map to outside
    if (get_num_bva_vars() != 0) {
        inc_outer = map_back_vars_to_without_bva(inc_outer);
    }
    return inc_outer;
}

bool CNF::check_bnn_sane(BNN& bnn)
{
    //assert(decisionLevel() == 0);

    int32_t ts = 0;
    int32_t undefs = 0;
    for(const auto& l: bnn) {
        if (value(l) == l_True) {
            ts++;
        }

        if (value(l) == l_Undef) {
            undefs++;
        }
    }
    assert(bnn.ts == ts);
    assert(bnn.undefs == undefs);

    if (bnn.empty()) {
        return false;
    }

    // we are at the cutoff no matter what undef is
    if (bnn.cutoff-ts <= 0) {
        if (bnn.set) {
            return true; //harmless, doesn't propagate
        }
        if (value(bnn.out) == l_False)
            return false; //always true, BAD
        if (value(bnn.out) == l_True)
            return true; //harmless, doesn't propagate

        //should have propagated bnn.out
        return false;
    }

    // we are under the cutoff no matter what undef is
    if (undefs < bnn.cutoff-ts) {
        if (bnn.set) {
            return false; //can never meet cutoff, BAD
        }
        if (value(bnn.out) == l_True)
            return false;  //can never meet cutoff, BAD
        if (value(bnn.out) == l_False)
            return true;

        //should have propagated bnn.out
        return false;
    }

    //it's set and cutoff can ONLY be met by ALL TRUE
    if (((!bnn.set && value(bnn.out) == l_True) || bnn.set) &&
        undefs == bnn.cutoff-ts)
    {
        return false;
    }

    return true;
}

void CNF::check_no_zero_ID_bins() const
{
    for(uint32_t i = 0; i < nVars()*2; i++) {
        Lit l = Lit::toLit(i);
        for(const auto& w: watches[l]) {
            //only do once per binary
            if (w.isBin()) {
                if (w.get_ID() == 0) {
                    cout << "ERROR, bin: " << l << " " << w.lit2() << " has ID " << w.get_ID() << endl;
                }
                assert(w.get_ID() > 0);
            }
        }
    }
}

bool CNF::zero_irred_cls(const CMSat::Lit lit) const
{
    for(auto const& w: watches[lit]) {
        switch(w.getType()) {
            case WatchType::watch_binary_t:
                if (w.red()) continue;
                else return false;
            case WatchType::watch_clause_t: {
                Clause* cl = cl_alloc.ptr(w.get_offset());
                if (cl->red()) continue;
                else return false;
            }
            case WatchType::watch_idx_t:
                release_assert(false);
                continue;
            case WatchType::watch_bnn_t:
                return false;
        }
    }
    return true;
}

#ifdef USE_TBUDDY
void CNF::free_bdds(vector<Xor>& xors)
{
    frat->flush();
    for(auto& x: xors) {
        delete x.bdd;
        x.bdd = NULL;
    }
}
#endif
