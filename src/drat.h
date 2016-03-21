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

#ifndef __DRAT_H__
#define __DRAT_H__

#include "clause.h"
#include <iostream>

namespace CMSat {
using namespace CMSat;

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
