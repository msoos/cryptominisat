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
#include <sstream>

using namespace CMSat;
using std::cout;
using std::endl;

CardFinder::CardFinder(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , toClear(solver->toClear)
{
}

std::string CardFinder::print_card(const vector<Lit>& lits) const {
    std::stringstream ss;
    for(size_t i = 0; i < lits.size(); i++) {
        ss << lits[i];
        if (i != lits.size()-1) {
            ss << ", ";
        }
    }

    return ss.str();
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

void CardFinder::get_vars_with_clash(const vector<Lit>& lits, vector<uint32_t>& clash) const {
    Lit last_lit = lit_Undef;
    for(const Lit x: lits) {
        if (x == ~last_lit) {
            clash.push_back(x.var());
        }
        last_lit = x;
    }
}

void CardFinder::print_cards(const vector<vector<Lit>>& card_constraints) const {
    for(const auto& card: card_constraints) {
        if (card.size() > 0) {
            cout << "c [cardfind] " << print_card(card) << endl;
        }
    }
}


void CardFinder::deal_with_clash(vector<uint32_t>& clash) {

    vector<uint32_t> idx_pos;
    vector<uint32_t> idx_neg;

    for(uint32_t var: clash) {
        Lit lit = Lit(var, false);
        if (seen[lit.toInt()]+seen[(~lit).toInt()] <= 1) {
            continue;
        }

        cout << "c [cardfind] Clash on var " << lit << endl;
        for(auto ws: solver->watches[lit]) {
            if (ws.isIdx()) {
                idx_pos.push_back(ws.get_idx());
                if (solver->conf.verbosity > 5) {
                    cout << "c [cardfind] -> IDX " << ws.get_idx() << ": "
                    << print_card(cards[ws.get_idx()]) << endl;
                }
            }
        }
        for(auto ws: solver->watches[~lit]) {
            if (ws.isIdx()) {
                idx_neg.push_back(ws.get_idx());
                if (solver->conf.verbosity > 5) {
                    cout << "c [cardfind] -> IDX " << ws.get_idx() << ": "
                    << print_card(cards[ws.get_idx()]) << endl;
                }
            }
        }

        //resolve each with each
        for(uint32_t pos: idx_pos) {
            for(uint32_t neg: idx_neg) {
                assert(pos != neg);
                //one has been removed already
                if (cards[pos].empty() || cards[neg].empty()) {
                    continue;
                }

                vector<Lit> new_card;
                bool found = false;
                for(Lit l: cards[pos]) {
                    if (l == lit) {
                        found = true;
                    } else {
                        new_card.push_back(l);
                    }
                }
                assert(found);

                for(Lit l: cards[neg]) {
                    if (l == ~lit) {
                        found = true;
                    } else {
                        new_card.push_back(l);
                    }
                }
                assert(found);

                std::sort(new_card.begin(), new_card.end());
                if (solver->conf.verbosity > 5) {
                    cout << "c [cardfind] -> Combined card: " << print_card(new_card) << endl;
                }

                //add the new cardinality constraint
                for(Lit l: new_card) {
                    solver->watches[l].push(Watched(cards.size()));
                }
                cards.push_back(new_card);
            }
        }

        //clear old cardinality constraints
        for(uint32_t pos: idx_pos) {
            cards[pos].clear();
        }
        for(uint32_t neg: idx_neg) {
            cards[neg].clear();
        }

        idx_pos.clear();
        idx_neg.clear();
    }
}

void CardFinder::find_cards()
{
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
            lits_in_card.push_back(l);
            for(const Lit l_c: lits_in_card) {
                if (!seen[l_c.toInt()]) {
                    toClear.push_back(l_c);
                }
                seen[l_c.toInt()]++;
                solver->watches[l_c].push(Watched(cards.size()));
                solver->watches.smudge(l_c);
            }
            total_sizes+=lits_in_card.size();
            std::sort(lits_in_card.begin(), lits_in_card.end());

            if (solver->conf.verbosity > 5) {
                cout << "c [cardfind]"
                << " Found card of size " << lits_in_card.size()
                << " for lit " << l << ": "
                << print_card(lits_in_card) << endl;
            }

            //fast push-back
            cards.resize(cards.size()+1);
            std::swap(cards[cards.size()-1], lits_in_card);

        } else {
            //cout << "lits_in_card.size():" << lits_in_card.size() << endl;
            //cout << "Found none for " << l << endl;
        }
    }

    std::sort(toClear.begin(), toClear.end());
    vector<uint32_t> vars_with_clash;
    get_vars_with_clash(toClear, vars_with_clash);
    deal_with_clash(vars_with_clash);

    cout << "c [cardfind] All constraints below:" << endl;
    print_cards(cards);

    //clean indexes
    for(auto& lit: solver->watches.get_smudged_list()) {
        auto& ws = solver->watches[lit];
        size_t j = 0;
        for(size_t i = 0; i < ws.size(); i++) {
            if (!ws[i].isIdx()) {
                ws[j++] = ws[i];
            }
        }
        ws.resize(j);
    }
    solver->watches.clear_smudged();
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
