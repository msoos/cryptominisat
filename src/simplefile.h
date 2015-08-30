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

#ifndef __SIMPLEFILE_H__
#define __SIMPLEFILE_H__

#include <fstream>
#include <iostream>
#include <string>
#include <cstdint>
using std::ios;

#include "solvertypes.h"

namespace CMSat {
using namespace CMSat;

class SimpleOutFile
{
public:
    void start(const string& fname)
    {
        outf = new std::ofstream(fname.c_str(), ios::out | ios::binary);
        outf->exceptions(~std::ios::goodbit);
        //buffer.resize(100000000);
        //outf->rdbuf()->pubsetbuf(&buffer.front(), buffer.size());
    }

    ~SimpleOutFile()
    {
        delete outf;
    }

    void put_uint32_t(const uint32_t val)
    {
        put(&val, 4);
    }

    void put_lbool(const lbool val)
    {
        put(&val, sizeof(lbool));
    }

    void put_uint64_t(const uint64_t val)
    {
        put(&val, 8);
    }

    void put_lit(const Lit l)
    {
        put_uint32_t(l.toInt());
    }

    template<class T>
    void put_vector(const vector<T>& d)
    {
        put_uint64_t(d.size());
        if (d.size() == 0)
            return;

        put(&d[0], d.size() * sizeof(T));
    }

    template<class T>
    void put_struct(const T& d)
    {
        put(&d, sizeof(T));
    }

private:
    std::ofstream* outf = NULL;
    //vector<char> buffer;

    void put(const void* ptr, size_t num)
    {
        outf->write((const char*)ptr, num);
    }
};

class SimpleInFile
{
public:
    void start(const string& fname)
    {
        inf = new std::ifstream(fname.c_str(), ios::in | ios::binary);
        inf->exceptions(~std::ios::goodbit);
    }

    ~SimpleInFile()
    {
        delete inf;
    }

    uint32_t get_uint32_t()
    {
        uint32_t val = 0;
        inf->read((char*)&val, 4);
        return val;
    }

    uint64_t get_uint64_t()
    {
        uint64_t val = 0;
        inf->read((char*)&val, 8);
        return val;
    }

    Lit get_lit()
    {
        uint32_t l = get_uint32_t();
        return Lit::toLit(l);
    }

    lbool get_lbool()
    {
        lbool l;
        inf->read((char*)&l, sizeof(lbool));
        return l;
    }

    template<class T>
    void get_vector(vector<T>& d)
    {
        assert(d.empty());
        uint64_t sz = get_uint64_t();
        if (sz == 0)
            return;

        d.resize(sz);
        get_raw(&d[0], d.size(), sizeof(T));
    }

    template<class T>
    void get_struct(T& d)
    {
        inf->read((char*)&d, sizeof(T));
    }

private:
    std::ifstream* inf = NULL;

    void get_raw(void* ptr, size_t num, size_t elem_sz)
    {
        inf->read((char*)ptr, num*elem_sz);
    }
};

}

#endif //__SIMPLEFILE_H__
