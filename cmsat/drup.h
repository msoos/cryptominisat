#ifndef __DRUP_H__
#define __DRUP_H__

namespace CMSat {
using namespace CMSat;

struct Drup
{
    void setFile(std::ostream*)
    {
    }

    bool enabled()
    {
        return true;
    }

    Drup operator<<(const Lit) const
    {

        return *this;
    }

    Drup operator<<(const char*) const
    {
        return *this;
    }

    Drup operator<<(const Clause&) const
    {
        return *this;
    }

    Drup operator<<(const vector<Lit>&) const
    {
        return *this;
    }
};

}

#endif //__DRUP_H__
