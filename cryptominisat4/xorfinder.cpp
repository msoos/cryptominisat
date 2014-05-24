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

#include "xorfinder.h"
#include "time_mem.h"
#include "solver.h"
#include "varreplacer.h"
#include "simplifier.h"
#include "clauseallocator.h"
#include <m4ri/m4ri.h>
#include <limits>
#include "sqlstats.h"

using namespace CMSat;
using std::cout;
using std::endl;

XorFinder::XorFinder(Simplifier* _subsumer, Solver* _solver) :
    subsumer(_subsumer)
    , solver(_solver)
    , numCalls(0)
    , seen(_solver->seen)
    , seen2(_solver->seen2)
{
    m4ri_build_all_codes();
}

bool XorFinder::findXors()
{
    const int64_t orig_max_time_find_xors = 200LL*1000LL*1000LL;
    maxTimeFindXors = orig_max_time_find_xors;
    double myTime = cpuTime();
    numCalls++;
    runStats.clear();
    xors.clear();
    xorOcc.clear();
    xorOcc.resize(solver->nVars());
    triedAlready.clear();

    vector<Lit> lits;
    for (vector<ClOffset>::iterator
        it = solver->longIrredCls.begin()
        , end = solver->longIrredCls.end()
        ; it != end && maxTimeFindXors > 0
        ; it++
    ) {
        ClOffset offset = *it;
        Clause* cl = solver->clAllocator.getPointer(offset);
        maxTimeFindXors -= 3;

        //Already freed
        if (cl->freed())
            continue;

        //Too large -> too expensive
        if ((long)cl->size() > solver->conf.maxXorToFind)
            return solver->ok;

        //If not tried already, find an XOR with it
        if (triedAlready.find(offset) == triedAlready.end()) {
            triedAlready.insert(offset);

            lits.resize(cl->size());
            for(size_t i = 0; i < cl->size(); i++) {
                lits[i] = (*cl)[i];
            }
            findXor(lits, cl->abst);
        }
    }

    size_t wsLit = 0;
    for (watch_array::const_iterator
        it = solver->watches.begin(), end = solver->watches.end()
        ; it != end && maxTimeFindXors > 0
        ; ++it, wsLit++
    ) {
        const Lit lit = Lit::toLit(wsLit);
        watch_subarray_const ws = *it;

        maxTimeFindXors -= ws.size()*3;
        for (watch_subarray_const::const_iterator
            it2 = ws.begin(), end2 = ws.end()
            ; it2 != end2
            ; it2++
        ) {
            //Only care about tertiaries
            if (!it2->isTri())
                continue;

            //Only bother about each tri-clause once
            if (lit > it2->lit2()
                || it2->lit2() > it2->lit3()
            ) {
                continue;
            }

            lits.resize(3);
            lits[0] = lit;
            lits[1] = it2->lit2();
            lits[2] = it2->lit3();

            findXor(lits, calcAbstraction(lits));
        }
    }

    const bool time_out = (maxTimeFindXors < 0);
    const double time_remain = (double)maxTimeFindXors/(double)orig_max_time_find_xors;
    runStats.findTime = cpuTime() - myTime;
    runStats.time_outs += time_out;
    assert(runStats.foundXors == xors.size());
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed(
            solver
            , "xorfind"
            , cpuTime() - myTime
            , time_out
            , time_remain
        );
    }

    if (solver->conf.verbosity >= 5) {
        cout << "c Found XORs: " << endl;
        for(vector<Xor>::const_iterator
            it = xors.begin(), end = xors.end()
            ; it != end
            ; it++
        ) {
            cout << "c " << *it << endl;
        }
    }

    if (solver->conf.doEchelonizeXOR && xors.size() > 0) {
        extractInfo();
    }

    if (solver->conf.verbosity >= 1) {
        runStats.printShort();
    }

    globalStats += runStats;

    return solver->ok;
}

bool XorFinder::extractInfo()
{
    double myTime = cpuTime();
    size_t origTrailSize = solver->trail_size();

    vector<uint32_t> varsIn(solver->nVars(), 0);
    for(const Xor& x: xors) {
        for(const Var v: x.vars) {
            varsIn[v]++;
        }
    }

    //Pre-filter XORs -- don't use any XOR which is not connected to ANY
    //other XOR. These cannot be XOR-ed with anything anyway
    size_t i = 0;
    vector<size_t> xorsToUse;
    for(vector<Xor>::const_iterator
        it = xors.begin(), end = xors.end()
        ; it != end
        ; it++, i++
    ) {
        const Xor& thisXor = *it;
        bool makeItIn = false;
        for(Var v: thisXor.vars) {
            if (varsIn[v] > 1) {
                makeItIn = true;
                break;
            }
        }
        //If this clause is not connected to ANY other, skip
        if (!makeItIn)
            continue;

        xorsToUse.push_back(i);
    }

    //Cut above-filtered XORs into blocks
    cutIntoBlocks(xorsToUse);
    runStats.blockCutTime += myTime - cpuTime();
    myTime = cpuTime();

    //These mappings will be needed for the matrixes, which will have far less
    //variables than solver->nVars()
    outerToInterVarMap.clear();
    outerToInterVarMap.resize(solver->nVars(), std::numeric_limits<size_t>::max());
    interToOUterVarMap.clear();
    interToOUterVarMap.resize(solver->nVars(), std::numeric_limits<size_t>::max());

    //Go through all blocks, and extract info
    i = 0;
    for(vector<vector<Var> >::const_iterator
        it = blocks.begin(), end = blocks.end()
        ; it != end
        ; it++, i++
    ) {
        //If block is already merged, skip
        if (it->empty())
            continue;

        //const uint64_t oldNewUnits = newUnits;
        //const uint64_t oldNewBins = newBins;
        if (!extractInfoFromBlock(*it, i))
            goto end;

        //cout << "New units this round: " << (newUnits - oldNewUnits) << endl;
        //cout << "New bins this round: " << (newBins - oldNewBins) << endl;
    }

end:

    const double time_used = cpuTime() - myTime;
    runStats.zeroDepthAssigns = solver->trail_size() - origTrailSize;
    runStats.extractTime += time_used;
    if (solver->conf.doSQL) {
        solver->sqlStats->time_passed_min(
            solver
            , "xorextract"
            , time_used
        );
    }

    return solver->ok;
}

bool XorFinder::extractInfoFromBlock(
    const vector<Var>& block
    , const size_t blockNum
) {
    assert(solver->okay());
    if (block.empty()) {
        return solver->okay();
    }

    //Outer-inner var mapping is needed because not all vars are in the matrix
    size_t num = 0;
    for(vector<Var>::const_iterator
        it2 = block.begin(), end2 = block.end()
        ; it2 != end2
        ; it2++, num++
    ) {
        //Used to put XOR into matrix
        outerToInterVarMap[*it2] = num;

        //Used to transform new data in matrix to solver
        interToOUterVarMap[num] = *it2;
    }

    //Get corresponding XORs
    const vector<size_t> thisXors = getXorsForBlock(blockNum);
    assert(thisXors.size() > 1 && "We pre-filter the set such that *every* block contains at least 2 xors");

    //Set up matrix
    size_t numCols = block.size()+1; //we need augmented column
    size_t matSize = numCols*thisXors.size();
    if (matSize > solver->conf.maxXORMatrix) {
        //this matrix is way too large, skip :(
        return solver->okay();
    }
    mzd_t *mat = mzd_init(thisXors.size(), numCols);
    assert(mzd_is_zero(mat));

    //Fill row-by-row
    size_t row = 0;
    for(vector<size_t>::const_iterator
        it = thisXors.begin(), end2 = thisXors.end()
        ; it != end2
        ; it++, row++
    ) {
        const Xor& thisXor = xors[*it];
        assert(thisXor.vars.size() > 2 && "All XORs must be larger than 2-long");
        //Put XOR into the matrix
        for(vector<Var>::const_iterator
            it2 = thisXor.vars.begin(), end3 = thisXor.vars.end()
            ; it2 != end3
            ; it2++
        ) {
            const Var var = outerToInterVarMap[*it2];
            assert(var < numCols-1);
            mzd_write_bit(mat, row, var, 1);
        }

        //Add RHS to the augmented columns
        if (thisXor.rhs)
            mzd_write_bit(mat, row, numCols-1, 1);
    }

    //Fully echelonize
    mzd_echelonize(mat, true);

    //Examine every row if it gives some new short truth
    vector<Lit> lits;
    for(size_t i = 0; i < row; i++) {
        //Extract places where it's '1'
        lits.clear();
        for(size_t c = 0; c < numCols-1; c++) {
            if (mzd_read_bit(mat, i, c))
                lits.push_back(Lit(interToOUterVarMap[c], false));

            //No point in going on, we cannot do anything with >2-long XORs
            if (lits.size() > 2)
                break;
        }

        //Extract RHS
        const bool rhs = mzd_read_bit(mat, i, numCols-1);

        switch(lits.size()) {
            case 0:
                //0-long XOR clause is equal to 1? If so, it's UNSAT
                if (rhs) {
                    solver->add_xor_clause_inter(lits, 1, false);
                    assert(!solver->okay());
                    goto end;
                }
                break;

            case 1: {
                runStats.newUnits++;
                solver->add_xor_clause_inter(lits, rhs, false);
                if (!solver->okay())
                    goto end;
                break;
            }

            case 2: {
                runStats.newBins++;
                solver->add_xor_clause_inter(lits, rhs, false);
                if (!solver->okay())
                    goto end;
                break;
            }

            default:
                //if resulting xor is larger than 2-long, we cannot extract anything.
                break;
        }
    }

    //Free mat, and return what need to be returned
    end:
    mzd_free(mat);

    return solver->okay();
}

vector<size_t> XorFinder::getXorsForBlock(const size_t blockNum)
{
    vector<size_t> xorsInThisBlock;

    for(size_t i = 0; i < xors.size(); i++) {
        const Xor& thisXor = xors[i];
        assert(thisXor.vars.size() > 2 && "XORs are always at least 3-long!");

        if (varToBlock[thisXor.vars[0]] == blockNum) {
            xorsInThisBlock.push_back(i);

            for(vector<Var>::const_iterator it = thisXor.vars.begin(), end = thisXor.vars.end(); it != end; it++) {
                assert(varToBlock[*it] == blockNum && "if any vars are in this block, ALL block are in this block");
            }
        }
    }

    return xorsInThisBlock;
}

void XorFinder::cutIntoBlocks(const vector<size_t>& xorsToUse)
{
    //Clearing data we will fill below
    varToBlock.clear();
    varToBlock.resize(solver->nVars(), std::numeric_limits<size_t>::max());
    blocks.clear();

    //Go through each XOR, and either make a new block for it
    //or merge it into an existing block
    //or merge it into an existing block AND merge blocks it joins together
    for(vector<size_t>::const_iterator it = xorsToUse.begin(), end = xorsToUse.end(); it != end; it++) {
        const Xor& thisXor = xors[*it];

        //Calc blocks for this XOR
        set<size_t> blocksBelongTo;
        for(vector<Var>::const_iterator it2 = thisXor.vars.begin(), end2 = thisXor.vars.end(); it2 != end2; it2++) {
            if (varToBlock[*it2] != std::numeric_limits<size_t>::max())
                blocksBelongTo.insert(varToBlock[*it2]);
        }

        switch(blocksBelongTo.size()) {
            case 0: {
                //Create new block
                vector<Var> block;
                for(vector<Var>::const_iterator it2 = thisXor.vars.begin(), end2 = thisXor.vars.end(); it2 != end2; it2++) {
                    varToBlock[*it2] = blocks.size();
                    block.push_back(*it2);
                }
                blocks.push_back(block);
                runStats.numBlocks++;

                continue;
            }

            case 1: {
                //Add to existing block
                const size_t blockNum = *blocksBelongTo.begin();
                vector<Var>& block = blocks[blockNum];
                for(vector<Var>::const_iterator it2 = thisXor.vars.begin(), end2 = thisXor.vars.end(); it2 != end2; it2++) {
                    if (varToBlock[*it2] == std::numeric_limits<size_t>::max()) {
                        block.push_back(*it2);
                        varToBlock[*it2] = blockNum;
                    }
                }
                continue;
            }

            default: {
                //Merge blocks into first block
                const size_t blockNum = *blocksBelongTo.begin();
                set<size_t>::const_iterator it2 = blocksBelongTo.begin();
                vector<Var>& finalBlock = blocks[blockNum];
                it2++; //don't merge the first into the first
                for(set<size_t>::const_iterator end2 = blocksBelongTo.end(); it2 != end2; it2++) {
                    for(vector<Var>::const_iterator
                        it3 = blocks[*it2].begin(), end3 = blocks[*it2].end()
                        ; it3 != end3
                        ; it3++
                    ) {
                        finalBlock.push_back(*it3);
                        varToBlock[*it3] = blockNum;
                    }
                    blocks[*it2].clear();
                    runStats.numBlocks--;
                }

                //add remaining vars
                for(vector<Var>::const_iterator
                    it3 = thisXor.vars.begin(), end3 = thisXor.vars.end()
                    ; it3 != end3
                    ; it3++
                ) {
                    if (varToBlock[*it3] == std::numeric_limits<size_t>::max()) {
                        finalBlock.push_back(*it3);
                        varToBlock[*it3] = blockNum;
                    }
                }
            }
        }
    }

    //caclulate stats
    for(vector<vector<Var> >::const_iterator
        it = blocks.begin(), end = blocks.end()
        ; it != end
        ; it++
    ) {
        //this set has been merged into another set. Skip
        if (it->empty())
            continue;

        //cout << "Block vars: " << it->size() << endl;
        runStats.numVarsInBlocks += it->size();
    }

    if (solver->conf.verbosity >= 2) {
        cout << "c Sum vars in blocks: " << runStats.numVarsInBlocks << endl;
    }
}

void XorFinder::findXor(vector<Lit>& lits, cl_abst_type abst)
{
    //Set this clause as the base for the FoundXors
    //fill 'seen' with variables
    FoundXors foundCls(lits, abst, seen);

    //Try to match on all literals
    for (vector<Lit>::const_iterator
        l = lits.begin(), end = lits.end()
        ; l != end
        ; l++
    ) {
        findXorMatch(solver->watches[(*l).toInt()], *l, foundCls);
        findXorMatch(solver->watches[(~(*l)).toInt()], ~(*l), foundCls);

        //More expensive
        //findXorMatchExt(solver->watches[(*l).toInt()], *l, foundCls);
        //findXorMatchExt(solver->watches[(~(*l)).toInt()], ~(*l), foundCls);

        //TODO stamping
        /*if (solver->conf.useCacheWhenFindingXors) {
            findXorMatch(solver->implCache[(*l).toInt()].lits, *l, foundCls);
            findXorMatch(solver->implCache[(~*l).toInt()].lits, ~(*l), foundCls);
        }*/

        maxTimeFindXors -= 5;
        if (foundCls.foundAll())
            break;
    }

    maxTimeFindXors -= 5;
    if (foundCls.foundAll()) {
        Xor thisXor(lits, foundCls.getRHS());
        assert(xorOcc.size() > lits[0].var());
        #ifdef VERBOSE_DEBUG_XOR_FINDER
        cout << "XOR found: " << lits << endl;
        #endif

        //Have we found this XOR clause already?
        const vector<uint32_t>& whereToFind = xorOcc[lits[0].var()];
        bool found = false;
        for (vector<uint32_t>::const_iterator
            it = whereToFind.begin(), end = whereToFind.end()
            ; it != end
            ; it++
        ) {
            if (xors[*it] == thisXor) {
                found = true;
                break;
            }
        }

        //If XOR clause is new, add it
        if (!found) {
            maxTimeFindXors -= 20;
            xors.push_back(thisXor);
            runStats.foundXors++;
            runStats.sumSizeXors += lits.size();
            uint32_t thisXorIndex = xors.size()-1;
            for (vector<Lit>::const_iterator
                l = lits.begin(), end = lits.end()
                ; l != end
                ; l++
            ) {
                xorOcc[l->var()].push_back(thisXorIndex);
            }
        }
    }

    //Clear 'seen'
    for (vector<Lit>::const_iterator
        l = lits.begin(), end = lits.end()
        ; l != end
        ; l++
    ) {
        seen[l->var()] = 0;
    }
}

//TODO stamping
/*void XorFinder::findXorMatch(
    const vector<LitExtra>& lits
    , const Lit lit
    , FoundXors& foundCls
) const {

    for (vector<LitExtra>::const_iterator
        it = lits.begin(), end = lits.end()
        ; it != end
        ; it++
    )  {
        if (seen[it->getLit().var()]) {
            foundCls.add(lit, it->getLit());
        }
    }
}*/

void XorFinder::findXorMatchExt(
    watch_subarray_const occ
    , Lit lit
    , FoundXors& foundCls
) {
    //seen2 is clear

    for (watch_subarray::const_iterator
        it = occ.begin(), end = occ.end()
        ; it != end
        ; it++
    ) {
        //Deal with binary
        if (it->isBinary()) {
            if (seen[it->lit2().var()]) {
                tmpClause.clear();
                tmpClause.push_back(lit);
                tmpClause.push_back(it->lit2());
                if (tmpClause[0] > tmpClause[1])
                    std::swap(tmpClause[0], tmpClause[1]);

                foundCls.add(tmpClause, varsMissing);
            }

            continue;
        }

        assert(it->isClause() && "This algo has not been updated to deal with TRI, sorry");

        //Deal with clause
        const ClOffset offset = it->getOffset();
        Clause& cl = *solver->clAllocator.getPointer(offset);
        if (cl.freed())
            continue;

        //Must not be larger than the original clauses
        if (cl.size() > foundCls.getSize())
            continue;

        tmpClause.clear();
        //cout << "Orig clause: " << foundCls.getOrigCl() << endl;

        bool rhs = true;
        uint32_t i = 0;
        for (const Lit *l = cl.begin(), *end = cl.end(); l != end; l++, i++) {

            //If this literal is not meant to be inside the XOR
            //then try to find a replacement for it from the cache
            if (!seen[l->var()]) {
                bool found = false;
                //TODO stamping
                /*const vector<LitExtra>& cache = solver->implCache[Lit(l->var(), true).toInt()].lits;
                for(vector<LitExtra>::const_iterator it2 = cache.begin(), end2 = cache.end(); it2 != end2 && !found; it2++) {
                    if (seen[l->var()] && !seen2[l->var()]) {
                        found = true;
                        seen2[l->var()] = true;
                        rhs ^= it2->getLit().sign();
                        tmpClause.push_back(it2->getLit());
                        //cout << "Added trans lit: " << tmpClause.back() << endl;
                    }
                }*/

                //Didn't find replacement
                if (!found)
                    goto end;
            }
            else
            //Fine, it's inside the orig clause, but we might have already added this lit
            {
                if (!seen2[l->var()]) {
                    seen2[l->var()] = true;
                    rhs ^= l->sign();
                    tmpClause.push_back(*l);
                    //cout << "Added lit: " << tmpClause.back() << endl;
                } else {
                    goto end; //HACK: we don't want both 'lit' and '~lit' end up in the clause
                }
            }
        }

        //either the invertedness has to match, or the size must be smaller
        if (rhs != foundCls.getRHS() && cl.size() == foundCls.getSize())
            goto end;

        //If the size of this clause is the same of the base clause, then
        //there is no point in using this clause as a base for another XOR
        //because exactly the same things will be found.
        if (cl.size() == foundCls.getSize())
            triedAlready.insert(offset);

        std::sort(tmpClause.begin(), tmpClause.end());
        foundCls.add(tmpClause, varsMissing);

        end:;
        //cout << "Not OK" << endl;

        //Clear 'seen2'
        for(const Lit tmp_lit: tmpClause) {
            seen2[tmp_lit.var()] = false;
        }
    }
}

void XorFinder::findXorMatch(
    watch_subarray_const occ
    , const Lit lit
    , FoundXors& foundCls
) {
    maxTimeFindXors -= occ.size();
    for (watch_subarray::const_iterator
        it = occ.begin(), end = occ.end()
        ; it != end
        ; it++
    ) {
        //Deal with binary
        if (it->isBinary()) {
            if (//Only once per binary
                lit < it->lit2()
                //only for correct binary
                && seen[it->lit2().var()]
            ) {
                tmpClause.clear();
                tmpClause.push_back(lit);
                tmpClause.push_back(it->lit2());

                foundCls.add(tmpClause, varsMissing);
                maxTimeFindXors-=5;
                if (foundCls.foundAll())
                    break;
            }

            continue;
        }

        //Deal with tertiary
        if (it->isTri()) {
            if (//Only once per tri
                lit < it->lit2() && it->lit2() < it->lit3()

                //Only for correct tri
                && seen[it->lit2().var()] && seen[it->lit3().var()]
            ) {
                bool rhs = true;
                rhs ^= lit.sign();
                rhs ^= it->lit2().sign();
                rhs ^= it->lit3().sign();

                if (rhs == foundCls.getRHS() || foundCls.getSize() > 3) {
                    tmpClause.clear();
                    tmpClause.push_back(lit);
                    tmpClause.push_back(it->lit2());
                    tmpClause.push_back(it->lit3());

                    foundCls.add(tmpClause, varsMissing);
                    maxTimeFindXors-=5;
                    if (foundCls.foundAll())
                        break;
                }

            }
            continue;
        }

        //Deal with clause
        const ClOffset offset = it->getOffset();
        const Clause& cl = *solver->clAllocator.getPointer(offset);
        if (cl.freed())
            continue;

        //Must not be larger than the original clause
        if (cl.size() > foundCls.getSize())
            continue;

        //Doesn't contain literals not in the original clause
        if ((cl.abst | foundCls.getAbst()) != foundCls.getAbst())
            continue;

        //Check RHS
        bool rhs = true;
        for (const Lit cl_lit :cl) {
            //early-abort, contains literals not in original clause
            if (!seen[cl_lit.var()])
                goto end;

            rhs ^= cl_lit.sign();
        }
        //either the invertedness has to match, or the size must be smaller
        if (rhs != foundCls.getRHS() && cl.size() == foundCls.getSize())
            continue;

        //If the size of this clause is the same of the base clause, then
        //there is no point in using this clause as a base for another XOR
        //because exactly the same things will be found.
        if (cl.size() == foundCls.getSize())
            triedAlready.insert(offset);

        foundCls.add(cl, varsMissing);
        maxTimeFindXors-=5;
        if (foundCls.foundAll())
            break;

        end:;
    }
}

size_t XorFinder::memUsed() const
{
    size_t mem = 0;
    mem += xors.capacity()*sizeof(Xor);
    mem += xorOcc.capacity()*sizeof(vector<uint32_t>);
    for(size_t i = 0; i < xorOcc.size(); i++) {
        mem += xorOcc[i].capacity()*sizeof(uint32_t);
    }
    mem += triedAlready.size()*sizeof(ClOffset); //TODO very much under-estimates
    mem += blocks.capacity()*sizeof(vector<Var>);
    for(size_t i = 0; i< blocks.size(); i++) {
        mem += blocks[i].capacity()*sizeof(Var);
    }
    mem += varToBlock.capacity()*sizeof(size_t);

    //Temporary
    mem += tmpClause.capacity()*sizeof(Lit);
    mem += varsMissing.capacity()*sizeof(uint32_t);

    //Temporaries for putting xors into matrix, and extracting info from matrix
    mem += outerToInterVarMap.capacity()*sizeof(size_t);
    mem += interToOUterVarMap.capacity()*sizeof(size_t);

    return mem;
}

void XorFinder::Stats::printShort() const
{
    cout
    << "c XOR finding "
    << " Num XORs: " << std::setw(6) << foundXors
    << " avg size: " << std::setw(4) << std::fixed << std::setprecision(1)
    << ((double)sumSizeXors/(double)foundXors)
    << " T: "
    << std::fixed << std::setprecision(2) << findTime
    << " T-o: " << (time_outs ? "Y" : "N")
    << endl;

    cout
    << "c Cut XORs into " << numBlocks << " block(s)"
    << " sum vars: " << numVarsInBlocks
    << " T: " << std::fixed << std::setprecision(2) << blockCutTime
    << endl;

    cout
    << "c Extracted XOR info."
    << " Units: " << newUnits
    << " Bins: " << newBins
    << " 0-depth-assigns: " << zeroDepthAssigns
    << " T: " << std::fixed << std::setprecision(2) << extractTime
    << endl;
}

void XorFinder::Stats::print(const size_t numCalls) const
{
    cout << "c --------- XOR STATS ----------" << endl;
    printStatsLine("c num XOR found on avg"
        , (double)foundXors/(double)numCalls
        , "avg size"
    );

    printStatsLine("c XOR avg size"
        , (double)sumSizeXors/(double)foundXors
    );

    printStatsLine("c XOR 0-depth assings"
        , zeroDepthAssigns
    );

    printStatsLine("c XOR unit found"
        , newUnits
    );

    printStatsLine("c XOR bin found"
        , newBins
    );

    printStatsLine("c XOR finding time"
        , findTime
        , (double)time_outs/(double)numCalls*100.0
        , "time-out"
    );
    cout << "c --------- XOR STATS END ----------" << endl;
}

XorFinder::Stats& XorFinder::Stats::operator+=(const XorFinder::Stats& other)
{
    //Time
    findTime += other.findTime;
    extractTime += other.extractTime;
    blockCutTime += other.blockCutTime;

    //XOR
    foundXors += other.foundXors;
    sumSizeXors += other.sumSizeXors;
    numVarsInBlocks += other.numVarsInBlocks;
    numBlocks += other.numBlocks;

    //Usefulness
    time_outs += other.time_outs;
    newUnits += other.newUnits;
    newBins += other.newBins;
    zeroDepthAssigns += other.zeroDepthAssigns;

    return *this;
}
