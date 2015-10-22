#include "gtest/gtest.h"

#include "cryptominisat4/cryptominisat.h"
#include <vector>
using std::vector;
using namespace CMSat;

TEST(assumptions_interface, empty)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);
}

TEST(assumptions_interface, single_true)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, false));
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);
}

TEST(assumptions_interface, single_false)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.get_conflict().size(), 1);
    EXPECT_EQ( s.get_conflict()[0], Lit(0, false));
}


TEST(assumptions_interface, single_false_then_true)
{
    SATSolver s;
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), true);

    ret = s.solve();
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.okay(), true);

}

TEST(assumptions_interface, binclause_true)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_True );
    EXPECT_EQ( s.get_model()[0], l_False );
    EXPECT_EQ( s.get_model()[1], l_True );
}

TEST(assumptions_interface, binclause_false)
{
    SATSolver s;
    s.new_var();
    s.new_var();
    s.add_clause(vector<Lit>{Lit(0, false), Lit(1, false)});
    vector<Lit> assumps;
    assumps.push_back(Lit(0, true));
    assumps.push_back(Lit(1, true));
    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0], Lit(0, false) );
    EXPECT_EQ( tmp[1], Lit(1, false) );
}

TEST(assumptions_interface, replace_true)
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
    EXPECT_EQ( ret, l_True);
    EXPECT_EQ( s.get_model()[0], l_False );
    EXPECT_EQ( s.get_model()[1], l_False );
}

TEST(assumptions_interface, replace_false)
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
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), true);

    EXPECT_EQ( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0], Lit(0, true) );
    EXPECT_EQ( tmp[1], Lit(1, false) );
}

TEST(assumptions_interface, set_var_by_prop)
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
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), true);

    EXPECT_EQ( s.get_conflict().size(), 1);

    vector<Lit> tmp = s.get_conflict();
    EXPECT_EQ( tmp[0], Lit(1, false) );
}

TEST(assumptions_interface, only_assump)
{
    SATSolver s;
    s.new_var();
    s.new_var();

    vector<Lit> assumps;
    assumps.push_back(Lit(1, true));
    assumps.push_back(Lit(1, false));

    lbool ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_False);
    EXPECT_EQ( s.okay(), true);
    EXPECT_EQ( s.get_conflict().size(), 2);

    vector<Lit> tmp = s.get_conflict();
    std::sort(tmp.begin(), tmp.end());
    EXPECT_EQ( tmp[0] , Lit(1, false) );
    EXPECT_EQ( tmp[1], Lit(1, true) );

    ret = s.solve(NULL);
    EXPECT_EQ( ret, l_True );

    ret = s.solve(&assumps);
    EXPECT_EQ( ret, l_False);
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
