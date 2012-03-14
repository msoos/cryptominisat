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

#include "RestartPrinter.h"
#include "ThreadControl.h"

#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

void RestartPrinter::printStatHeader() const
{
    if (control->conf.verbosity >= 2) {
        cout << "c "
        << "========================================================================================="
        << endl;
        cout << "c"
        << " types(t): F = full restart, N = normal restart" << endl;
        cout << "c"
        << " types(t): S = simplification begin/end, E = solution found" << endl;
        cout << "c"
        << " restart types(rt): st = static, dy = dynamic" << endl;

        cout << "c "
        << std::setw(2) << "t"
        //<< std::setw(3) << "rt"
        //<< std::setw(6) << "Rest"
        << std::setw(space) << "Confl"
        << std::setw(space) << "Vars"
        << std::setw(space) << "NormCls"
        << std::setw(space) << "BinCls"
        << std::setw(space) << "Learnts"
        << std::setw(space) << "ClLits"
        << std::setw(space) << "LtLits"
        << std::setw(space) << "MTavgCS"
        << std::setw(space) << "LTAvgG"
        //<< std::setw(space) << "STAvgG"
        << endl;
    }
}

void RestartPrinter::printRestartStat(const char* type)
{

    lastConflPrint = control->sumSolvingStats.numConflicts;

    if (control->conf.verbosity >= 2) {
        cout << "c "
        << std::setw(2) << type
        //<< std::setw(6) << control->starts
        << std::setw(space) << control->sumSolvingStats.numConflicts
        << std::setw(space) << control->getNumFreeVars()
        << std::setw(space) << control->clauses.size()
        << std::setw(space) << control->numBins
        << std::setw(space) << control->learnts.size() << " "
        << std::setw(space) << control->clausesLits << " "
        << std::setw(space) << control->learntsLits<< " ";

//         if (control->conflSizeHist.isvalid()) {
//             cout << std::setw(space) << std::fixed << std::setprecision(2)
//             << control->conflSizeHist.getAvgDouble();
//         } else {
//             cout << std::setw(space) << "no data";
//         }
//
//         if (control->glueHistory.getTotalNumeElems() > 0) {
//             cout << std::setw(space) << std::fixed << std::setprecision(2)
//             << control->glueHistory.getAvgAllDouble();
//         } else {
//             cout << std::setw(space) << "no data";
//         }
//         if (glueHistory.isvalid()) {
//             cout << std::setw(space) << std::fixed << std::setprecision(2) << glueHistory.getAvgDouble();
//         } else {
//             cout << std::setw(space) << "no data";
//         }
//
//         if (control->conf.doPrintAvgBranch) {
//             if (control->avgBranchDepth.isvalid())
//                 cout << std::setw(space) << control->avgBranchDepth.getAvgUInt();
//             else
//                 cout << std::setw(space) << "no data";
//         }

        #ifdef USE_GAUSS
        print_gauss_sum_stats();
        #endif //USE_GAUSS

        cout << endl;
    }
}

void RestartPrinter::printEndSearchStat()
{
    if (control->conf.verbosity >= 1)
        printRestartStat("E");
}
