#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "cryptominisat3/cryptominisat.h"
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
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( xor_interface )
bool is_critical(const std::range_error&) { return true; }

BOOST_AUTO_TEST_CASE(xorcheck_no_inverted_lits)
{
    SATSolver s;
    s.new_var();
    BOOST_REQUIRE_EXCEPTION(s.add_xor_clause(vector<Lit>{Lit(0, true)}, false);, std::range_error, is_critical);
}

BOOST_AUTO_TEST_CASE(xor_check_no_inverted_lits2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    vector<Lit> lits;
    lits.push_back(Lit(0, false));
    lits.push_back(Lit(1, true));
    BOOST_REQUIRE_EXCEPTION(s.add_xor_clause(lits, true);, std::range_error, is_critical);
}

BOOST_AUTO_TEST_CASE(xor_check_sat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_False);
    }
}

BOOST_AUTO_TEST_CASE(xor_check_unsat_solution)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
    }
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values)
{
    SATSolver s;
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
    }
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(1, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(size_t i = 0;i < 10; i++) {
        lbool ret = s.solve();
        BOOST_CHECK_EQUAL( ret, l_True);
        BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
        BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    }
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values3)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(0, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values4)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(0, false)}, false);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}


BOOST_AUTO_TEST_CASE(xor_check_solution_values5)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false)}, true);
    vector<Lit> assump = {Lit(0, false)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values6)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false)}, false);
    vector<Lit> assump = {Lit(0, true)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
}

BOOST_AUTO_TEST_CASE(xor_check_solution_values7)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false), Lit(2, false)}, false);
    vector<Lit> assump = {Lit(0, false), Lit(1, false)};
    lbool ret = s.solve(&assump);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
}

BOOST_AUTO_TEST_CASE(xor_3_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false), Lit(2, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(1, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_True);
}

BOOST_AUTO_TEST_CASE(xor_3_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false), Lit(2, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(1, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
}

BOOST_AUTO_TEST_CASE(xor_4_long)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false), Lit(2, false), Lit(3, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(1, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(2, false)}, false);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[3], l_False);
}

BOOST_AUTO_TEST_CASE(xor_4_long2)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.new_var();
    s.new_var();
    s.add_xor_clause(vector<Lit>{Lit(0, false), Lit(1, false), Lit(2, false), Lit(3, false)}, true);
    s.add_xor_clause(vector<Lit>{Lit(0, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(1, false)}, false);
    s.add_xor_clause(vector<Lit>{Lit(2, false)}, true);
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL(s.get_model()[0], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[1], l_False);
    BOOST_CHECK_EQUAL(s.get_model()[2], l_True);
    BOOST_CHECK_EQUAL(s.get_model()[3], l_False);
}

BOOST_AUTO_TEST_CASE(xor_very_long)
{
    SATSolver s;
    vector<Lit> lits;
    for(unsigned i = 0; i < 30; i++) {
        s.new_var();
        lits.push_back(Lit(i, false));
    }
    s.add_xor_clause(lits, false);
    for(unsigned i = 0; i < 29; i++) {
        s.add_xor_clause(vector<Lit>{Lit(i, false)}, false);
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
    SATSolver s;
    vector<Lit> lits;
    for(unsigned i = 0; i < 30; i++) {
        s.new_var();
        lits.push_back(Lit(i, false));
    }
    s.add_xor_clause(lits, true);
    for(unsigned i = 0; i < 29; i++) {
        s.add_xor_clause(vector<Lit>{Lit(i, false)}, false);
    }
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    for(unsigned i = 0; i < 29; i++) {
        BOOST_CHECK_EQUAL(s.get_model()[i], l_False);
    }
    BOOST_CHECK_EQUAL(s.get_model()[29], l_True);
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


