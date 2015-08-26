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

#include "cloffset.h"
#include "solvertypes.h"
#include "propby.h"

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

    struct QueueElem
    {
        QueueElem(Lit _propagated, Lit _other_lit, bool _red) :
            propagated(_propagated)
            , other_lit(_other_lit)
            , red(_red)
        {}

        Lit propagated;
        Lit other_lit;
        bool red;
    };

    struct ResetReason
    {
        ResetReason(Var _var_reason_changed, PropBy _orig_propby) :
            var_reason_changed(_var_reason_changed)
            , orig_propby(_orig_propby)
        {}

        Var var_reason_changed;
        PropBy orig_propby;
    };

private:

    bool check_timeout_due_to_hyperbin();
    void unmark_all_bins();
    void randomize_roots();
    bool handle_lit_popped_from_queue(const Lit lit, const Lit propagating, const bool red);
    bool empty_failed_list();
    void fill_roots();
    bool watches_only_contains_nonbin(const Lit lit) const;
    bool replace_until_fixedpoint(bool& aborted);
    void enqueue(const Lit lit, const Lit other_lit, bool red_cl);

    void setup();
    void build_intree();
    void do_one();
    void tree_look();

    vector<Lit> roots;
    vector<Lit> failed;
    vector<ResetReason> reset_reason_stack;
    deque<QueueElem> queue;
    vector<char> depth_failed;
    int64_t bogoprops_to_use;
    int64_t bogoprops_remain;

    size_t hyperbin_added;
    size_t removedIrredBin;
    size_t removedRedBin;
    size_t numCalls = 0;

    Solver* solver;
    vector<uint16_t>& seen;
};

inline std::ostream& operator<<(std::ostream& os, const InTree::QueueElem& elem)
{
    if (elem.propagated == lit_Undef) {
        os << "NONE";
    } else {
        os << "prop:" << elem.propagated
        << " other_lit:" << elem.other_lit
        << " red: " << elem.red;
    }

    return os;
}

}

