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

private:
    vector<T> inter;
};

}

#endif //__MYSTACK_H__