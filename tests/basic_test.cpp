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

#include <fstream>

#include "cryptominisat4/cryptominisat.h"
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
        lbool ret = s.solve();
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
        lbool ret = s.solve();
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
        lbool ret = s.solve();
        EXPECT_EQ( ret, l_False);
    }
    EXPECT_EQ( s.nVars(), 1);
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
        lbool ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ( s.okay(), true);
    }
    EXPECT_EQ( s.nVars(), 1);
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
        lbool ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ( s.okay(), true);
    }
    EXPECT_EQ( s.nVars(), 1);
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
        lbool ret = s.solve();
        EXPECT_EQ( ret, l_True);
        EXPECT_EQ(s.get_model()[0], l_True);
        EXPECT_EQ(s.get_model()[1], l_True);
    }
    EXPECT_EQ( s.nVars(), 2);
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
    EXPECT_EQ( s.nVars(), 2);
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
    EXPECT_EQ( s.nVars(), 2);
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
    EXPECT_EQ( s.nVars(), 2);
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
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 4);
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
    EXPECT_EQ( s.nVars(), 4);
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
    EXPECT_EQ( s.nVars(), 30);
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
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 3);

    s.add_xor_clause(vector<uint32_t>{0U}, false);
    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.get_model()[0], l_False);
    EXPECT_EQ( s.get_model()[1], l_False);
    EXPECT_EQ( s.get_model()[2], l_True);
    EXPECT_EQ( s.nVars(), 3);

    s.add_xor_clause(vector<uint32_t>{1U}, true);
    ret = s.solve();
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.nVars(), 3);
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
    EXPECT_EQ( s.nVars(), 3);
}

TEST(xor_interface, unit)
{
    SATSolver s;
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    EXPECT_EQ( ret, l_True);

    vector<Lit> units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 1);
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
    EXPECT_EQ( units.size(), 1);
    EXPECT_EQ( units[0], Lit(0, false));

    s.add_clause(vector<Lit>{Lit(1, true)});
    ret = s.solve();
    EXPECT_EQ( ret, l_True);

    units = s.get_zero_assigned_lits();
    EXPECT_EQ( units.size(), 2);
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
    EXPECT_EQ( units.size(), 2);
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
    EXPECT_EQ( pairs.size(), 1);
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
    EXPECT_EQ( pairs.size(), 2);
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
    EXPECT_EQ( pairs.size(), 1);

    s.add_xor_clause(vector<uint32_t>{1, 2}, false);
    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    pairs = s.get_all_binary_xors();
    EXPECT_EQ( pairs.size(), 2);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
