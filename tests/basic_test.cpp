#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "cryptominisat.h"
using namespace CMSat;
#include <vector>
using std::vector;

BOOST_AUTO_TEST_SUITE( minimal_interface )

BOOST_AUTO_TEST_CASE(start)
{
    MainSolver s;
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(onelit)
{
    MainSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(twolit)
{
    MainSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    s.add_clause(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
}

BOOST_AUTO_TEST_CASE(multi_solve_unsat)
{
    MainSolver s;
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

BOOST_AUTO_TEST_SUITE_END()
