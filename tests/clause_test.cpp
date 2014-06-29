#define BOOST_TEST_MODULE assumptions
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/clause.h"
#include <sstream>
#include <stdlib.h>

using namespace CMSat;

struct F {
    F() : c_ptr(NULL) {
    }

    ~F() {
        delete c_ptr;
    }

    void allocate_space_for_two()
    {
        void* tmp = malloc(sizeof(Clause) + 2*sizeof(Lit));
        std::vector<Lit> lits;
        lits.push_back(Lit(0, false));
        lits.push_back(Lit(1, false));
        c_ptr = new(tmp) Clause(lits, 0);
    }

    Clause* c_ptr;
};

BOOST_FIXTURE_TEST_SUITE( clause_test, F )

BOOST_AUTO_TEST_CASE(convert_to_string)
{
    allocate_space_for_two();
    (*c_ptr)[0] = Lit(0, false);
    (*c_ptr)[1] = Lit(1, false);

    std::stringstream ss;
    ss << *c_ptr;
    BOOST_CHECK_EQUAL( ss.str(), "1 2");
}

BOOST_AUTO_TEST_CASE(convert_to_string2)
{
    allocate_space_for_two();
    (*c_ptr)[0] = Lit(0, false);
    (*c_ptr)[1] = Lit(1, true);

    std::stringstream ss;
    ss << *c_ptr;
    BOOST_CHECK_EQUAL( ss.str(), "1 -2");
}

BOOST_AUTO_TEST_CASE(numPropAndConfl)
{
    allocate_space_for_two();
    c_ptr->stats.propagations_made = 10;
    c_ptr->stats.conflicts_made = 7;

    BOOST_CHECK_EQUAL( c_ptr->stats.numPropAndConfl(1), 17);
    BOOST_CHECK_EQUAL( c_ptr->stats.numPropAndConfl(2), 24);
}

BOOST_AUTO_TEST_CASE(clear)
{
    allocate_space_for_two();
    c_ptr->stats.propagations_made = 10;
    c_ptr->stats.conflicts_made = 7;
    c_ptr->stats.clear(0.0);

    BOOST_CHECK_EQUAL( c_ptr->stats.propagations_made, 0);
    BOOST_CHECK_EQUAL( c_ptr->stats.conflicts_made, 0);
}

BOOST_AUTO_TEST_SUITE_END()