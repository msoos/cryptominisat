#ifndef __WATCHARRAY_H__
#define __WATCHARRAY_H__

#include "watched.h"
#include <vector>

namespace CMSat {
using namespace CMSat;
using std::vector;

struct watch_array;

struct Elem
{
    uint32_t offset = 0;
    uint32_t size = 0;
    uint32_t alloc = 0;
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

    const Watched& operator[](const uint32_t at) const;
    uint32_t size() const;
    bool empty() const;
    const Watched* begin() const;
    const Watched* end() const;
    typedef const Watched* const_iterator;
};

struct watch_array
{
    uint32_t alloc = 0;
    Watched* base_ptr = NULL;
    uint32_t next_space_offset = 0;
    vector<Elem> watches;

    ~watch_array()
    {
        delete base_ptr;
    }

    uint32_t get_space(uint32_t num)
    {
        //cout << "geting: " << num << endl;
        //print_stat();

        if (next_space_offset + num >= alloc) {
            uint32_t toalloc = alloc*2 + num*2 + 100;
            base_ptr = (Watched*)realloc(base_ptr, toalloc*sizeof(Watched));
            assert(base_ptr != NULL);
            alloc = toalloc;
        }
        assert(next_space_offset + num < alloc);

        uint32_t toret = next_space_offset;
        next_space_offset += num;

        //cout << "got it, returning offset:" << toret << endl;
        //print_stat();

        return toret;
    }

    void print_stat()
    {
        cout
        << "alloc: " << alloc
        << " next_space_offset: " << next_space_offset
        << " watches.size(): " << watches.size()
        << " base_ptr: " << base_ptr
        << endl;
    }

    void delete_offset(uint32_t offs)
    {
        //TODO
    }

    size_t memUsed() const
    {
        return alloc*sizeof(Watched);
    }

    watch_subarray operator[](size_t at)
    {
        return watch_subarray(watches.begin() + at, this);
    }

    watch_subarray_const operator[](size_t at) const
    {
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
        //__builtin_prefetch(watches[at].data());
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
    return *(base->base_ptr + base_at->offset + at);
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
    return base->base_ptr + base_at->offset;
}

inline Watched* watch_subarray::end()
{
    return begin() + base_at->size;
}

inline const Watched* watch_subarray::begin() const
{
    return base->base_ptr + base_at->offset;
}

inline const Watched* watch_subarray::end() const
{
    return begin() + base_at->size;
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
        uint32_t new_offset = base->get_space(new_alloc);

        //Copy
        if (base_at->size > 0) {
            Watched* newptr = base->base_ptr + new_offset;
            Watched* oldptr = base->base_ptr + base_at->offset;
            memmove(newptr, oldptr, base_at->size * sizeof(Watched));
            base->delete_offset(base_at->offset);
        }

        //Update
        base_at->offset = new_offset;
        base_at->alloc = new_alloc;
    }

    //There is enough space
    assert(base_at->alloc > base_at->size);

    //Append to the end
    *(base->base_ptr + base_at->offset + base_at->size) = watched;
    base_at->size++;
}

inline const Watched& watch_subarray_const::operator[](const uint32_t at) const
{
    return *(base->base_ptr + base_at->offset + at);
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
    return base->base_ptr + base_at->offset;
}

inline const Watched* watch_subarray_const::end() const
{
    return begin() + base_at->size;
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
