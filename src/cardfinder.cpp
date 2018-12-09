/******************************************
Copyright (c) 2018, Mate Soos

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

#include "cardfinder.h"
#include "time_mem.h"
#include "solver.h"
#include "watched.h"
#include "watchalgos.h"

#include <limits>

using namespace CMSat;
using std::cout;
using std::endl;

CardFinder::CardFinder(Solver* _solver) :
    solver(_solver)
{
}

bool CardFinder::find_connector(Lit lit1, Lit lit2) const
{
    //cout << "Finding connector: " << lit1 << ", " << lit2 << endl;

    //look through the shorter one
    if (solver->watches[lit1].size() > solver->watches[lit2].size()) {
        std::swap(lit1, lit2);
    }

    for(const Watched& x : solver->watches[lit1]) {
        if (!x.isBin()) {
            continue;
        }

        if (x.lit2() == lit2)
            return true;
    }
    return false;
}

void CardFinder::find_cards()
{
    vector<uint8_t>& seen = solver->seen2;
    vector<Lit>& toClear = solver->toClear;

    cards.clear();
    double myTime = cpuTime();
    assert(toClear.size() == 0);
    for (uint32_t i = 0; i < solver->nVars()*2; i++) {
        const Lit l = Lit::toLit(i);
        vector<Lit> lits_in_card;
        if (seen[l.toInt()]) {
            //cout << "Skipping " << l << " we have seen it before" << endl;
            continue;
        }

        for(const Watched& x : solver->watches[~l]) {
            if (!x.isBin()) {
                continue;
            }
            const Lit other = x.lit2();

            bool all_found = true;
            for(const Lit other2: lits_in_card) {
                if (!find_connector(other, ~other2)) {
                    all_found = false;
                    break;
                }
            }
            if (all_found) {
                lits_in_card.push_back(~other);
                // cout << "added to lits_in_card: " << ~other << endl;
            }
        }
        if (lits_in_card.size() > 1) {
            if (solver->conf.verbosity > 5) {
                cout << "c [cardfind] Found card of size " << lits_in_card.size()
                << " for lit " << l << endl;
            }
            for(const Lit l_c: lits_in_card) {
                seen[l_c.toInt()] = 1;
                toClear.push_back(l_c);
            }
            lits_in_card.push_back(l);
            cards.push_back(lits_in_card);
            total_sizes+=lits_in_card.size();
        } else {
            //cout << "lits_in_card.size():" << lits_in_card.size() << endl;
            //cout << "Found none for " << l << endl;
        }
    }
    for(const Lit x: toClear) {
        seen[x.toInt()] = 0;
    }
    toClear.clear();

    if (solver->conf.verbosity) {
        double avg = 0;
        if (cards.size() > 0) {
            avg = (double)total_sizes/(double)cards.size();
        }

        cout << "c [cardfind] "
        << "cards: " << cards.size()
        << " avg size: " << avg
        << solver->conf.print_times(cpuTime()-myTime)
        << endl;
    }
}
