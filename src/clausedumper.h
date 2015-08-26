/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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

#ifndef __CLAUSEDUMPER_H__
#define __CLAUSEDUMPER_H__

#include <ostream>
#include <vector>
#include <limits>
#include "cryptominisat4/solvertypesmini.h"
#include "cloffset.h"

using std::vector;

namespace CMSat {

class Solver;

class ClauseDumper
{
public:
    ClauseDumper(const Solver* _solver) :
        solver(_solver)
    {}

    void open_file_and_dump_red_clauses(const std::string& redDumpFname);
    void open_file_and_dump_irred_clauses(const std::string& irredDumpFname);

private:
    const Solver* solver;
    std::ostream* outfile = NULL;

    void open_dump_file(const std::string& filename);
    void dumpBinClauses(
        const bool dumpRed
        , const bool dumpIrred
    );
    void dumpTriClauses(
        const bool alsoRed
        , const bool alsoIrred
    );

    void dumpEquivalentLits();
    void dumpUnitaryClauses();
    void dumpRedClauses(const uint32_t maxSize);
    void dump_clauses(
        const vector<ClOffset>& cls
        , size_t max_size = std::numeric_limits<size_t>::max()
    );

    void dump_blocked_clauses();
    void dump_component_clauses();
    void dumpIrredClauses();

    vector<Lit> tmpCl;

};

}

#endif //__CLAUSEDUMPER_H__
