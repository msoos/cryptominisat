/*************************************************************
MiniSat       --- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat --- Copyright (c) 2014, Mate Soos

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
***************************************************************/

#ifndef __WATCHARRAY_H__
#define __WATCHARRAY_H__

#include "watched.h"
#include "Vec.h"
#include <vector>

namespace CMSat {
using namespace CMSat;
using std::vector;

struct watch_subarray
{
    explicit watch_subarray(vec<Watched>& _array) :
        array(_array)
    {}

    vec<Watched>& array;
    Watched& operator[](const size_t at)
    {
        return array[at];
    }

    Watched& at(const size_t at)
    {
        return array[at];
    }

    void clear()
    {
        array.clear();
    }

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    Watched* begin()
    {
        return array.data();
    }

    Watched* end()
    {
        return array.data()+array.size();
    }

    const Watched* begin() const
    {
        return array.data();
    }

    const Watched* end() const
    {
        return array.data()+array.size();
    }

    void shrink(const size_t num)
    {
        array.resize(array.size()-num);
    }

    void shrink_(const size_t num)
    {
        shrink(num);
    }

    void push(const Watched& watched)
    {
        array.push(watched);
    }

    typedef Watched* iterator;
    typedef const Watched* const_iterator;
};

struct watch_subarray_const
{
    explicit watch_subarray_const(const vec<Watched  >& _array) :
        array(_array)
    {}

    const vec<Watched  >& array;

    watch_subarray_const(const watch_subarray& other) :
        array(other.array)
    {}

    const Watched& operator[](const size_t pos) const
    {
        return array[pos];
    }

    const Watched& at(const size_t pos) const
    {
        return array[pos];
    }

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    const Watched* begin() const
    {
        return array.data();
    }

    const Watched* end() const
    {
        return array.data() + array.size();
    }

    typedef const Watched* const_iterator;
};

class watch_array
{
public:
    vector<vec<Watched> > watches;
    vector<Lit> smudged_list;
    vector<char> smudged;

    void smudge(const Lit lit) {
        if (!smudged[lit.toInt()]) {
            smudged_list.push_back(lit);
            smudged[lit.toInt()] = true;
        }
    }

    const vector<Lit>& get_smudged_list() const {
        return smudged_list;
    }

    void clear_smudged()
    {
        for(const Lit lit: smudged_list) {
            assert(smudged[lit.toInt()]);
            smudged[lit.toInt()] = false;
        }
        smudged_list.clear();
    }

    watch_subarray operator[](size_t pos)
    {
        return watch_subarray(watches[pos]);
    }

    watch_subarray at(size_t pos)
    {
        return watch_subarray(watches.at(pos));
    }

    watch_subarray_const operator[](size_t at) const
    {
        return watch_subarray_const(watches[at]);
    }

    watch_subarray_const at(size_t pos) const
    {
        return watch_subarray_const(watches.at(pos));
    }

    void resize(const size_t new_size)
    {
        assert(smudged_list.empty());
        watches.resize(new_size);
        smudged.resize(new_size, false);
    }

    size_t mem_used() const
    {
        double mem = watches.capacity()*sizeof(vec<Watched>);
        for(size_t i = 0; i < watches.size(); i++) {
            //1.2 is overhead
            mem += (double)watches[i].capacity()*(double)sizeof(Watched)*1.2;
        }
        mem += smudged.capacity()*sizeof(char);
        mem += smudged_list.capacity()*sizeof(Lit);
        return mem;
    }

    size_t size() const
    {
        return watches.size();
    }

    void prefetch(const size_t at) const
    {
        __builtin_prefetch(watches[at].data());
    }

    struct iterator
    {
        vector<vec<Watched> >::iterator it;
        explicit iterator(vector<vec<Watched> >::iterator _it) :
            it(_it)
        {}

        iterator operator++()
        {
            ++it;
            return *this;
        }

        watch_subarray operator*()
        {
            return watch_subarray(*it);
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
        vector<vec<Watched  > >::const_iterator it;
        explicit const_iterator(vector<vec<Watched  > >::const_iterator _it) :
            it(_it)
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
            return watch_subarray_const(*it);
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
        return iterator(watches.begin());
    }

    iterator end()
    {
        return iterator(watches.end());
    }

    const_iterator begin() const
    {
        return const_iterator(watches.begin());
    }

    const_iterator end() const
    {
        return const_iterator(watches.end());
    }

    /*void fitToSize()
    {
        consolidate();
    }*/

    void consolidate()
    {
        for(auto& ws: watches) {
            ws.shrink_to_fit();
        }
        watches.shrink_to_fit();
    }

    void print_stat()
    {
    }

    size_t mem_used_alloc() const
    {
        size_t mem = 0;
        for(auto& ws: watches) {
            mem += ws.capacity()*sizeof(Watched);
        }

        return mem;
    }

    size_t mem_used_array() const
    {
        size_t mem = 0;
        mem += watches.capacity()*sizeof(vector<Watched>);
        mem += sizeof(watch_array);
        return mem;
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

inline void swap(watch_subarray a, watch_subarray b)
{
    a.array.swap(b.array);
}


} //End of namespace

#endif //__WATCHARRAY_H__
