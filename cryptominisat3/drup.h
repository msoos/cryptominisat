#ifndef __DRUP_H__
#define __DRUP_H__

#include "clause.h"
#include <iostream>

namespace CMSat {
using namespace CMSat;

enum DrupFlag{fin, deldelay, del, findelay};

struct Drup
{
    Drup()
    {
    }

    virtual ~Drup()
    {
    }

    virtual bool enabled()
    {
        return false;
    }

    virtual void forget_delay()
    {
    }

    virtual bool something_delayed()
    {
        return false;
    }

    virtual Drup& operator<<(const Lit)
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

     virtual Drup& operator<<(const DrupFlag)
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

    bool something_delayed() override
    {
        return delete_filled;
    }

    void forget_delay() override
    {
        todel.str(string());
        must_delete_next = false;
        delete_filled = false;
    }

    bool enabled() override
    {
        return true;
    }

    std::stringstream todel;
    bool delete_filled = false;
    bool must_delete_next = false;

    Drup& operator<<(const Lit lit) override
    {
        if (must_delete_next) {
            todel << lit << " ";
        } else {
            *file << lit << " ";
        }

        return *this;
    }

    Drup& operator<<(const Clause& cl) override
    {
        if (must_delete_next) {
            todel << cl;
        } else {
            *file << cl;
        }

        return *this;
    }

    Drup& operator<<(const DrupFlag flag) override
    {
        switch (flag)
        {
            case DrupFlag::fin:
                if (must_delete_next) {
                    todel << " 0\n";
                    delete_filled = true;
                } else {
                    *file << " 0\n";
                }
                must_delete_next = false;
                break;

            case DrupFlag::deldelay:
                assert(!delete_filled);
                assert(todel.str() == "");
                todel.str(string());
                delete_filled = false;

                must_delete_next = true;
                break;

            case DrupFlag::findelay:
                assert(delete_filled);
                *file << "d " << todel.str();
                todel.str(string());
                delete_filled = false;
                break;

            case DrupFlag::del:
                todel.str(string());
                delete_filled = false;

                must_delete_next = false;
                *file << "d ";
                break;
        }

        return *this;
    }

    Drup& operator<<(const vector<Lit>& lits) override
    {
         if (must_delete_next) {
            todel << lits;
        } else {
            *file << lits;
        }

        return *this;
    }

    std::ostream* file = NULL;
};

}

#endif //__DRUP_H__
