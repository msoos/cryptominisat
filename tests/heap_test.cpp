#include "gtest/gtest.h"

#include "cryptominisat4/cryptominisat.h"

#include "src/heap.h"

struct Comp
{
    bool operator()(uint32_t a, uint32_t b) const
    {
        return a < b;
    }
};

TEST(heap_minim, simple)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    EXPECT_EQ( heap.heap_property(), true);
}

TEST(heap_minim, empty)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    EXPECT_EQ(heap.remove_min(), 1);
    EXPECT_EQ(heap.remove_min(), 2);
    EXPECT_EQ( heap.heap_property(), true);
}

TEST(heap_minim, empty2)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(1);
    heap.insert(2);
    heap.insert(3);
    EXPECT_EQ(heap.remove_min(), 1);
    EXPECT_EQ(heap.remove_min(), 2);
    EXPECT_EQ(heap.remove_min(), 3);
    EXPECT_EQ( heap.heap_property(), true);
}

TEST(heap_minim, empty_lots)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    for(size_t i = 0; i < 100; i++) {
        heap.insert(99-i);
        EXPECT_EQ( heap.heap_property(), true);
    }
    for(size_t i = 0; i < 100; i++) {
        EXPECT_EQ(heap.remove_min(), i);
        EXPECT_EQ(heap.heap_property(), true);
    }
}

TEST(heap_minim, copy_heap)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    for(size_t i = 0; i < 100; i++) {
        heap.insert(99-i);
        EXPECT_EQ( heap.heap_property(), true);
    }
    Heap<Comp> mycopy(heap);
    for(size_t i = 0; i < 100; i++) {
        EXPECT_EQ(mycopy.remove_min(), i);
        EXPECT_EQ(mycopy.heap_property(), true);
    }
}

TEST(heap_minim, inserted_inside)
{
    Comp cmp;
    Heap<Comp> heap(cmp);
    heap.insert(10);
    heap.insert(20);
    EXPECT_EQ(heap.in_heap(10), true);
    EXPECT_EQ(heap.in_heap(20), true);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
