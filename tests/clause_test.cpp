#define BOOST_TEST_MODULE assumptions
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/clause.h"
#include <sstream>
#include <stdlib.h>

using namespace CMSat;

struct F {
    F() {
    }

    ~F() {
    }

    Clause* allocate_space_for(size_t n)
    {
        void* tmp = malloc(sizeof(Clause) + n*sizeof(Lit));
        std::vector<Lit> lits;
        for(size_t i = 0; i < n ; i++) {
            lits.push_back(Lit(i, false));
        }
        Clause* c_ptr = new(tmp) Clause(lits, 0);
        return c_ptr;
    }
};

BOOST_FIXTURE_TEST_SUITE( clause_test, F )

BOOST_AUTO_TEST_CASE(convert_to_string)
{
    Clause& cl = *allocate_space_for(3);
    cl[0] = Lit(0, false);
    cl[1] = Lit(1, false);
    cl[2] = Lit(2, false);

    std::stringstream ss;
    ss << cl;
    BOOST_CHECK_EQUAL( ss.str(), "1 2 3");
}

BOOST_AUTO_TEST_CASE(convert_to_string2)
{
    Clause& cl = *allocate_space_for(3);
    cl[0] = Lit(0, false);
    cl[1] = Lit(1, true);
    cl[2] = Lit(2, false);

    std::stringstream ss;
    ss << cl;
    BOOST_CHECK_EQUAL( ss.str(), "1 -2 3");
}

#ifdef STATS_NEEDED
BOOST_AUTO_TEST_CASE(weighted_prop_and_confl)
{
    Clause& cl = *allocate_space_for(3);
    cl.stats.propagations_made = 10;
    cl.stats.conflicts_made = 7;

    BOOST_CHECK_EQUAL( cl.stats.weighted_prop_and_confl(1.0, 1.0), 17);
    BOOST_CHECK_EQUAL( cl.stats.weighted_prop_and_confl(1.0, 2.0), 24);
}

BOOST_AUTO_TEST_CASE(clear)
{
    Clause& cl = *allocate_space_for(3);
    cl.stats.propagations_made = 10;
    cl.stats.conflicts_made = 7;
    cl.stats.clear(0.0);

    BOOST_CHECK_EQUAL( cl.stats.propagations_made, 0);
    BOOST_CHECK_EQUAL( cl.stats.conflicts_made, 0);
}
#endif

BOOST_AUTO_TEST_SUITE_END()