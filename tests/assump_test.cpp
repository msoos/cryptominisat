#define BOOST_TEST_MODULE assumptions
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/cryptominisat.h"
#include <vector>
using std::vector;
using namespace CMSat;

BOOST_AUTO_TEST_SUITE( assumptions_interface )

BOOST_AUTO_TEST_CASE(empty)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.okay(), true);
}

BOOST_AUTO_TEST_CASE(single_true)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, false));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.okay(), true);
}

BOOST_AUTO_TEST_CASE(single_false)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.get_conflict().size(), 1);
    BOOST_CHECK_EQUAL( s.get_conflict()[0], Lit(0, false));
}


BOOST_AUTO_TEST_CASE(single_false_then_true)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.okay(), true);

    ret = s.solve();
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.okay(), true);

}

BOOST_AUTO_TEST_CASE(binclause_true)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True );
    BOOST_CHECK_EQUAL( s.get_model()[0], l_False );
    BOOST_CHECK_EQUAL( s.get_model()[1], l_True );
}

BOOST_AUTO_TEST_CASE(binclause_false)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL( tmp[0], Lit(0, false) );
    BOOST_CHECK_EQUAL( tmp[1], Lit(1, false) );
}

BOOST_AUTO_TEST_CASE(replace_true)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, true)});
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_True);
    BOOST_CHECK_EQUAL( s.get_model()[0], l_False );
    BOOST_CHECK_EQUAL( s.get_model()[1], l_False );
}

BOOST_AUTO_TEST_CASE(replace_false)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, true)}); //a V -b
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, false)}); //-a V b
    //a == b

    vector<Lit> assumps;
    assumps.push_back(Lit(0, false));
    assumps.push_back(Lit(1, true));
    //a = 1, b = 0

    lbool ret = s.solve(&assumps);
    BOOST_CHECK( ret == l_False);
    BOOST_CHECK_EQUAL( s.okay(), true);

    BOOST_CHECK_EQUAL( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK( tmp[0] == Lit(0, true) );
    BOOST_CHECK( tmp[1] == Lit(1, false) );
}

BOOST_AUTO_TEST_CASE(set_var_by_prop)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)}); //a = 1
    s.add_clause(vector<Lit>{Lit(0, true), Lit(1, false)}); //-a V b
    //-> b = 1

    vector<Lit> assumps;
    assumps.push_back(Lit(1, true));
    //b = 0

    lbool ret = s.solve(&assumps);
    BOOST_CHECK( ret == l_False);
    BOOST_CHECK_EQUAL( s.okay(), true);

    BOOST_CHECK_EQUAL( s.get_conflict().size(), 1);

    vector<Lit> tmp = s.get_conflict();
    BOOST_CHECK_EQUAL( tmp[0], Lit(1, false) );
}

BOOST_AUTO_TEST_CASE(only_assump)
{
    SATSolver s;
    s.new_var();
    s.new_var();

    vector<Lit> assumps;
    assumps.push_back(Lit(1, true));
    assumps.push_back(Lit(1, false));

    lbool ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
    BOOST_CHECK_EQUAL( s.okay(), true);
    BOOST_CHECK_EQUAL( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    BOOST_CHECK_EQUAL( tmp[0] , Lit(1, false) );
    BOOST_CHECK_EQUAL( tmp[1], Lit(1, true) );

    ret = s.solve(NULL);
    BOOST_CHECK_EQUAL( ret, l_True );

    ret = s.solve(&assumps);
    BOOST_CHECK_EQUAL( ret, l_False);
}


BOOST_AUTO_TEST_SUITE_END()