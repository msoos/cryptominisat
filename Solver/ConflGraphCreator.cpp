/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#include "CommandControl.h"
#include <string>
#include <fstream>
using std::string;
using std::vector;

class PropByFull
{
    private:
        uint16_t type;
        uint16_t isize;
        Clause* clause;
        Lit lits[3];
        ClauseData data;

    public:
        PropByFull(PropBy orig
                    , Lit otherLit
                    , const ClauseAllocator& alloc
                    , const vector<ClauseData>& clauseData
                    , const vector<lbool>& assigns
        ) :
            type(10)
            , isize(0)
            , clause(NULL)
        {
            if (orig.getType() == binary_t || orig.getType() == tertiary_t) {
                lits[0] = otherLit;
                lits[1] = orig.getOtherLit();
                if (orig.getType() == tertiary_t) {
                    lits[2] = orig.getOtherLit2();
                    type = 2;
                    isize = 3;
                } else {
                    type = 1;
                    isize = 2;
                }
            }
            if (orig.isClause()) {
                if (orig.isNULL()) {
                    type = 0;
                    isize = 0;
                    clause = NULL;
                    return;
                }
                clause = alloc.getPointer(orig.getClause());
                data = clauseData[clause->getNum()];
                if (orig.getWatchNum()) std::swap(data[0], data[1]);
                isize = clause->size();
                type = 0;
            }
        }

        PropByFull() :
            type(0)
            , clause(NULL)
        {}

        PropByFull(const PropByFull& other) :
            type(other.type)
            , isize(other.isize)
            , clause(other.clause)
            , data(other.data)
        {
            memcpy(lits, other.lits, sizeof(Lit)*3);
        }

        PropByFull& operator=(const PropByFull& other)
        {
            type = other.type,
            isize = other.isize;
            clause = other.clause;
            data = other.data;
            //delete xorLits;
            memcpy(lits, other.lits, sizeof(Lit)*3);
            return *this;
        }

        const uint32_t size() const
        {
            return isize;
        }

        const bool isNULL() const
        {
            return type == 0 && clause == NULL;
        }

        const bool isClause() const
        {
            return type == 0;
        }

        const bool isBinary() const
        {
            return type == 1;
        }

        const bool isTri() const
        {
            return type == 2;
        }

        const Clause* getClause() const
        {
            return clause;
        }

        Clause* getClause()
        {
            return clause;
        }

        const Lit operator[](const uint32_t i) const
        {
            switch (type) {
                case 0:
                    assert(clause != NULL);

                    if (i <= 1) return (*clause)[data[i]];
                    if (i == data[0]) return (*clause)[(data[1] == 0 ? 1 : 0)];
                    if (i == data[1]) return (*clause)[(data[0] == 1 ? 0 : 1)];
                    return (*clause)[i];

                default :
                    return lits[i];
            }
        }
};

inline std::ostream& operator<<(std::ostream& os, const PropByFull& propByFull)
{

    if (propByFull.isBinary()) {
        os << propByFull[0] << " " << propByFull[1];
    } else if (propByFull.isTri()) {
        os <<propByFull[0] << " " << propByFull[1] << " " << propByFull[2];
    } else if (propByFull.isClause()) {
        if (propByFull.isNULL()) os << "null clause";
        else os << *propByFull.getClause();
    }
    return os;
}

const string CommandControl::simplAnalyseGraph(PropBy conflHalf, vector<Lit>& out_learnt, uint32_t& out_btlevel, uint32_t &glue)
{
    int pathC = 0;
    Lit p = lit_Undef;

    out_learnt.push_back(lit_Undef);      // (leave room for the asserting literal)
    int index   = trail.size() - 1;
    out_btlevel = 0;
    std::stringstream resolutions;

    PropByFull confl(conflHalf, failBinLit, *clAllocator, clauseData, assigns);
    do {
        assert(!confl.isNULL());          // (otherwise should be UIP)

        //Update resolutions output
        if (p != lit_Undef) {
            resolutions << " | ";
        }
        resolutions << "{ " << confl << " | " << pathC << " -- ";

        for (uint32_t j = (p == lit_Undef) ? 0 : 1, size = confl.size(); j != size; j++) {
            Lit q = confl[j];
            const Var my_var = q.var();

            if (!seen[my_var] //if already handled, don't care
                && varData[my_var].level > 0 //if it's assigned at level 0, it's assigned FALSE, so leave it out
            ) {
                seen[my_var] = 1;
                assert(varData[my_var].level <= decisionLevel());

                if (varData[my_var].level == decisionLevel()) {
                    pathC++;
                } else {
                    out_learnt.push_back(q);

                    //Backtracking level is largest of thosee inside the clause
                    if (varData[my_var].level > out_btlevel)
                        out_btlevel = varData[my_var].level;
                }
            }
        }
        resolutions << pathC << " }";

        //Go through the trail backwards, select the one that is to be resolved
        while (!seen[trail[index--].var()]);

        p = trail[index+1];
        confl = PropByFull(varData[p.var()].reason, p, *clAllocator, clauseData, assigns);
        seen[p.var()] = 0; // this one is resolved
        pathC--;
    } while (pathC > 0); //UIP when eveything goes through this one
    assert(pathC == 0);
    out_learnt[0] = ~p;

    // clear out seen
    for (uint32_t j = 0; j != out_learnt.size(); j++)
        seen[out_learnt[j].var()] = 0;    // ('seen[]' is now cleared)

    //Calculate glue
    glue = calcNBLevels(out_learnt);

    return resolutions.str();
}

void CommandControl::genConfGraph(const PropBy conflPart)
{
    assert(ok);
    assert(!conflPart.isNULL());

    static int num = 0;
    num++;
    std::stringstream s;
    s << "confls/" << "confl" << num << ".dot";
    std::string filename = s.str();

    std::ofstream file;
    file.open(filename.c_str());
    if (!file) {
        std::cout << "Couldn't open filename " << filename << std::endl;
        std::cout << "Maybe you forgot to create subdirectory 'confls'" << std::endl;
        exit(-1);
    }
    file << "digraph G {" << std::endl;

    //Special vertex indicating final conflict clause (to help us)
    vector<Lit> out_learnt;
    uint32_t out_btlevel, glue;
    const std::string res = simplAnalyseGraph(conflPart, out_learnt, out_btlevel, glue);
    file << "vertK -> dummy;";
    file << "dummy "
    << "[ "
    << " shape=record"
    << " , label=\"{"
    << " clause: " << out_learnt
    << " | btlevel: " << out_btlevel
    << " | glue: " << glue
    << " | {resol: | " << res << " }"
    << "}\""
    << " , fontsize=8"
    << " ];" << std::endl;

    PropByFull confl(conflPart, failBinLit, *clAllocator, clauseData, assigns);
    #ifdef VERBOSE_DEBUG_GEN_CONFL_DOT
    std::cout << "conflict: "<< confl << std::endl;
    #endif

    vector<Lit> lits;
    for (uint32_t i = 0; i < confl.size(); i++) {
        const Lit lit = confl[i];
        assert(value(lit) == l_False);
        lits.push_back(lit);

        //Put these into the impl. graph for sure
        seen[lit.var()] = true;
    }

    for (vector<Lit>::const_iterator it = lits.begin(), end = lits.end(); it != end; it++) {
        file << "x" << it->unsign() << " -> vertK "
        << "[ "
        << " label=\"" << lits << "\""
        << " , fontsize=8"
        << " ];" << std::endl;
    }

    //Special conflict vertex
    file << "vertK"
    << " [ "
    << "shape=\"box\""
    << ", style=\"filled\""
    << ", color=\"darkseagreen\""
    << ", label=\"K : " << lits << "\""
    << "];" << std::endl;

    //Calculate which literals are directly connected with the conflict
    vector<Lit> insideImplGraph;
    while(!lits.empty())
    {
        vector<Lit> newLits;
        for (uint32_t i = 0; i < lits.size(); i++) {
            PropBy reason = varData[lits[i].var()].reason;
            //Reason in NULL, so remove: it's got no antedecent
            if (reason.isNULL()) continue;

            #ifdef VERBOSE_DEBUG_GEN_CONFL_DOT
            std::cout << "Reason for lit " << lits[i] << " : " << reason << std::endl;
            #endif

            PropByFull prop(reason, lits[i], *clAllocator, clauseData, assigns);
            for (uint32_t i2 = 0; i2 < prop.size(); i2++) {
                const Lit lit = prop[i2];
                assert(value(lit) != l_Undef);

                //Don't put into the impl. graph lits at 0 decision level
                if (varData[lit.var()].level == 0) continue;

                //Already added, just drop
                if (seen[lit.var()]) continue;

                seen[lit.var()] = true;
                newLits.push_back(lit);
                insideImplGraph.push_back(lit);
            }
        }
        lits = newLits;
    }

    //Print edges
    for (uint32_t i = 0; i < trail.size(); i++) {
        const Lit lit = trail[i];

        //0-decision level means it's pretty useless to put into the impl. graph
        if (varData[lit.var()].level == 0) continue;

        //Not directly connected with the conflict, drop
        if (!seen[lit.var()]) continue;

        PropBy reason = varData[lit.var()].reason;

        //A decision variable, it is not propagated by any clause
        if (reason.isNULL()) continue;

        PropByFull prop(reason, lit, *clAllocator, clauseData, assigns);
        for (uint32_t i = 0; i < prop.size(); i++) {
            if (prop[i] == lit //This is being propagated, don't make a circular line
                || varData[prop[i].var()].level == 0 //'clean' clauses of 0-level lits
            ) continue;

            file << "x" << prop[i].unsign() << " -> x" << lit.unsign() << " "
            << "[ "
            << " label=\"";
            for(uint32_t i2 = 0; i2 < prop.size();) {
                //'clean' clauses of 0-level lits
                if (varData[prop[i2].var()].level == 0) {
                    i2++;
                    continue;
                }

                file << prop[i2];
                i2++;
                if (i2 != prop.size()) file << " ";
            }
            file << "\""
            << " , fontsize=8"
            << " ];" << std::endl;
        }
    }

    //Print vertex definitions
    for (uint32_t i = 0; i < trail.size(); i++) {
        Lit lit = trail[i];

        //Only vertexes that really have been used
        if (seen[lit.var()] == 0) continue;
        seen[lit.var()] = 0;

        file << "x" << lit.unsign()
        << " [ "
        << " shape=\"box\""
        //<< ", size = 0.8"
        << ", style=\"filled\"";
        if (varData[lit.var()].reason.isNULL())
            file << ", color=\"darkorange2\""; //decision var
        else
            file << ", color=\"darkseagreen4\""; //propagated var

        file << ", label=\"" << (lit.sign() ? "-" : "") << "x" << lit.unsign() << " @ " << varData[lit.var()].level << "\""
        << " ];" << std::endl;
    }

    file  << "}" << std::endl;
    file.close();

    std::cout << "c Printed implication graph (with conflict clauses) to file "
    << filename << std::endl;
}