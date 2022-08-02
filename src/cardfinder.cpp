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

#include "constants.h"
#include "cardfinder.h"
#include "time_mem.h"
#include "solver.h"
#include "watched.h"
#include "watchalgos.h"

#include <limits>
#include <sstream>

//TODO read: https://sat-smt.in/assets/slides/daniel1.pdf

using namespace CMSat;
using std::cout;
using std::endl;

CardFinder::CardFinder(Solver* _solver) :
    solver(_solver)
    , seen(solver->seen)
    , seen2(solver->seen2)
    , toClear(solver->toClear)
{
}

//TODO order encoding!
//Also, convert to order encoding.

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

//See "Detecting cardinality constraints in CNF"
//By Armin Biere, Daniel Le Berre, Emmanuel Lonca, and Norbert Manthey
//Sect. 3.3 -- two-product encoding
//
void CardFinder::find_two_product_atmost1() {
    vector<vector<Lit>> new_cards;
    for(size_t at_row = 0; at_row < cards.size(); at_row++) {
        vector<Lit>& card_row = cards[at_row];
        seen2[at_row] = 1;
        if (card_row.empty()) {
            continue;
        }
        Lit r = card_row[0];

        //find min(NAG(r))
        Lit l = lit_Undef;
        for(const auto& ws: solver->watches[r]) {
            if (!ws.isBin()) continue;
            if (l == lit_Undef) {
                l = ws.lit2();
            } else {
                if (ws.lit2() < l) {
                    l = ws.lit2();
                }
            }
        }
        if (l == lit_Undef) {
            continue;
        }

        //find the column
        for(const auto& ws: solver->watches[l]) {
            if (ws.isBin()) {
                Lit c = ws.lit2();
                if (c == r) continue;
                for(const auto& ws2: solver->watches[c]) {
                    if (ws2.isIdx()) {
                        size_t at_col = ws2.get_idx();
                        if (seen2[at_col]) {
                            //only do row1->row2, it's good enough
                            //don't do reverse too
                            continue;
                        }
                        vector<Lit>& card_col = cards[at_col];
                        if (card_col.empty()) continue;

                        /*cout << "c [cardfind] Potential card for"
                        << " row: " << print_card(card_row)
                        << " -- col: " << print_card(card_col)
                        << endl;*/

                        vector<Lit> card;

                        //mark all lits in row's bin-connected graph
                        for(const Lit row: card_row) {
                            for(const auto& ws3: solver->watches[row]) {
                                if (ws3.isBin()) {
                                    seen[ws3.lit2().toInt()] = 1;
                                }
                            }
                        }

                        //find matching column
                        for(const Lit col: card_col) {
                            for(const auto& ws3: solver->watches[col]) {
                                if (ws3.isBin()) {
                                    Lit conn_lit = ws3.lit2();
                                    if (seen[conn_lit.toInt()]) {
                                        //cout << "part of card: " << ~conn_lit << endl;
                                        card.push_back(~conn_lit);
                                    }
                                }
                            }
                        }

                        if (card.size() > 2) {
                            /*cout << "Reassembled two product: "
                            << print_card(card) << endl;*/
                            new_cards.push_back(card);
                        }

                        //unmark
                        for(const Lit row: card_row) {
                            for(const auto& ws3: solver->watches[row]) {
                                if (ws3.isBin()) {
                                    seen[ws3.lit2().toInt()] = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    size_t old_size = cards.size();
    cards.resize(cards.size()+new_cards.size());
    for(auto& card: new_cards) {
        std::sort(card.begin(), card.end());
        std::swap(cards[old_size], card);
        old_size++;
    }

    //clear seen2
    for(size_t at_row = 0; at_row < cards.size(); at_row++) {
        seen2[at_row] = 0;
    }
}

void CardFinder::print_cards(const vector<vector<Lit>>& card_constraints) const {
    for(const auto& card: card_constraints) {
        cout << "c [cardfind] final: " << print_card(card) << endl;
    }
}


void CardFinder::deal_with_clash(vector<uint32_t>& clash) {

    vector<uint32_t> idx_pos;
    vector<uint32_t> idx_neg;

    for(uint32_t var: clash) {
        Lit lit = Lit(var, false);
        if (seen[lit.toInt()] == 0 || seen[(~lit).toInt()] == 0) {
            continue;
        }

        //cout << "c [cardfind] Clash on var " << lit << endl;
        for(auto ws: solver->watches[lit]) {
            if (ws.isIdx()) {
                idx_pos.push_back(ws.get_idx());

                /*cout << "c [cardfind] -> IDX " << ws.get_idx() << ": "
                << print_card(cards[ws.get_idx()]) << endl;*/
            }
        }
        for(auto ws: solver->watches[~lit]) {
            if (ws.isIdx()) {
                idx_neg.push_back(ws.get_idx());

                /*cout << "c [cardfind] -> IDX " << ws.get_idx() << ": "
                << print_card(cards[ws.get_idx()]) << endl;*/
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
                /*cout << "c [cardfind] -> Combined card: "
                << print_card(new_card) << endl;*/

                //add the new cardinality constraint
                for(Lit l: new_card) {
                    solver->watches[l].push(Watched(cards.size(), WatchType::watch_idx_t));
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

void CardFinder::clean_empty_cards()
{
    size_t j = 0;
    for(size_t i = 0; i < cards.size(); i++) {
        if (!cards[i].empty()) {
            std::swap(cards[j], cards[i]);
            j++;
        }
    }
    cards.resize(j);
}

//See "Detecting cardinality constraints in CNF"
//By Armin Biere, Daniel Le Berre, Emmanuel Lonca, and Norbert Manthey
//Sect. 3.1 -- greeedy algorithm. "S" there is "lits_in_card" here
//
void CardFinder::find_pairwise_atmost1()
{
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
                solver->watches[l_c].push(Watched(cards.size(), WatchType::watch_idx_t));
                solver->watches.smudge(l_c);
            }
            total_sizes+=lits_in_card.size();
            std::sort(lits_in_card.begin(), lits_in_card.end());
            verb_print(1, "found simple card " << print_card(lits_in_card) << " on lit " << l);

            //fast push-back
            cards.resize(cards.size()+1);
            std::swap(cards[cards.size()-1], lits_in_card);

        } else {
            //cout << "lits_in_card.size():" << lits_in_card.size() << endl;
            //cout << "Found none for " << l << endl;
        }
    }

    //Now deal with so-called "Nested encoding"
    //  i.e. x1+x2+x4+x5 <= 1
    //  divided into the cardinality constraints
    //  x1+x2+x3 <= 1 and \not x3+x4+x5 <= 1
    //See sect. 3.2 of same paper
    //
    std::sort(toClear.begin(), toClear.end());
    vector<uint32_t> vars_with_clash;
    get_vars_with_clash(toClear, vars_with_clash);
    deal_with_clash(vars_with_clash);
    for(const Lit x: toClear) {
        seen[x.toInt()] = 0;
    }
    toClear.clear();
}

void CardFinder::find_cards()
{
    cards.clear();
    double myTime = cpuTime();

    find_pairwise_atmost1();
    find_two_product_atmost1();

    //print result
    clean_empty_cards();
    if (solver->conf.verbosity) {
        verb_print(1, "[cardfind] All constraints below:");
        print_cards(cards);
    }

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
