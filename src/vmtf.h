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
using std::vector;

namespace CMSat {

struct Link {
    // variable indices
    uint32_t prev = std::numeric_limits<uint32_t>::max();
    uint32_t next = std::numeric_limits<uint32_t>::max();
};

typedef vector<Link> Links;

struct Queue {

  // We use integers instead of variable pointers.  This is more compact and
  // also avoids issues due to moving the variable table during 'resize'.

  uint32_t first, last;    // anchors (head/tail) for doubly linked list
  uint32_t unassigned;     // all variables after this one are assigned
  uint64_t vmtf_bumped;     // see vmtf_update_queue_unassigned

  Queue () :
      first (std::numeric_limits<uint32_t>::max()),
      last (std::numeric_limits<uint32_t>::max()),
      unassigned (std::numeric_limits<uint32_t>::max()),
      vmtf_bumped (std::numeric_limits<uint64_t>::max())
  {}

  // We explicitly provide the mapping of integer indices to vmtf_links to the
  // following two (inlined) functions.  This avoids including
  // 'internal.hpp' and breaks a cyclic dependency, so we can keep their
  // code here in this header file.  Otherwise they are just ordinary doubly
  // linked list 'dequeue' and 'enqueue' operations.

  inline void dequeue (Links & vmtf_links, uint32_t idx) {
    Link & l = vmtf_links[idx];

    if (l.prev != std::numeric_limits<uint32_t>::max()) {
        vmtf_links[l.prev].next = l.next;
    } else {
        first = l.next;
    }

    if (l.next != std::numeric_limits<uint32_t>::max()) {
        vmtf_links[l.next].prev = l.prev;
    } else {
        last = l.prev;
    }
  }

  inline void enqueue (Links & vmtf_links, uint32_t idx) {
    Link & l = vmtf_links[idx];
    l.prev = last;
    if (l.prev != std::numeric_limits<uint32_t>::max()) {
        vmtf_links[last].next = idx;
    } else {
        first = idx;
    }
    last = idx;
    l.next = std::numeric_limits<uint32_t>::max();
  }
};

} //namespace

#endif //VMTF_H
