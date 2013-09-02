#define BOOST_TEST_MODULE assumptions
#include <boost/test/unit_test.hpp>

#include "solver.h"
using namespace CMSat;

BOOST_AUTO_TEST_SUITE( assumptions_interface )

BOOST_AUTO_TEST_CASE(empty)
{
    Solver s;
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(single_true)
{
    Solver s;
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, false));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
}

BOOST_AUTO_TEST_CASE(single_false)
{
    Solver s;
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.conflict.size(), 1);
    BOOST_CHECK_EQUAL( s.conflict[0], Lit(0, false));
}

BOOST_AUTO_TEST_CASE(binclause_true)
{
    Solver s;
    s.newVar();
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True );
    BOOST_CHECK_EQUAL( s.model[0], l_False );
    BOOST_CHECK_EQUAL( s.model[1], l_True );
}

BOOST_AUTO_TEST_CASE(binclause_false)
{
    Solver s;
    s.newVar();
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.conflict.size(), 2);
    vector<Lit> tmp = s.conflict;
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL( tmp[0], Lit(0, false) );
    BOOST_CHECK_EQUAL( tmp[1], Lit(1, false) );
}

BOOST_AUTO_TEST_CASE(replace_true)
{
    Solver s;
    s.newVar();
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false), Lit(1, true)});
    s.addClause(vector<Lit>{Lit(0, true), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.model[0], l_False );
    BOOST_CHECK_EQUAL( s.model[1], l_False );
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(replace_false, 2)
BOOST_AUTO_TEST_CASE(replace_false)
{
    Solver s;
    s.newVar();
    s.newVar();
    s.addClause(vector<Lit>{Lit(0, false), Lit(1, true)});
    s.addClause(vector<Lit>{Lit(0, true), Lit(1, false)});

    vector<Lit> assumps;
    assumps.push_back(Lit(0, false));
    assumps.push_back(Lit(1, true));

    lbool ret = s.solve(&assumps);
    BOOST_CHECK( ret == l_False);

    BOOST_CHECK( s.conflict.size() == 2);
    vector<Lit> tmp = s.conflict;
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK( tmp[0] == Lit(0, true) );
    BOOST_CHECK( tmp[1] == Lit(1, false) );
}


BOOST_AUTO_TEST_SUITE_END()