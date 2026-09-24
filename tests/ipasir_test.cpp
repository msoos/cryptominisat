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

#include "gtest/gtest.h"
#include <string>
#include <vector>
#include <initializer_list>
#include <cstdio>
extern "C" {
#include "src/ipasir.h"
}

TEST(ipasir_interface, start)
{
    void* s = ipasir_init();
    ipasir_release(s);
}

TEST(ipasir_interface, sat)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    int val = ipasir_val(s, 1);
    EXPECT_EQ(val, 1);

    ipasir_release(s);
}

TEST(ipasir_interface, sat2)
{
    void* s = ipasir_init();
    ipasir_add(s, -1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    ipasir_add(s, 1);
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    int val = ipasir_val(s, 1);
    EXPECT_EQ(val, 1);
    val = ipasir_val(s, 2);
    EXPECT_EQ(val, 2);

    ipasir_release(s);
}

TEST(ipasir_interface, sat4)
{
    void* s = ipasir_init();
    ipasir_add(s, -2);
    ipasir_add(s, -3);
    ipasir_add(s, 0);

    ipasir_add(s, -1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    ipasir_add(s, 1);
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    int val = ipasir_val(s, 1);
    EXPECT_EQ(val, 1);

    val = ipasir_val(s, 2);
    EXPECT_EQ(val, 2);

    val = ipasir_val(s, 3);
    EXPECT_EQ(val, -3);

    ipasir_release(s);
}


TEST(ipasir_interface, unsat)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 0);
    ipasir_add(s, -1);
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    ipasir_release(s);
}

TEST(ipasir_interface, unsat2)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    ipasir_add(s, -1);
    ipasir_add(s, -2);
    ipasir_add(s, 0);

    ipasir_add(s, 1);
    ipasir_add(s, -2);
    ipasir_add(s, 0);

    ipasir_add(s, -1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    ipasir_release(s);
}

TEST(ipasir_interface, unsat_empty)
{
    void* s = ipasir_init();
    ipasir_add(s, 0);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    ipasir_release(s);
}


TEST(ipasir_interface, assump)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    int used = ipasir_failed(s, -1);
    EXPECT_EQ(used, 1);

    ipasir_release(s);
}

TEST(ipasir_interface, assump_multi)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);
    EXPECT_EQ(ipasir_failed(s, -1), 1);

    //Redo with 2
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    ipasir_assume(s, -2);
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);
    EXPECT_EQ(ipasir_failed(s, -1), 0);
    EXPECT_EQ(ipasir_failed(s, -2), 1);

    //final, it should be SAT
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    ipasir_release(s);
}

TEST(ipasir_interface, assump_multi2)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 2);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    //add assump 2 as well
    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);
    EXPECT_EQ(ipasir_failed(s, -1), 1);
    EXPECT_EQ(ipasir_failed(s, -2), 1);

    //final, it should be SAT
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    ipasir_release(s);
}

TEST(ipasir_interface, assump_multi3)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 3);
    ipasir_add(s, 0);

    ipasir_add(s, -7);
    ipasir_add(s, -2);
    ipasir_add(s, 0);

    ipasir_add(s, 1);
    ipasir_add(s, 4);
    ipasir_add(s, 6);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    //one assum
    ipasir_assume(s, -1);
    ipasir_assume(s, -3);
    ipasir_assume(s, -4);
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);
    EXPECT_EQ(ipasir_failed(s, -1), 1);
    EXPECT_EQ(ipasir_failed(s, -2), 0);
    EXPECT_EQ(ipasir_failed(s, -3), 1);
    EXPECT_EQ(ipasir_failed(s, -4), 0);
    EXPECT_EQ(ipasir_failed(s, 4), 0);
    EXPECT_EQ(ipasir_failed(s, -6), 0);


    //one assum
    ipasir_assume(s, 7);
    ipasir_assume(s, 2);
    ipasir_assume(s, -6);
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);
    EXPECT_EQ(ipasir_failed(s, -1), 0);
    EXPECT_EQ(ipasir_failed(s, 2), 1);
    EXPECT_EQ(ipasir_failed(s, -3), 0);
    EXPECT_EQ(ipasir_failed(s, 7), 1);
    EXPECT_EQ(ipasir_failed(s, -6), 0);
    EXPECT_EQ(ipasir_failed(s, 6), 0);

    //final, it should be SAT
    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    ipasir_release(s);
}


TEST(ipasir_interface, assump_yevgeny)
{
    void* s = ipasir_init();

    ipasir_add(s, -1);
	ipasir_add(s, 0);

	int ret = ipasir_solve(s);
	EXPECT_EQ(ret, 10);

	ipasir_assume(s, 1);
	ret = ipasir_solve(s);
	EXPECT_EQ(ret, 20);

	int failed = ipasir_failed(s, 1);
    EXPECT_EQ(failed, 1);
}

TEST(ipasir_interface, assump2)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 2);
    ipasir_add(s, 3);
    ipasir_add(s, 4);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    ipasir_assume(s, -3);
    ipasir_assume(s, -4);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    //We check the ASSUMPTION LITERAL, not the conflict clause!
    int used = ipasir_failed(s, -1);
    EXPECT_EQ(used, 1);
    int used2 = ipasir_failed(s, -2);
    EXPECT_EQ(used2, 1);
    int used3 = ipasir_failed(s, -3);
    EXPECT_EQ(used3, 1);
    int used4 = ipasir_failed(s, -4);
    EXPECT_EQ(used4, 1);

    ipasir_release(s);
}

TEST(ipasir_interface, assump3)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 2);
    ipasir_add(s, -3);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    ipasir_assume(s, 3);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    int used = ipasir_failed(s, -1);
    EXPECT_EQ(used, 1);
    int used2 = ipasir_failed(s, -2);
    EXPECT_EQ(used2, 1);
    int used3 = ipasir_failed(s, 3);
    EXPECT_EQ(used3, 1);

    ipasir_release(s);
}

TEST(ipasir_interface, assump_clears)
{
    void* s = ipasir_init();
    ipasir_add(s, 1);
    ipasir_add(s, 0);

    ipasir_assume(s, -1);

    int ret = ipasir_solve(s);
    EXPECT_EQ(ret, 20);

    ret = ipasir_solve(s);
    EXPECT_EQ(ret, 10);

    ipasir_release(s);
}

TEST(ipasir_interface, ipasir_assump_beyond_problemvars)
{
    void* s = ipasir_init();
    ipasir_add(s, -7);
    ipasir_add(s, 0);
    ipasir_assume(s, 10);
    int ret = ipasir_solve(s);
    ASSERT_EQ(ret, 10);

    EXPECT_EQ(ipasir_val(s, 10), 10);
    ipasir_release(s);
}

TEST(ipasir_interface, ipasir_val)
{
    void* s = ipasir_init();
    ipasir_add(s, -7);
    ipasir_add(s, 0);
    ipasir_add(s, 8);
    ipasir_add(s, 0);
    int ret = ipasir_solve(s);
    ASSERT_EQ(ret, 10);

    EXPECT_EQ(ipasir_val(s, -7), -7);
    EXPECT_EQ(ipasir_val(s, 7), -7);
    EXPECT_EQ(ipasir_val(s, -8), 8);
    EXPECT_EQ(ipasir_val(s, 8), 8);
    ipasir_release(s);
}


static void add_cl(void* s, std::initializer_list<int> lits)
{
    for(int l: lits) ipasir_add(s, l);
    ipasir_add(s, 0);
}

TEST(ipasir_interface, signature)
{
    EXPECT_EQ(std::string(ipasir_signature()).rfind("cryptominisat-", 0), 0U);
}

TEST(ipasir_interface, incremental_sat_then_unsat)
{
    void* s = ipasir_init();
    add_cl(s, {1, 2});
    EXPECT_EQ(ipasir_solve(s), 10);

    add_cl(s, {-1});
    EXPECT_EQ(ipasir_solve(s), 10);
    EXPECT_EQ(ipasir_val(s, 1), -1);
    EXPECT_EQ(ipasir_val(s, 2), 2);

    add_cl(s, {-2});
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_EQ(ipasir_solve(s), 20);
    ipasir_release(s);
}

TEST(ipasir_interface, failed_only_responsible)
{
    void* s = ipasir_init();
    add_cl(s, {1});
    ipasir_assume(s, -1);
    ipasir_assume(s, 2);
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_EQ(ipasir_failed(s, -1), 1);
    EXPECT_EQ(ipasir_failed(s, 2), 0);
    ipasir_release(s);
}

TEST(ipasir_interface, failed_reset_between_solves)
{
    void* s = ipasir_init();
    add_cl(s, {1});
    add_cl(s, {2});
    ipasir_assume(s, -1);
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_EQ(ipasir_failed(s, -1), 1);

    ipasir_assume(s, -1);
    ipasir_assume(s, -2);
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_GE(ipasir_failed(s, -1) + ipasir_failed(s, -2), 1);

    ipasir_assume(s, -2);
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_EQ(ipasir_failed(s, -1), 0);
    EXPECT_EQ(ipasir_failed(s, -2), 1);
    ipasir_release(s);
}

TEST(ipasir_interface, failed_when_unsat_without_assumps)
{
    void* s = ipasir_init();
    add_cl(s, {1});
    add_cl(s, {-1});
    ipasir_assume(s, 2);
    EXPECT_EQ(ipasir_solve(s), 20);
    EXPECT_EQ(ipasir_failed(s, 2), 0);
    ipasir_release(s);
}

TEST(ipasir_interface, activation_literals)
{
    // clause i is "act_i -> x_i", with act_i = 10+i
    void* s = ipasir_init();
    for(int i = 1; i <= 5; i++) add_cl(s, {-(10+i), i});
    add_cl(s, {-1, -2, -3, -4, -5});

    for(int i = 1; i <= 5; i++) ipasir_assume(s, 10+i);
    EXPECT_EQ(ipasir_solve(s), 20);
    for(int i = 1; i <= 5; i++) EXPECT_EQ(ipasir_failed(s, 10+i), 1);

    for(int i = 1; i <= 4; i++) ipasir_assume(s, 10+i);
    EXPECT_EQ(ipasir_solve(s), 10);
    for(int i = 1; i <= 4; i++) EXPECT_EQ(ipasir_val(s, i), i);
    EXPECT_EQ(ipasir_val(s, 5), -5);
    ipasir_release(s);
}

TEST(ipasir_interface, pigeonhole)
{
    // 4 pigeons, 3 holes; var p*3+h+1 = pigeon p in hole h
    void* s = ipasir_init();
    for(int p = 0; p < 4; p++) {
        for(int h = 0; h < 3; h++) ipasir_add(s, p*3+h+1);
        ipasir_add(s, 0);
    }
    for(int h = 0; h < 3; h++)
        for(int p1 = 0; p1 < 4; p1++)
            for(int p2 = p1+1; p2 < 4; p2++)
                add_cl(s, {-(p1*3+h+1), -(p2*3+h+1)});
    EXPECT_EQ(ipasir_solve(s), 20);
    ipasir_release(s);
}

TEST(ipasir_interface, random_model_satisfies_clauses)
{
    std::vector<std::vector<int>> cls;
    uint32_t seed = 12345;
    auto rnd = [&](uint32_t n) { seed = seed*1103515245U + 12345U; return (seed >> 16) % n; };
    const int nvars = 60;
    for(int i = 0; i < 200; i++) {
        std::vector<int> cl;
        for(int j = 0; j < 3; j++) {
            int v = rnd(nvars)+1;
            cl.push_back(rnd(2) ? v : -v);
        }
        cls.push_back(cl);
    }

    void* s = ipasir_init();
    for(size_t done = 0; done < cls.size();) {
        for(size_t i = 0; i < 50; i++, done++) {
            for(int l: cls[done]) ipasir_add(s, l);
            ipasir_add(s, 0);
        }
        ASSERT_EQ(ipasir_solve(s), 10);
        for(size_t i = 0; i < done; i++) {
            bool sat = false;
            for(int l: cls[i]) sat |= ipasir_val(s, l) == l;
            EXPECT_TRUE(sat) << "clause " << i << " not satisfied";
        }
    }
    ipasir_release(s);
}

TEST(ipasir_interface, simplify)
{
    void* s = ipasir_init();
    add_cl(s, {1, 2});
    add_cl(s, {-1, 2});
    EXPECT_NE(ipasir_simplify(s), 20);
    EXPECT_EQ(ipasir_solve(s), 10);
    EXPECT_EQ(ipasir_val(s, 2), 2);

    add_cl(s, {-2});
    EXPECT_EQ(ipasir_simplify(s), 20);
    EXPECT_EQ(ipasir_solve(s), 20);
    ipasir_release(s);
}

static int never_terminate(void*) { return 0; }
static void learn_cb(void*, int*) {}

TEST(ipasir_interface, callbacks_accepted)
{
    void* s = ipasir_init();
    ipasir_set_terminate(s, nullptr, never_terminate);
    ipasir_set_learn(s, nullptr, 3, learn_cb);
    add_cl(s, {1, 2});
    add_cl(s, {-1});
    EXPECT_EQ(ipasir_solve(s), 10);
    EXPECT_EQ(ipasir_val(s, 2), 2);
    ipasir_release(s);
}

TEST(ipasir_interface, trace_proof)
{
    const char* fname = "ipasir_trace_proof.out";
    FILE* f = fopen(fname, "w");
    ASSERT_NE(f, nullptr);
    void* s = ipasir_init();
    ipasir_trace_proof(s, f);
    add_cl(s, {1, 2});
    add_cl(s, {-1, 2});
    add_cl(s, {1, -2});
    add_cl(s, {-1, -2});
    EXPECT_EQ(ipasir_solve(s), 20);
    ipasir_release(s);
    EXPECT_GT(ftell(f), 0);
    fclose(f);
    remove(fname);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
