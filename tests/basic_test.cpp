#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/cryptominisat.h"
using namespace CMSat;
#include <vector>
using std::vector;

BOOST_AUTO_TEST_SUITE( normal_interface )

BOOST_AUTO_TEST_CASE(start)
{
    SATSolver s;
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(onelit)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(twolit)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
}

BOOST_AUTO_TEST_CASE(multi_solve_unsat)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_False);
    }
}

BOOST_AUTO_TEST_CASE(multi_solve_unsat_multi_thread)
{
    SATSolver s;
    s.set_num_threads(2);
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_False);
    }
}

BOOST_AUTO_TEST_CASE(solve_multi_thread)
{
    SATSolver s;
    s.set_num_threads(2);
    s.new_vars(2);
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);

    s.add_clause(vector<Lit>{Lit(0, true)});
    ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( xor_interface )
bool is_critical(const std::range_error&) { return true; }

BOOST_AUTO_TEST_CASE(xor_check_sat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<unsigned>{0U}, false);
    s.add_xor_clause(vector<unsigned>{0U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_False);
    }
    BOOST_CHECK_EQUAL( s.nVars(), 1);
}

BOOST_AUTO_TEST_CASE(xor_check_unsat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<Var>{0U}, true);
    s.add_xor_clause(vector<Var>{0U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
    }
    BOOST_CHECK_EQUAL( s.nVars(), 1);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<Var>{0U}, true);
    s.add_xor_clause(vector<Var>{0U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
    }
    BOOST_CHECK_EQUAL( s.nVars(), 1);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U}, true);
    s.add_xor_clause(vector<Var>{1U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
        BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
        BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    }
    BOOST_CHECK_EQUAL( s.nVars(), 2);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values3)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 0U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values4)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 0U}, false);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.nVars(), 2);
}


BOOST_AUTO_TEST_CASE(xor_check_solution_values5)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U}, true);
    vector<Lit> assump = {Lit(0, false)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 2);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values6)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U}, false);
    vector<Lit> assump = {Lit(0, true)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 2);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values7)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, false);
    vector<Lit> assump = {Lit(0, false), Lit(1, false)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_3_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, true);
    s.add_xor_clause(vector<Var>{0}, true);
    s.add_xor_clause(vector<Var>{1}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_True);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_3_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<Var>{0U}, true);
    s.add_xor_clause(vector<Var>{1U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_4_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U, 2U, 3U}, false);
    s.add_xor_clause(vector<Var>{0U}, false);
    s.add_xor_clause(vector<Var>{1U}, false);
    s.add_xor_clause(vector<Var>{2U}, false);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[3], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 4);
}

BOOST_AUTO_TEST_CASE(xor_4_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Var>{0U, 1U, 2U, 3U}, true);
    s.add_xor_clause(vector<Var>{0U}, false);
    s.add_xor_clause(vector<Var>{1U}, false);
    s.add_xor_clause(vector<Var>{2U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[3], l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 4);
}

BOOST_AUTO_TEST_CASE(xor_very_long)
{
    SATSolver s;
    vector<Var> vars;
    for(unsigned i = 0; i < 30; i++) {
        s.new_var();
        vars.push_back(i);
    }
    s.add_xor_clause(vars, false);
    for(unsigned i = 0; i < 29; i++) {
        s.add_xor_clause(vector<Var>{i}, false);
    }
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(unsigned i = 0; i < 30; i++) {
        BOOST_CHECK_EQUAL(s.get_model()[i], l_False);
    }
    BOOST_CHECK_EQUAL( s.nVars(), 30);
}

BOOST_AUTO_TEST_CASE(xor_very_long2)
{
    for(size_t num = 3; num < 30; num++) {
        SATSolver s;
        vector<Var> vars;
        for(unsigned i = 0; i < num; i++) {
            s.new_var();
            vars.push_back(i);
        }
        s.add_xor_clause(vars, true);
        for(unsigned i = 0; i < num-1; i++) {
            s.add_xor_clause(vector<Var>{i}, false);
        }
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
        for(unsigned i = 0; i < num-1; i++) {
            BOOST_CHECK_EQUAL(s.get_model()[i], l_False);
        }
        BOOST_CHECK_EQUAL(s.get_model()[num-1], l_True);
        BOOST_CHECK_EQUAL( s.nVars(), num);
    }
}

BOOST_AUTO_TEST_CASE(xor_check_unsat)
{
    SATSolver s;
    s.new_vars(3);
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_check_unsat_multi_thread)
{
    SATSolver s;
    s.set_num_threads(3);
    s.new_vars(3);
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, false);
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_check_unsat_multi_solve_multi_thread)
{
    SATSolver s;
    s.set_num_threads(3);
    s.new_vars(3);
    s.add_xor_clause(vector<Var>{0U, 1U}, false);
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.nVars(), 3);

    s.add_xor_clause(vector<Var>{0U}, false);
    ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL( s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL( s.get_model()[2], l_True);
    BOOST_CHECK_EQUAL( s.nVars(), 3);

    s.add_xor_clause(vector<Var>{1U}, true);
    ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_CASE(xor_norm_mix_unsat_multi_thread)
{
    SATSolver s;
    //s.set_num_threads(3);
    s.new_vars(3);
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_xor_clause(vector<Var>{0U, 1U, 2U}, false);
    s.add_clause(vector<Lit>{Lit(1, false)});
    s.add_clause(vector<Lit>{Lit(2, false)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.nVars(), 3);
}

BOOST_AUTO_TEST_SUITE_END()

/*struct F {
    F() : i( 1 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
    ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }

    int i;
};

BOOST_FIXTURE_TEST_CASE( test_fix, F)
{
    BOOST_CHECK( i == 1 );
}

BOOST_FIXTURE_TEST_CASE( test_fix2, F)
{
    BOOST_CHECK_EQUAL( i, 1 );
}*/


