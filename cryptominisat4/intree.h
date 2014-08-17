/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2014, Mate Soos. All rights reserved.
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

#include "cloffset.h"
#include "solvertypes.h"

#include <vector>
#include <deque>
using std::vector;
using std::deque;

namespace CMSat {

class Solver;

class InTree
{
public:
    InTree(Solver* _solver);
    bool intree_probe();

private:
    void randomize_roots();
    void handle_lit_popped_from_queue(const Lit lit);
    bool empty_failed_list();
    void fill_roots();
    bool watches_only_contains_nonbin(const Lit lit) const;
    bool replace_until_fixedpoint();
    void enqueue(const Lit lit);

    void setup();
    void build_intree();
    void do_one();
    bool tree_look();

    vector<Lit> roots;
    vector<Lit> failed;
    deque<Lit> queue;
    vector<char> depth_failed;
    int64_t bogoprops_to_use;
    int64_t bogoprops_remain;

    Solver* solver;
    vector<uint16_t>& seen;
};

}

