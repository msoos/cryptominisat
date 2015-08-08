/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

#ifndef __MYSTACK_H__
#define __MYSTACK_H__

#include <vector>
using std::vector;

namespace CMSat {

template<class T>
class MyStack
{
public:
    void clear()
    {
        inter.clear();
    }

    bool empty() const
    {
        return inter.empty();
    }

    void pop()
    {
        assert(!inter.empty());
        inter.resize(inter.size()-1);
    }

    const T top() const
    {
        return inter.back();
    }

    void push(const T& data)
    {
        inter.push_back(data);
    }

    size_t capacity() const
    {
        return inter.capacity();
    }

    size_t mem_used() const
    {
        return capacity()*sizeof(T);
    }

private:
    vector<T> inter;
};

}

#endif //__MYSTACK_H__