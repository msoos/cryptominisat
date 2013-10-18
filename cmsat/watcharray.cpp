#include "watcharray.h"
#include <algorithm>
using namespace CMSat;

void watch_array::consolidate()
{
    size_t total_needed = total_needed_during_consolidate();

    vector<Mem> newmems;
    size_t at_watches = 0;
    //size_t last_needed = 0;
    while(total_needed > 0) {
        Mem newmem;
        size_t needed = std::min<size_t>(total_needed, WATCH_MAX_SIZE_ONE_ALLOC);
        assert(needed > 0);

        //If last one was larger than this, let's take the last one
        //needed = std::max<size_t>(needed, last_needed/2);
        //last_needed = needed;
        total_needed -= needed;

        newmem.alloc = needed;
        newmem.base_ptr = (Watched*)malloc(newmem.alloc*sizeof(Watched));
        for(; at_watches < watches.size(); at_watches++) {
            //Not used
            if (watches[at_watches].size == 0) {
                watches[at_watches] = Elem();
                continue;
            }

            Elem& ws = watches[at_watches];

            //Allow for some space to breathe
            size_t toalloc = extra_space_during_consolidate(ws.size);

            //Does not fit into this 'newmem'
            if (newmem.next_space_offset + toalloc > newmem.alloc) {
                break;
            }

            ws.alloc = toalloc;
            Watched* orig_ptr = mems[ws.num].base_ptr + ws.offset;
            Watched* new_ptr = newmem.base_ptr + newmem.next_space_offset;
            memmove(new_ptr, orig_ptr, ws.size * sizeof(Watched));
            ws.num = newmems.size();
            ws.offset = newmem.next_space_offset;
            //ws.accessed = 0;
            newmem.next_space_offset += ws.alloc;
        }
        newmems.push_back(newmem);
    }
    assert(at_watches == watches.size());
    assert(total_needed == 0);

    for(size_t i = 0; i < mems.size(); i++) {
        free(mems[i].base_ptr);
    }

    mems = newmems;
    for(auto& mem: free_mem) {
        mem.clear();
    }
    free_mem_used = 0;
    free_mem_not_used = 0;
}

void watch_array::print_stat(bool detailed) const
{
    cout
    << " watches.size(): " << watches.size()
    << " mems.size(): " << mems.size()
    << endl;
    for(size_t i = 0; i < mems.size(); i++) {
        const Mem& mem = mems[i];
        cout
        << " -- mem " << i
        << " alloc: " << mem.alloc
        << " next_space_offset: " << mem.next_space_offset
        << " base_ptr: " << mem.base_ptr
        << endl;
    }

    if (detailed) {
        cout << "free stats:" << endl;
        for(size_t i = 0; i < free_mem.size(); i++)
        {
            cout << "->free_mem[" << i << "]: " << free_mem[i].size() << endl;
        }
    }

    /*for(size_t i = 0; i < watches.size(); i++) {
        cout << "ws[" << i << "] accessed: " << watches[i].accessed << " size: " << watches[i].size << endl;
    }*/

    cout
    << "free mem used:" << free_mem_used
    << " free mem not used: " << free_mem_not_used
    << " perc: "
    << std::fixed << std::setprecision(2) << (double)free_mem_used/(double)(free_mem_not_used+free_mem_used)*100.0
    << "%"
    << endl;
}