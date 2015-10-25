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

#include "gtest/gtest.h"
#include "cryptominisat4/solvertypesmini.h"
#include <vector>
#include <ostream>
#include <iostream>
#include <sstream>
#include "src/solver.h"
#include "cryptominisat4/cryptominisat.h"

using std::cout;
using std::endl;
using std::vector;
using std::string;
using std::istringstream;
using std::stringstream;
using namespace CMSat;

// trim from start
static inline std::string &ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

vector<Lit> str_to_cl(const string& data)
{
    vector<string> tokens;
    stringstream ss(data);
    string token;
    while (getline(ss,token, ','))
    {
        tokens.push_back(token);
    }

    vector<Lit> ret;
    for(string& token: tokens) {
        string trimmed = trim(token);
        size_t endptr;
        long i = std::stol(trimmed, &endptr);
        if (endptr != trimmed.size()) {
            cout << "Error, input token: '" << token << "' wasn't completely used up, wrong token!" << endl;
            exit(-1);
        }
        Lit lit(abs(i)-1, i < 0);
        ret.push_back(lit);
    }
    //cout << "input is: " << data << " LITs is: " << ret << endl;
    return ret;
}

vector<vector<Lit> > str_to_vecs(const string& data)
{
    vector<vector<Lit> > ret;
    stringstream ss(data);
    string token;
    while (getline(ss,token, ';'))
    {
        ret.push_back(str_to_cl(token));
    }

    return ret;
}

void add_cls(vector<vector<Lit> >& ret,
             const Solver* s,
             const vector<ClOffset>& offsets)
{
    for(auto off: offsets) {
        Clause& cl = *s->cl_alloc.ptr(off);
        vector<Lit> lits;
        for(Lit l: cl) {
            lits.push_back(l);
        }
        std::sort(lits.begin(), lits.end());
        ret.push_back(lits);
    }
}

void add_impl_cls(
    vector<vector<Lit> >& ret,
    const Solver* s,
    const bool add_irred,
    const bool add_red)
{
    for(size_t i = 0; i < s->nVars()*2; i++) {
        Lit lit = Lit::toLit(i);
        for(const Watched& ws: s->watches[lit]) {
            if (ws.isBin()
                && lit < ws.lit2()
                && ((add_irred && !ws.red()) || (add_red && ws.red()))
            ) {
                vector<Lit> cl;
                cl.push_back(lit);
                cl.push_back(ws.lit2());
                ret.push_back(cl);
            }

            if (ws.isTri()
                && lit < ws.lit2()
                && lit < ws.lit3()
                && ((add_irred && !ws.red()) || (add_red && ws.red()))
            ) {
                vector<Lit> cl;
                cl.push_back(lit);
                cl.push_back(ws.lit2());
                cl.push_back(ws.lit3());
                ret.push_back(cl);
            }
        }
    }
}

vector<vector<Lit> > get_irred_cls(const Solver* s)
{
    vector<vector<Lit> > ret;
    add_cls(ret, s, s->longIrredCls);
    add_impl_cls(ret, s, true, false);

    return ret;
}

struct VecVecSorter
{
    bool operator()(const vector<Lit>&a, const vector<Lit>& b) const
    {
        if (a.size() != b.size()) {
            return a.size() < b.size();
        }

        for(size_t i = 0; i < a.size(); i++) {
            if (a[i] != b[i]) {
                return a[i] < b[i];
            }
        }
        return false;
    }
};

void check_fuzzy_equal(
    vector<vector<Lit> >& cls_expected,
    vector<vector<Lit> >& cls_actual)
{
    for(vector<Lit>& x: cls_actual) {
        std::sort(x.begin(), x.end());
    }
    for(vector<Lit>& x: cls_expected) {
        std::sort(x.begin(), x.end());
    }

    VecVecSorter sorter;
    std::sort(cls_actual.begin(), cls_actual.end(), sorter);
    std::sort(cls_expected.begin(), cls_expected.end(), sorter);

    EXPECT_EQ(cls_expected, cls_actual);
}

string print(const vector<vector<Lit> >& cls)
{
    std::stringstream ss;
    for(auto cl: cls) {
        ss << cl << endl;
    }
    return ss.str();
}

void check_irred_cls_eq(const Solver* s, const string& data)
{
    vector<vector<Lit> > cls_expected = str_to_vecs(data);
    vector<vector<Lit> > cls = get_irred_cls(s);

    check_fuzzy_equal(cls_expected, cls);
}

void check_irred_cls_contains(const Solver* s, const string& data)
{
    vector<Lit> looking_for = str_to_cl(data);
    vector<vector<Lit> > cls = get_irred_cls(s);

    bool found_cl = false;
    for(auto cl: cls) {
        if (cl == looking_for) {
            found_cl = true;
            break;
        }

    }
    if (!found_cl) {
        cout << "Expected to find: " << looking_for << endl;
        cout << "But only found  : ";
        for(auto cl: cls) {
            cout << cl << ", ";
        }
        cout << endl;
    }
    EXPECT_TRUE(found_cl);
}

void print_model(const SATSolver&s)
{
    assert(s.okay());
    for(size_t i = 0; i < s.nVars(); i++) {
        cout << "Model [" << i << "]: " << s.get_model()[i] << endl;
    }
}

void check_set_lits(const Solver* s, const std::string& data)
{
    vector<Lit> lits = str_to_cl(data);
    std::sort(lits.begin(), lits.end());

    vector<Lit> set_lits = s->get_zero_assigned_lits();
    std::sort(set_lits.begin(), set_lits.end());
    EXPECT_EQ(lits, set_lits);
}

string print_cache(const vector<LitExtra>& c)
{
    std::stringstream ss;
    for(LitExtra a: c) {
        ss << a.getLit() << "(irred: " << a.getOnlyIrredBin() << " ), ";
    }
    return ss.str();
}

void check_impl_cache_contains(const Solver* s, const std::string& data)
{
    vector<Lit> lits = str_to_cl(data);
    assert(lits.size() == 2);

    const vector<LitExtra>& cache_lits = s->implCache[lits[0]].lits;
    /*cout << "cache[0]: " << print_cache(s->implCache[Lit(0, false)].lits) << endl;
    cout << "cache[1]: " << print_cache(s->implCache[Lit(1, false)].lits) << endl;
    cout << "cache[2]: " << print_cache(s->implCache[Lit(2, false)].lits) << endl;

    cout << "cache[~0]: " << print_cache(s->implCache[Lit(0, true)].lits) << endl;
    cout << "cache[~1]: " << print_cache(s->implCache[Lit(1, true)].lits) << endl;
    cout << "cache[~2]: " << print_cache(s->implCache[Lit(2, true)].lits) << endl;
    */
    bool inside = false;
    for(LitExtra l: cache_lits) {
        if (l.getLit() == lits[1])
            inside = true;
    }
    EXPECT_TRUE(inside);
}

// void add_to_cache_irred(Solver* s, const string& data)
// {
//     vector<Lit> lits = str_to_cl(data);
//     assert(lits.size() == 2);
//     assert(s->implCache.size() > lits[0].toInt());
//     s->implCache[lits[0]].lits.push_back(LitExtra(lits[1], true));
// }


// string print(const vector<Lit>& dat) {
//     std::stringstream m;
//     for(size_t i = 0; i < dat.size();) {
//         m << dat[i];
//         i++;
//         if (i < dat.size()) {
//             m << ", ";
//         }
//     }
//     return m.str();
// }
