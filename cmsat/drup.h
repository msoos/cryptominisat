#ifndef __DRUP_H__
#define __DRUP_H__

#include "clause.h"
#include <iostream>

namespace CMSat {
using namespace CMSat;

struct Drup
{
    Drup()
    {
    }

    virtual bool enabled()
    {
        return false;
    }

    virtual Drup& operator<<(const Lit)
    {

        return *this;
    }

    virtual Drup& operator<<(const char*)
    {
        return *this;
    }

    virtual Drup& operator<<(const Clause&)
    {
        return *this;
    }

    virtual Drup& operator<<(const vector<Lit>&)
    {
        return *this;
    }
};

struct DrupFile: public Drup
{
    void setFile(std::ostream* _file)
    {
        file = _file;
    }

    bool enabled()
    {
        return true;
    }

    Drup& operator<<(const Lit lit)
    {
        *file << lit;

        return *this;
    }

    Drup& operator<<(const char* str)
    {
        *file << str;

        return *this;
    }

    Drup& operator<<(const Clause& cl)
    {
        *file << cl << " 0\n";

        return *this;
    }

    Drup& operator<<(const vector<Lit>& lits)
    {
        *file << lits << " 0\n";

        return *this;
    }

    std::ostream* file = NULL;
};

}

#endif //__DRUP_H__
