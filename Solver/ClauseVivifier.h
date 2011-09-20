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

#ifndef CLAUSEVIVIFIER_H
#define CLAUSEVIVIFIER_H

#include <vector>
#include "constants.h"
using std::vector;

class ThreadControl;
class Clause;

class ClauseVivifier {
    public:
        ClauseVivifier(ThreadControl* control);
        const bool vivify();

    private:

        //Actual algorithms used
        const bool vivifyClausesNormal();
        const bool vivifyClausesCache(vector<Clause*>& clauses);

        ///Sort clauses according to size
        struct SortBySize
        {
            const bool operator () (const Clause* x, const Clause* y);
        };

        uint32_t numCalls;
        ThreadControl* control;
};

#endif //CLAUSEVIVIFIER_H
