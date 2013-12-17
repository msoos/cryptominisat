#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "solver.h"
using namespace CMSat;

BOOST_AUTO_TEST_SUITE( minimal_interface )

BOOST_AUTO_TEST_CASE(start)
{
    Solver s;
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(onelit)
{
    Solver s;
    s.new_external_var();
    s.addClauseOuter(vector<Lit>{Lit(0, false)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(twolit)
{
    Solver s;
    s.new_external_var();
    s.addClauseOuter(vector<Lit>{Lit(0, false)});
    s.addClauseOuter(vector<Lit>{Lit(0, true)});
    lbool ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_False);
}

BOOST_AUTO_TEST_CASE(multi_solve_unsat)
{
    Solver s;
    s.new_external_var();
    s.addClauseOuter(vector<Lit>{Lit(0, false)});
    s.addClauseOuter(vector<Lit>{Lit(0, true)});
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
