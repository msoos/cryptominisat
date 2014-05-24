#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/cryptominisat.h"

#include <vector>
#include "cryptominisat4/heap.h"
using std::vector;

BOOST_AUTO_TEST_SUITE( heap_minim )

struct Comp
{
    bool operator()(uint32_t a, uint32_t b) const
    {
        return a < b;
    };
};

BOOST_AUTO_TEST_CASE(simple)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    BOOST_CHECK_EQUAL( heap.heapProperty(), true);
}

BOOST_AUTO_TEST_CASE(empty)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    BOOST_CHECK_EQUAL(heap.removeMin(), 1);
    BOOST_CHECK_EQUAL(heap.removeMin(), 2);
    BOOST_CHECK_EQUAL( heap.heapProperty(), true);
}

BOOST_AUTO_TEST_CASE(empty2)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    BOOST_CHECK_EQUAL(heap.removeMin(), 1);
    BOOST_CHECK_EQUAL(heap.removeMin(), 2);
    BOOST_CHECK_EQUAL(heap.removeMin(), 3);
    BOOST_CHECK_EQUAL( heap.heapProperty(), true);
}

BOOST_AUTO_TEST_CASE(empty_lots)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    for(size_t i = 0; i < 100; i++) {
        heap.insert(99-i);
        BOOST_CHECK_EQUAL( heap.heapProperty(), true);
    }
    for(size_t i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(heap.removeMin(), i);
    }
    BOOST_CHECK_EQUAL( heap.heapProperty(), true);
}

BOOST_AUTO_TEST_SUITE_END()