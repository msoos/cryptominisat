#ifndef __WATCHARRAY_H__
#define __WATCHARRAY_H__

#include "watched.h"
#include <vector>

namespace CMSat {
using namespace CMSat;
using std::vector;

struct watch_subarray
{
    explicit watch_subarray(vector<Watched>& _array) :
        array(_array)
    {}

    vector<Watched>& array;
    Watched& operator[](const size_t at)
    {
        return array[at];
    }

    struct iterator
    {
        explicit iterator(vector<Watched>::iterator _it) :
            it(_it)
        {}

        vector<Watched>::iterator it;

        iterator operator++()
        {
            it++;
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            it++;
            return tmp;
        }

        Watched& operator*()
        {
            return *it;
        }
    };

    struct const_iterator
    {
        explicit const_iterator(vector<Watched>::const_iterator _it) :
            it(_it)
        {}

        vector<Watched>::const_iterator it;

        const_iterator operator++()
        {
            it++;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            it++;
            return tmp;
        }

        const Watched& operator*() const
        {
            return *it;
        }
    };

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    iterator begin()
    {
        return iterator(array.begin());
    }

    iterator end()
    {
        return iterator(array.end());
    }

    const_iterator begin() const
    {
        return const_iterator(array.begin());
    }

    const_iterator end() const
    {
        return const_iterator(array.end());
    }

    void shrink(const size_t num)
    {
        array.resize(array.size()-1);
    }

    void shrink_(const size_t num)
    {
        shrink(num);
    }
};

struct watch_subarray_const
{
    explicit watch_subarray_const(const vector<Watched>& _array) :
        array(_array)
    {}

    const vector<Watched>& array;
    const Watched& operator[](const size_t at) const
    {
        return array[at];
    }

    struct const_iterator
    {
        explicit const_iterator(vector<Watched>::const_iterator _it) :
            it(_it)
        {}

        vector<Watched>::const_iterator it;

        const_iterator operator++()
        {
            it++;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            it++;
            return tmp;
        }

        const Watched& operator*() const
        {
            return *it;
        }
    };

    size_t size() const
    {
        return array.size();
    }

    bool empty() const
    {
        return array.empty();
    }

    const_iterator begin() const
    {
        return const_iterator(array.begin());
    }

    const_iterator end() const
    {
        return const_iterator(array.end());
    }
};

struct watch_array
{
    vector<vector<Watched> > watches;

    watch_subarray operator[](size_t at)
    {
        return watch_subarray(watches[at]);
    }

    watch_subarray_const operator[](size_t at) const
    {
        return watch_subarray_const(watches[at]);
    }

    void new_var()
    {
        watches.resize(watches.size()+2);
    }

    void resize(const size_t new_size)
    {
        watches.resize(new_size);
    }

    size_t capacity() const
    {
        return watches.capacity();
    }

    size_t size() const
    {
        return watches.size();
    }

    void shrink_to_fit()
    {
        watches.shrink_to_fit();
    }

    void prefetch(const size_t at) const
    {
        __builtin_prefetch(watches[at].data());
    }

    struct iterator
    {
        vector<vector<Watched> >::iterator it;
        explicit iterator(vector<vector<Watched> >::iterator _it) :
            it(_it)
        {}

        iterator operator++()
        {
            ++it;
            return *this;
        }

        iterator operator++(int)
        {
            iterator tmp = *this;
            ++it;
            return tmp;
        }

        vector<Watched>& operator*()
        {
            return *it;
        }

        bool operator==(const iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const iterator& it2) const
        {
            return it != it2.it;
        }

        vector<Watched>& operator->()
        {
            return *it;
        }
    };

    struct const_iterator
    {
        vector<vector<Watched> >::const_iterator it;
        explicit const_iterator(vector<vector<Watched> >::const_iterator _it) :
            it(_it)
        {}

        const_iterator operator++()
        {
            ++it;
            return *this;
        }

        const_iterator operator++(int)
        {
            const_iterator tmp = *this;
            ++it;
            return tmp;
        }

        const vector<Watched>& operator*() const
        {
            return *it;
        }

        bool operator==(const const_iterator& it2) const
        {
            return it == it2.it;
        }

        bool operator!=(const const_iterator& it2) const
        {
            return it != it2.it;
        }

        const vector<Watched>& operator->() const
        {
            return *it;
        }
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
};

} //End of namespace

#endif //__WATCHARRAY_H__
