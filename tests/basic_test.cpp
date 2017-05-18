/******************************************
Copyright (c) 2016, Mate Soos

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

#include <fstream>

#include "cryptominisat5/cryptominisat.h"
#include "src/solverconf.h"
using namespace CMSat;
#include <vector>
using std::vector;


TEST(normal_interface, start)
{
    SATSolver s;
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);
}

TEST(normal_interface, onelit)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);
}

TEST(normal_interface, twolit)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), false);
}

TEST(normal_interface, multi_solve_unsat)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), false);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_False);
        EXPECT_EQ( s.okay(), false);
    }
}

TEST(normal_interface, multi_solve_unsat_multi_thread)
{
    SATSolver s;
    s.set_num_threads(2);
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), false);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_False);
        EXPECT_EQ( s.okay(), false);
    }
}

TEST(normal_interface, solve_multi_thread)
{
    SATSolver s;
    s.set_num_threads(2);
    s.new_vars(2);
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    s.add_clause(vector<Lit>{Lit(0, true)});
    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_False);
    EXPECT_EQ(s.get_model()[1], l_True);
}

TEST(normal_interface, logfile)
{
    SATSolver* s = new SATSolver();
    s->log_to_file("testfile");
    s->new_vars(2);
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    lbool ret = s->solve();
    EXPECT_EQ( ret, l_True);
    delete s;

    std::ifstream infile("testfile");
    std::string line;
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::new_vars( 2 )");
    std::getline(infile, line);
    EXPECT_EQ(line, "1 2 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::solve(  )");
}

TEST(normal_interface, logfile2)
{
    SATSolver* s = new SATSolver();
    s->log_to_file("testfile");
    s->new_vars(2);
    s->add_clause(vector<Lit>{Lit(0, false)});
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    lbool ret = s->solve();
    s->add_clause(vector<Lit>{Lit(1, false)});
    ret = s->solve();
    delete s;

    std::ifstream infile("testfile");
    std::string line;
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::new_vars( 2 )");
    std::getline(infile, line);
    EXPECT_EQ(line, "1 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "1 2 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::solve(  )");
    std::getline(infile, line);
    EXPECT_EQ(line, "2 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::solve(  )");
}

TEST(normal_interface, logfile2_assumps)
{
    SATSolver* s = new SATSolver();
    s->log_to_file("testfile");
    s->new_vars(2);
    s->add_clause(vector<Lit>{Lit(0, false)});
    s->add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    std::vector<Lit> assumps {Lit(0, false), Lit(1, true)};
    lbool ret = s->solve(&assumps);
    s->add_clause(vector<Lit>{Lit(1, false)});
    assumps.clear();
    assumps.push_back(Lit(1, true));
    ret = s->solve(&assumps);
    delete s;

    std::ifstream infile("testfile");
    std::string line;
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::new_vars( 2 )");
    std::getline(infile, line);
    EXPECT_EQ(line, "1 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "1 2 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::solve( 1 -2 )");
    std::getline(infile, line);
    EXPECT_EQ(line, "2 0");
    std::getline(infile, line);
    EXPECT_EQ(line, "c Solver::solve( -2 )");
}

bool is_critical(const std::range_error&) { return true; }

TEST(xor_interface, xor_check_sat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<unsigned>{0U}, false);
    s.add_xor_clause(vector<unsigned>{0U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_False);
    }
    EXPECT_EQ( s.nVars(), 1u);
}

TEST(xor_interface, xor_check_unsat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ( s.okay(), true);
    }
    EXPECT_EQ( s.nVars(), 1u);
}

TEST(xor_interface, xor_check_solution_values)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ( s.okay(), true);
    }
    EXPECT_EQ( s.nVars(), 1u);
}

TEST(xor_interface, xor_check_solution_values2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    s.add_xor_clause(vector<uint32_t>{1U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ(s.get_model()[0], l_True);
        EXPECT_EQ(s.get_model()[1], l_True);
    }
    EXPECT_EQ( s.nVars(), 2u);
}

TEST(xor_interface, xor_check_solution_values3)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 0U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
}

TEST(xor_interface, xor_check_solution_values4)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 0U}, false);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.nVars(), 2u);
}


TEST(xor_interface, xor_check_solution_values5)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U}, true);
    vector<Lit> assump = {Lit(0, false)};
    lbool ret = s.solve(&assump);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);
    EXPECT_EQ(s.get_model()[0], l_True);
    EXPECT_EQ(s.get_model()[1], l_False);
    EXPECT_EQ( s.nVars(), 2u);
}

TEST(xor_interface, xor_check_solution_values6)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U}, false);
    vector<Lit> assump = {Lit(0, true)};
    lbool ret = s.solve(&assump);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_False);
    EXPECT_EQ(s.get_model()[1], l_False);
    EXPECT_EQ( s.nVars(), 2u);
}

TEST(xor_interface, xor_check_solution_values7)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, false);
    vector<Lit> assump = {Lit(0, false), Lit(1, false)};
    lbool ret = s.solve(&assump);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_True);
    EXPECT_EQ(s.get_model()[1], l_True);
    EXPECT_EQ(s.get_model()[2], l_False);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_3_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, true);
    s.add_xor_clause(vector<uint32_t>{0}, true);
    s.add_xor_clause(vector<uint32_t>{1}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_True);
    EXPECT_EQ(s.get_model()[1], l_True);
    EXPECT_EQ(s.get_model()[2], l_True);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_3_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<uint32_t>{0U}, true);
    s.add_xor_clause(vector<uint32_t>{1U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_True);
    EXPECT_EQ(s.get_model()[1], l_True);
    EXPECT_EQ(s.get_model()[2], l_False);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_4_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U, 3U}, false);
    s.add_xor_clause(vector<uint32_t>{0U}, false);
    s.add_xor_clause(vector<uint32_t>{1U}, false);
    s.add_xor_clause(vector<uint32_t>{2U}, false);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_False);
    EXPECT_EQ(s.get_model()[1], l_False);
    EXPECT_EQ(s.get_model()[2], l_False);
    EXPECT_EQ(s.get_model()[3], l_False);
    EXPECT_EQ( s.nVars(), 4u);
}

TEST(xor_interface, xor_4_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U, 3U}, true);
    s.add_xor_clause(vector<uint32_t>{0U}, false);
    s.add_xor_clause(vector<uint32_t>{1U}, false);
    s.add_xor_clause(vector<uint32_t>{2U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ(s.get_model()[0], l_False);
    EXPECT_EQ(s.get_model()[1], l_False);
    EXPECT_EQ(s.get_model()[2], l_True);
    EXPECT_EQ(s.get_model()[3], l_False);
    EXPECT_EQ( s.nVars(), 4u);
}

TEST(xor_interface, xor_very_long)
{
    SATSolver s;
    vector<uint32_t> vars;
    for(unsigned i = 0; i < 30; i++) {
        s.new_var();
        vars.push_back(i);
    }
    s.add_xor_clause(vars, false);
    for(unsigned i = 0; i < 29; i++) {
        s.add_xor_clause(vector<uint32_t>{i}, false);
    }
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    for(unsigned i = 0; i < 30; i++) {
        EXPECT_EQ(s.get_model()[i], l_False);
    }
    EXPECT_EQ( s.nVars(), 30u);
}

TEST(xor_interface, xor_very_long2)
{
    for(size_t num = 3; num < 30; num++) {
        SATSolver s;
        vector<uint32_t> vars;
        for(unsigned i = 0; i < num; i++) {
            s.new_var();
            vars.push_back(i);
        }
        s.add_xor_clause(vars, true);
        for(unsigned i = 0; i < num-1; i++) {
            s.add_xor_clause(vector<uint32_t>{i}, false);
        }
        lbool ret = s.solve();
        EXPECT_EQ( ret, l_True);
        for(unsigned i = 0; i < num-1; i++) {
            EXPECT_EQ(s.get_model()[i], l_False);
        }
        EXPECT_EQ(s.get_model()[num-1], l_True);
        EXPECT_EQ( s.nVars(), num);
    }
}

TEST(xor_interface, xor_check_unsat)
{
    SATSolver s;
    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_check_unsat_multi_thread)
{
    SATSolver s;
    s.set_num_threads(3);
    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), false);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_check_unsat_multi_solve_multi_thread)
{
    SATSolver s;
    s.set_num_threads(3);
    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0U, 1U}, false);
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.nVars(), 3u);

    s.add_xor_clause(vector<uint32_t>{0U}, false);
    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.get_model()[0], l_False);
    EXPECT_EQ( s.get_model()[1], l_False);
    EXPECT_EQ( s.get_model()[2], l_True);
    EXPECT_EQ( s.nVars(), 3u);

    s.add_xor_clause(vector<uint32_t>{1U}, true);
    ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, xor_norm_mix_unsat_multi_thread)
{
    SATSolver s;
    //s.set_num_threads(3);
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_xor_clause(vector<uint32_t>{0U, 1U, 2U}, false);
    s.add_clause(vector<Lit>{Lit(1, false)});
    s.add_clause(vector<Lit>{Lit(2, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.nVars(), 3u);
}

TEST(xor_interface, unit)
{
    SATSolver s;
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<Lit> units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 1u);
    EXPECT_EQ( units[0], Lit(0, false));
}

TEST(xor_interface, unit2)
{
    SATSolver s;
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<Lit> units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 1u);
    EXPECT_EQ( units[0], Lit(0, false));

    s.add_clause(vector<Lit>{Lit(1, true)});
    ret = s.solve();
    EXPECT_EQ( ret, l_True);

    units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 2u);
    EXPECT_EQ( units[0], Lit(0, false));
    EXPECT_EQ( units[1], Lit(1, true));
}

TEST(xor_interface, unit3)
{
    SATSolver s;
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, true)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<Lit> units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 2u);
    EXPECT_EQ( units[0], Lit(0, false));
    EXPECT_EQ( units[1], Lit(1, true));
}

TEST(xor_interface, xor1)
{
    SolverConf conf;
    conf.simplify_at_startup = true;
    conf.full_simplify_at_startup = true;
    SATSolver s(&conf);

    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0, 1}, false);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<std::pair<Lit, Lit> > pairs = s.get_all_binary_xors();
    EXPECT_EQ( pairs.size(), 1u);
}

TEST(xor_interface, xor2)
{
    SolverConf conf;
    conf.simplify_at_startup = true;
    conf.full_simplify_at_startup = true;
    SATSolver s(&conf);

    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0, 1}, false);
    s.add_xor_clause(vector<uint32_t>{1, 2}, false);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<std::pair<Lit, Lit> > pairs = s.get_all_binary_xors();
    EXPECT_EQ( pairs.size(), 2u);
}

TEST(xor_interface, abort_early)
{
    SATSolver s;
    s.set_no_simplify();
    s.set_no_equivalent_lit_replacement();

    s.set_num_threads(2);
    s.set_max_confl(0);
    s.new_vars(2);

    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, true)});
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, false)});
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, true)});

    lbool ret = s.solve();
    EXPECT_EQ( ret, l_Undef);
}

TEST(xor_interface, xor3)
{
    SolverConf conf;
    conf.simplify_at_startup = true;
    conf.simplify_at_every_startup = true;
    conf.full_simplify_at_startup = true;
    SATSolver s(&conf);

    s.new_vars(3);
    s.add_xor_clause(vector<uint32_t>{0, 1}, false);
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<std::pair<Lit, Lit> > pairs = s.get_all_binary_xors();
    EXPECT_EQ( pairs.size(), 1u);

    s.add_xor_clause(vector<uint32_t>{1, 2}, false);
    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    pairs = s.get_all_binary_xors();
    EXPECT_EQ( pairs.size(), 2u);
}

TEST(error_throw, multithread_newvar)
{
    SATSolver s;

    s.new_vars(3);
    EXPECT_THROW({
        s.set_num_threads(3);}
        , std::runtime_error);
}

TEST(error_throw, multithread_0)
{
    SATSolver s;

    EXPECT_THROW({
        s.set_num_threads(0);}
        , std::runtime_error);
}

TEST(error_throw, multithread_drat)
{
    SATSolver s;
    std::ostream* os = NULL;
    s.set_drat(os, false);

    EXPECT_THROW({
        s.set_num_threads(3);}
        , std::runtime_error);
}

TEST(error_throw, toomany_vars)
{
    SATSolver s;

    EXPECT_THROW({
        s.new_vars(1ULL << 28);}
        , CMSat::TooManyVarsError);
}

TEST(error_throw, toomany_vars2)
{
    SATSolver s;
    s.new_vars(1ULL << 27);

    EXPECT_THROW({
        s.new_vars(1ULL << 27);}
        , CMSat::TooManyVarsError);
}

TEST(error_throw, toomany_vars_single)
{
    SATSolver s;
    s.new_vars((1ULL << 28) -1);

    EXPECT_THROW({
        s.new_var();}
        , CMSat::TooManyVarsError);
}

TEST(no_error_throw, long_clause)
{
    SATSolver s;
    s.new_vars(1ULL << 20);

    vector<Lit> cl;
    for(size_t i = 0; i < 1ULL << 20; i++) {
        cl.push_back(Lit(i, false));
    }
    s.add_clause(cl);
    lbool ret = s.solve();
    EXPECT_EQ(ret, l_True);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
