/******************************************
Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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

#ifndef __FRAT_H__
#define __FRAT_H__

#include "constants.h"
#include "clause.h"
#include "sqlstats.h"

#include <vector>
#include <iostream>
#include <stdio.h>

using std::vector;
// #define DEBUG_FRAT

#if 0
#define FRAT_PRINT((...) \
    do { \
        const uint32_t tmp_num = sprintf((char*)buf_ptr, __VA_ARGS__); \
        buf_ptr+=tmp_num; \
        buf_len+=tmp_num; \
    } while (0)
#else
#define FRAT_PRINT(...) do {} while (0)
#endif


namespace CMSat {

enum DratFlag{fin, deldelay, del, findelay, add, origcl, chain, finalcl, reloc};

class Drat
{
public:
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

    virtual void set_sumconflicts_ptr(uint64_t*)
    {
    }

    virtual void set_sqlstats_ptr(SQLStats*)
    {
    }

    virtual void forget_delay()
    {
    }

    virtual bool get_conf_id() {
        return false;
    }

    virtual bool something_delayed()
    {
        return false;
    }

    virtual Drat& operator<<(const int32_t)
    {
        return *this;
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

    virtual Drat& operator<<(const char*)
    {
        return *this;
    }

    virtual Drat& operator<<(const DratFlag)
    {
        return *this;
    }

    virtual void setFile(FILE*)
    {
    }

    virtual FILE* getFile()
    {
        return NULL;
    }

    virtual void flush();

    int buf_len;
    unsigned char* drup_buf = NULL;
    unsigned char* buf_ptr = NULL;
};

template<bool binfrat = false>
class DratFile: public Drat
{
public:
    DratFile(vector<uint32_t>& _interToOuterMain) :
        interToOuterMain(_interToOuterMain)
    {
        drup_buf = new unsigned char[2 * 1024 * 1024];
        buf_ptr = drup_buf;
        buf_len = 0;
        memset(drup_buf, 0, 2 * 1024 * 1024);

        del_buf = new unsigned char[2 * 1024 * 1024];
        del_ptr = del_buf;
        del_len = 0;
    }

    virtual ~DratFile()
    {
        flush();
        delete[] drup_buf;
        delete[] del_buf;
    }

    virtual void set_sumconflicts_ptr(uint64_t* _sumConflicts) override
    {
        sumConflicts = _sumConflicts;
    }

    virtual void set_sqlstats_ptr(SQLStats* _sqlStats) override
    {
        sqlStats = _sqlStats;
    }

    virtual Drat& operator<<(const int32_t clauseID) override
    {
        if (must_delete_next) {
            byteDRUPdID(clauseID);
        } else {
            byteDRUPaID(clauseID);
        }
        return *this;
    }

    virtual FILE* getFile() override
    {
        return drup_file;
    }

    void flush() override
    {
        binDRUP_flush();
    }

    void binDRUP_flush() {
        fwrite(drup_buf, sizeof(unsigned char), buf_len, drup_file);
        buf_ptr = drup_buf;
        buf_len = 0;
    }

    void setFile(FILE* _file) override
    {
        drup_file = _file;
    }

    bool something_delayed() override
    {
        return delete_filled;
    }

    void forget_delay() override
    {
        del_ptr = del_buf;
        del_len = 0;
        must_delete_next = false;
        delete_filled = false;
    }

    bool enabled() override
    {
        return true;
    }

    int del_len = 0;
    unsigned char* del_buf;
    unsigned char* del_ptr;

    bool delete_filled = false;
    bool must_delete_next = false;

    Drat& operator<<(const Clause& cl) override
    {
        if (must_delete_next) {
            byteDRUPdID(cl.stats.ID);
            for(const Lit l: cl) byteDRUPd(l);
        } else {
            byteDRUPaID(cl.stats.ID);
            for(const Lit l: cl) byteDRUPa(l);
        }

        return *this;
    }

    Drat& operator<<(const vector<Lit>& cl) override
    {
        if (must_delete_next) {
            for(const Lit l: cl) {
                byteDRUPd(l);
            }
        } else {
            for(const Lit l: cl) {
                byteDRUPa(l);
            }
        }

        return *this;
    }

    Drat& operator<<(const DratFlag flag) override
    {
        switch (flag)
        {
            case DratFlag::fin:
                if (must_delete_next) {
                    if (binfrat) {
                        *del_ptr++ = 0;
                        del_len++;
                    } else {
                        *del_ptr++ = '0';
                        *del_ptr++ = '\n';
                        del_len+=2;
                    }
                    delete_filled = true;
                } else {
                    if (binfrat) {
                        *buf_ptr++ = 0;
                        buf_len++;
                    } else {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = '\n';
                        buf_len+=2;
                    }
                    if (buf_len > 1048576) {
                        binDRUP_flush();
                    }
                    if (adding && sqlStats) sqlStats->set_id_confl(cl_id, *sumConflicts);
                    FRAT_PRINT("c set_id_confl (%d, %lld), adding: %d\n", cl_id, *sumConflicts, adding);
                }
                cl_id = 0;
                must_delete_next = false;
                break;

            case DratFlag::deldelay:
                adding = false;
                assert(!delete_filled);
                forget_delay();
                *del_ptr++ = 'd';
                del_len++;
                if (!binfrat)  {
                    *del_ptr++ = ' ';
                    del_len++;
                }
                delete_filled = false;
                must_delete_next = true;
                break;

            case DratFlag::findelay:
                assert(delete_filled);
                memcpy(buf_ptr, del_buf, del_len);
                buf_len += del_len;
                buf_ptr += del_len;
                if (buf_len > 1048576) {
                    binDRUP_flush();
                }

                forget_delay();
                break;

            case DratFlag::add:
                adding = true;
                cl_id = 0;
                *buf_ptr++ = 'a';
                buf_len++;
                if (!binfrat) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case DratFlag::chain:
                if (!binfrat) {
                    *buf_ptr++ = '0';
                    *buf_ptr++ = ' ';
                    *buf_ptr++ = 'l';
                    *buf_ptr++ = ' ';
                    buf_len+=4;
                }
                break;

            case DratFlag::del:
                adding = false;
                *buf_ptr++ = 'd';
                buf_len++;
                if (!binfrat) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case DratFlag::reloc:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'r';
                buf_len++;
                if (!binfrat) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case DratFlag::finalcl:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'f';
                buf_len++;
                if (!binfrat) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case DratFlag::origcl:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'o';
                buf_len++;
                if (!binfrat) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

                break;

        }

        return *this;
    }

private:
    Drat& operator<<(const Lit lit) override
    {
        if (must_delete_next) {
            byteDRUPd(lit);
        } else {
            byteDRUPa(lit);
        }

        return *this;
   }

    void byteDRUPa(const Lit l)
    {
        uint32_t v = l.var();
        v = interToOuterMain[v];
        if (binfrat) {
            unsigned int u = 2 * (v + 1) + l.sign();
            do {
                *buf_ptr++ = (u & 0x7f) | 0x80;
                buf_len++;
                u = u >> 7;
            } while (u);
            // End marker of this unsigned number
            *(buf_ptr - 1) &= 0x7f;
        } else {
            uint32_t num = sprintf(
                (char*)buf_ptr, "%s%d ", (l.sign() ? "-": ""), l.var()+1);
            buf_ptr+=num;
            buf_len+=num;
        }
    }

    virtual Drat& operator<<(const char* str)
    {
        #ifdef DEBUG_FRAT
        this->flush();
        uint32_t num = sprintf((char*)buf_ptr, "c %s", str);
        buf_ptr+=num;
        buf_len+=num;
        this->flush();
        #endif

        return *this;
    }

    void byteDRUPaID(const int32_t id)
    {
        if (adding && cl_id == 0) cl_id = id;
        if (binfrat) {
            for(unsigned i = 0; i < 6; i++) {
                *buf_ptr++ = (id>>(8*i))&0xff;
                buf_len++;
            }
        } else {
            uint32_t num = sprintf((char*)buf_ptr, "%d ", id);
            buf_ptr+=num;
            buf_len+=num;
        }
    }

    void byteDRUPdID(const int32_t id)
    {
        if (binfrat) {
            for(unsigned i = 0; i < 6; i++) {
                *del_ptr++ = (id>>(8*i))&0xff;
                del_len++;
            }
        } else {
            uint32_t num = sprintf((char*)del_ptr, "%d ", id);
            del_ptr+=num;
            del_len+=num;
        }
    }

    void byteDRUPd(Lit l)
    {
        uint32_t v = l.var();
        v = interToOuterMain[v];
        if (binfrat) {
            unsigned int u = 2 * (v + 1) + l.sign();
            do {
                *del_ptr++ = (u & 0x7f) | 0x80;
                del_len++;
                u = u >> 7;
            } while (u);

            // End marker of this unsigned number
            *(del_ptr - 1) &= 0x7f;
        } else {
            uint32_t num = sprintf(
                (char*)del_ptr, "%s%d ", (l.sign() ? "-": ""), l.var()+1);
            del_ptr+=num;
            del_len+=num;
        }
    }

    bool adding = false;
    int32_t cl_id = 0;
    FILE* drup_file = nullptr;
    vector<uint32_t>& interToOuterMain;
    uint64_t* sumConflicts = nullptr;
    SQLStats* sqlStats = NULL;
};

}

#endif //__FRAT_H__
