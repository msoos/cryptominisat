#define BOOST_TEST_MODULE basic_interface
#include <boost/test/unit_test.hpp>

#include "cryptominisat4/cryptominisat.h"

#include "src/heap.h"

BOOST_AUTO_TEST_SUITE( heap_minim )

struct Comp
{
    bool operator()(uint32_t a, uint32_t b) const
    {
        return a < b;
    }
};

BOOST_AUTO_TEST_CASE(simple)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    BOOST_CHECK_EQUAL( heap.heap_property(), true);
}

BOOST_AUTO_TEST_CASE(empty)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    BOOST_CHECK_EQUAL(heap.remove_min(), 1);
    BOOST_CHECK_EQUAL(heap.remove_min(), 2);
    BOOST_CHECK_EQUAL( heap.heap_property(), true);
}

BOOST_AUTO_TEST_CASE(empty2)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    BOOST_CHECK_EQUAL(heap.remove_min(), 1);
    BOOST_CHECK_EQUAL(heap.remove_min(), 2);
    BOOST_CHECK_EQUAL(heap.remove_min(), 3);
    BOOST_CHECK_EQUAL( heap.heap_property(), true);
}

BOOST_AUTO_TEST_CASE(empty_lots)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    for(size_t i = 0; i < 100; i++) {
        heap.insert(99-i);
        BOOST_CHECK_EQUAL( heap.heap_property(), true);
    }
    for(size_t i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(heap.remove_min(), i);
        BOOST_CHECK_EQUAL(heap.heap_property(), true);
    }
}

BOOST_AUTO_TEST_CASE(copy_heap)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    for(size_t i = 0; i < 100; i++) {
        heap.insert(99-i);
        BOOST_CHECK_EQUAL( heap.heap_property(), true);
    }
    Heap<Comp> mycopy(heap);
    for(size_t i = 0; i < 100; i++) {
        BOOST_CHECK_EQUAL(mycopy.remove_min(), i);
        BOOST_CHECK_EQUAL(mycopy.heap_property(), true);
    }
}

BOOST_AUTO_TEST_CASE(inserted_inside)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(10);
    heap.insert(20);
    BOOST_CHECK_EQUAL(heap.in_heap(10), true);
    BOOST_CHECK_EQUAL(heap.in_heap(20), true);
}

BOOST_AUTO_TEST_SUITE_END()
