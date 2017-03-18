/******************************************
Copyright (c) 2016, Mate Soos

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
***********************************************/

#ifndef __DRAT_H__
#define __DRAT_H__

#include "clause.h"
#include <iostream>

namespace CMSat {

enum DratFlag{fin, deldelay, del, findelay};

struct Drat
{
    Drat()
    {
    }

    virtual ~Drat()
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

    virtual Drat& operator<<(const Lit)
    {

        return *this;
    }

    virtual Drat& operator<<(const Clause&)
    {
        return *this;
    }

    virtual Drat& operator<<(const vector<Lit>&)
    {
        return *this;
    }

    virtual Drat& operator<<(const DratFlag)
    {
        return *this;
    }

    virtual void setFile(std::ostream*)
    {
    }
};

template<bool add_ID>
struct DratFile: public Drat
{
    void setFile(std::ostream* _file) override
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

    Drat& operator<<(const Lit lit) override
    {
        if (must_delete_next) {
            todel << lit << " ";
        } else {
            *file << lit << " ";
        }

        return *this;
    }

    Drat& operator<<(const Clause& cl) override
    {
        if (must_delete_next) {
            todel << cl << " ";
        } else {
            *file << cl << " ";
        }
        #ifdef STATS_NEEDED
        if (add_ID) {
            ID = cl.stats.ID;
            assert(ID != 0);
        }
        #endif

        return *this;
    }

    Drat& operator<<(const DratFlag flag) override
    {
        switch (flag)
        {
            case DratFlag::fin:
                if (must_delete_next) {
                    todel << "0\n";
                    delete_filled = true;
                } else {
                    if (add_ID) {
                        *file << "0 "
                        << ID
                        << "\n";
                    } else {
                        *file << "0 \n";
                    }
                }
                if (add_ID) {
                    ID = 1;
                }
                must_delete_next = false;
                break;

            case DratFlag::deldelay:
                assert(!delete_filled);
                assert(todel.str() == "");
                todel.str(string());
                delete_filled = false;

                must_delete_next = true;
                break;

            case DratFlag::findelay:
                assert(delete_filled);
                *file << "d " << todel.str();
                todel.str(string());
                delete_filled = false;
                break;

            case DratFlag::del:
                todel.str(string());
                delete_filled = false;

                must_delete_next = false;
                *file << "d ";
                break;
        }

        return *this;
    }

    Drat& operator<<(const vector<Lit>& lits) override
    {
        if (must_delete_next) {
            todel << lits << " ";
        } else {
            *file << lits << " ";
        }

        return *this;
    }

    std::ostream* file = NULL;
    int64_t ID = 1;
};

}

#endif //__DRAT_H__
