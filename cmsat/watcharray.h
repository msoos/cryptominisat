#ifndef __WATCHARRAY_H__
#define __WATCHARRAY_H__

#include <stdlib.h>
#include "watched.h"
#include <vector>

namespace CMSat {
using namespace CMSat;
using std::vector;

struct watch_array;

struct Elem
{
    Elem() :
        num(0)
        , offset(0)
    {}

    uint32_t num:8;
    uint32_t offset:24;
    uint32_t size = 0;
    uint32_t alloc = 0;

    void print_stat() const
    {
        cout
        << "elem."
        << " num: " << num
        << " offset:" << offset
        << " size" << size
        << " alloc" << alloc
        << endl;
    }
};

struct Mem
{
    uint32_t alloc = 0;
    Watched* base_ptr = NULL;
    uint32_t next_space_offset = 0;
};

struct OffsAndNum
{
    OffsAndNum(uint32_t _offset, uint32_t _num) :
        offset(_offset)
        , num(_num)
    {}

    uint32_t offset;
    uint32_t num;
};

struct watch_subarray
{
    vector<Elem>::iterator base_at;
    watch_array* base;
    explicit watch_subarray(vector<Elem>::iterator _base_at, watch_array* _base) :
        base_at(_base_at)
        , base(_base)
    {}

    Watched& operator[](const uint32_t at);
    void clear();
    uint32_t size() const;
    bool empty() const;
    Watched* begin();
    Watched* end();
    const Watched* begin() const;
    const Watched* end() const;
    void shrink(const uint32_t num);
    void shrink_(const uint32_t num);
    void push(const Watched& watched);

    typedef Watched* iterator;
    typedef const Watched* const_iterator;
};

struct watch_subarray_const
{
    vector<Elem>::const_iterator base_at;
    const watch_array* base;
    explicit watch_subarray_const(vector<Elem>::const_iterator _base_at, const watch_array* _base) :
        base_at(_base_at)
        , base(_base)
    {}

    watch_subarray_const(const watch_subarray& other) :
        base_at(other.base_at)
        , base(other.base)
    {}

    void print_stat() const;
    const Watched& operator[](const uint32_t at) const;
    uint32_t size() const;
    bool empty() const;
    const Watched* begin() const;
    const Watched* end() const;
    typedef const Watched* const_iterator;
};

struct watch_array
{
    vector<Elem> watches;
    vector<Mem> mems;

    watch_array()
    {
        //We need at least 1
        Mem new_mem;
        size_t elems = 1000;
        new_mem.base_ptr = (Watched*)malloc(elems*sizeof(Watched));
        new_mem.alloc = elems;
        mems.push_back(new_mem);
    }

    ~watch_array()
    {
        for(size_t i = 0; i < mems.size(); i++) {
            free(mems[i].base_ptr);
        }
    }

    uint32_t get_suitable_base(uint32_t elems)
    {
        //print_stat();

        size_t last_alloc = 1000;
        for(size_t i = 0; i < mems.size(); i++) {
            if (mems[i].next_space_offset + elems < mems[i].alloc) {
                return i;
            }
            last_alloc = mems[i].alloc;
        }
        assert(mems.size() < 255);

        Mem new_mem;
        new_mem.alloc = 2*last_alloc + 2*elems;
        new_mem.base_ptr = (Watched*)malloc(new_mem.alloc*sizeof(Watched));
        assert(new_mem.base_ptr != NULL);
        mems.push_back(new_mem);
        return mems.size()-1;
    }

    OffsAndNum get_space(uint32_t elems)
    {
        uint32_t num = get_suitable_base(elems);
        Mem& mem = mems[num];
        assert(mem.next_space_offset + elems < mem.alloc);

        uint32_t off_to_ret = mem.next_space_offset;
        mem.next_space_offset += elems;

        return OffsAndNum(off_to_ret, num);
    }

    void consolidate()
    {
        size_t needed = 0;
        for(size_t i = 0; i < watches.size(); i++) {
            if (watches[i].size)
                needed += watches[i].size*1.5 + 2;
        }

        Mem newmem;
        newmem.alloc = (double)needed*1.2;
        newmem.base_ptr = (Watched*)malloc(newmem.alloc*sizeof(Watched));
        for(size_t i = 0; i < watches.size(); i++) {
            //Never used
            if (watches[i].alloc == 0)
                continue;

            //Freeing this up
            if (watches[i].size == 0) {
                watches[i] = Elem();
                continue;
            }


            Elem& ws = watches[i];

            //Allow for some space to breathe
            ws.alloc = ws.size*1.5 + 2;

            Watched* orig_ptr = mems[ws.num].base_ptr + ws.offset;
            Watched* new_ptr = newmem.base_ptr + newmem.next_space_offset;
            memmove(new_ptr, orig_ptr, ws.size * sizeof(Watched));
            ws.num = 0;
            ws.offset = newmem.next_space_offset;
            newmem.next_space_offset += ws.alloc;
        }

        for(size_t i = 0; i < mems.size(); i++) {
            free(mems[i].base_ptr);
        }
        mems.clear();

        mems.push_back(newmem);
    }

    void print_stat() const
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
    }

    void delete_offset(uint32_t num, uint32_t offs)
    {
        //TODO
    }

    size_t mem_used_alloc() const
    {
        size_t total = 0;
        for(size_t i = 0; i < mems.size(); i++) {
            total += mems[i].alloc*sizeof(Watched);
        }
        return total;
    }

    size_t mem_used_array() const
    {
        size_t total = 0;
        total += watches.capacity() * sizeof(Elem);
        total += mems.capacity() * sizeof(Mem);
        return total;
    }

    watch_subarray operator[](size_t at)
    {
        assert(watches.size() > at);
        return watch_subarray(watches.begin() + at, this);
    }

    watch_subarray_const operator[](size_t at) const
    {
        assert(watches.size() > at);
        return watch_subarray_const(watches.begin() + at, this);
    }

    void resize(const size_t new_size)
    {
        watches.resize(new_size);
    }

    uint32_t size() const
    {
        return watches.size();
    }

    void shrink_to_fit()
    {
        watches.shrink_to_fit();
    }

    void prefetch(const size_t at) const
    {
        __builtin_prefetch(mems[watches[at].num].base_ptr + watches[at].offset);
    }

    struct iterator
    {
        vector<Elem>::iterator it;
        watch_array* base;
        explicit iterator(vector<Elem>::iterator _it, watch_array* _base) :
            it(_it)
            , base(_base)
        {}

        iterator operator++()
        {
            ++it;
            return *this;
        }

        watch_subarray operator*()
        {
            return watch_subarray(it, base);
        }

        bool operator==(const iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const iterator& it2) const
        {
            return it != it2.it;
        }

        friend size_t operator-(const iterator& lhs, const iterator& rhs);
    };

    struct const_iterator
    {
        vector<Elem>::const_iterator it;
        const watch_array* base;
        explicit const_iterator(vector<Elem>::const_iterator _it, const watch_array* _base) :
            it(_it)
            , base(_base)
        {}

        const_iterator(const iterator& other) :
            it(other.it)
            , base(other.base)
        {}

        const_iterator operator++()
        {
            ++it;
            return *this;
        }

        const watch_subarray_const operator*() const
        {
            return watch_subarray_const(it, base);
        }

        bool operator==(const const_iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const const_iterator& it2) const
        {
            return it != it2.it;
        }

        friend size_t operator-(const const_iterator& lhs, const const_iterator& rhs);
    };

    iterator begin()
    {
        return iterator(watches.begin(), this);
    }

    iterator end()
    {
        return iterator(watches.end(), this);
    }

    const_iterator begin() const
    {
        return const_iterator(watches.begin(), this);
    }

    const_iterator end() const
    {
        return const_iterator(watches.end(), this);
    }

    void fitToSize()
    {
        //TODO shirnk
    }
};

inline size_t operator-(const watch_array::iterator& lhs, const watch_array::iterator& rhs)
{
    return lhs.it-rhs.it;
}

inline size_t operator-(const watch_array::const_iterator& lhs, const watch_array::const_iterator& rhs)
{
    return lhs.it-rhs.it;
}

inline Watched& watch_subarray::operator[](const uint32_t at)
{
    return *(begin() + at);
}

inline void watch_subarray::clear()
{
    base_at->size = 0;
}

inline uint32_t watch_subarray::size() const
{
    return base_at->size;
}

inline bool watch_subarray::empty() const
{
    return size() == 0;
}

inline Watched* watch_subarray::begin()
{
    return base->mems[base_at->num].base_ptr + base_at->offset;
}

inline Watched* watch_subarray::end()
{
    return begin() + size();
}

inline const Watched* watch_subarray::begin() const
{
    return base->mems[base_at->num].base_ptr + base_at->offset;
}

inline const Watched* watch_subarray::end() const
{
    return begin() + size();
}

inline void watch_subarray::shrink(const uint32_t num)
{
    base_at->size -= num;
}

inline void watch_subarray::shrink_(const uint32_t num)
{
    shrink(num);
}

inline void watch_subarray::push(const Watched& watched)
{
    //Make space
    if (base_at->alloc <= base_at->size) {
        uint32_t new_alloc = base_at->alloc*2+2;
        OffsAndNum off_and_num = base->get_space(new_alloc);

        //Copy
        if (base_at->size > 0) {
            Watched* newptr = base->mems[off_and_num.num].base_ptr + off_and_num.offset;
            Watched* oldptr = begin();
            memmove(newptr, oldptr, size() * sizeof(Watched));
            base->delete_offset(base_at->num, base_at->offset);
        }

        //Update
        base_at->num = off_and_num.num;
        base_at->offset = off_and_num.offset;
        base_at->alloc = new_alloc;
    }

    //There is enough space
    assert(base_at->alloc > base_at->size);

    //Append to the end
    operator[](size()) = watched;
    base_at->size++;
}

inline const Watched& watch_subarray_const::operator[](const uint32_t at) const
{
    return *(begin() + at);
}
inline uint32_t watch_subarray_const::size() const
{
    return base_at->size;
}

inline bool watch_subarray_const::empty() const
{
    return size() == 0;
}
inline const Watched* watch_subarray_const::begin() const
{
    return base->mems[base_at->num].base_ptr + base_at->offset;
}

inline const Watched* watch_subarray_const::end() const
{
    return begin() + size();
}

inline void watch_subarray_const::print_stat() const
{
    base->print_stat();
    base_at->print_stat();
}

inline void swap(watch_subarray a, watch_subarray b)
{
    Elem tmp;
    tmp = *(a.base_at);
    *(a.base_at) = *(b.base_at);
    *(b.base_at) = tmp;
}


} //End of namespace

#endif //__WATCHARRAY_H__
