/******************************************
Copyright (c) 2019, Armin Biere

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#ifndef VMTF_H

#include <vector>
#include <cstdint>
#include "constants.h"
using std::vector;

namespace CMSat {

// variable indices
struct Link {
    uint32_t prev = numeric_limits<uint32_t>::max(); ///< VARIABLE NUMBER
    uint32_t next = numeric_limits<uint32_t>::max(); ///< VARIABLE NUMBER
};

struct Queue {

    // We use integers instead of variable pointers.  This is more compact and
    // also avoids issues due to moving the variable table during 'resize'.

    uint32_t first;  ///< VARIABLE NUMBER. anchor (head/tail) for doubly linked list.
    uint32_t last;   ///< VARIABLE NUMBER. anchor (head/tail) for doubly linked list.
    uint32_t unassigned;     ///< VARIABLE NUMBER. All variables after this one are assigned
    uint64_t vmtf_bumped;     ///< Last unassigned variable's btab value

    Queue () :
        first (numeric_limits<uint32_t>::max()),
        last (numeric_limits<uint32_t>::max()),
        unassigned (numeric_limits<uint32_t>::max()),
        vmtf_bumped (0)
    {}

    // We explicitly provide the mapping of integer indices to vmtf_links to the
    // following two (inlined) functions.  They are just ordinary doubly
    // linked list 'dequeue' and 'enqueue' operations.

    // Removes from the list
    void dequeue (vector<Link>& vmtf_links, const uint32_t var) {
        auto& l = vmtf_links[var];

        if (l.prev != numeric_limits<uint32_t>::max()) {
            // Not the first one in the list
            vmtf_links[l.prev].next = l.next;
        } else {
            first = l.next;
        }

        if (l.next != numeric_limits<uint32_t>::max()) {
            // No the last one in the list
            vmtf_links[l.next].prev = l.prev;
        } else {
            last = l.prev;
        }
    }

    // Puts varible at the head of the list
    void enqueue (vector<Link>& vmtf_links, const uint32_t var) {
        auto& l = vmtf_links[var];
        l.prev = last;
        if (l.prev != numeric_limits<uint32_t>::max()) {
            // Not the first one in the list
            vmtf_links[last].next = var;
        } else {
            first = var;
        }
        last = var;
        l.next = numeric_limits<uint32_t>::max();
    }
};

} //namespace

#endif //VMTF_H
