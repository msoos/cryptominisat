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

#pragma once

#include <vector>
#include <iostream>
#include <stdio.h>

#include "clause.h"
#include "sqlstats.h"
#include "xor.h"


using std::vector;

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

  enum FratFlag{fin, deldelay, deldelayx, del, delx, findelay, add, addx, origcl, origclx, fratchain, finalcl, finalx, reloc, implyclfromx, implyxfromcls, weakencl, restorecl, assump, unsatcore, modelF};
  enum FratOutcome{satisfiable, unsatisfiable, unknown};

class Frat
{
public:
    Frat() { }
    virtual ~Frat() { }
    virtual bool enabled() { return false; }
    virtual void set_sumconflicts_ptr(uint64_t*) { }
    virtual void set_sqlstats_ptr(SQLStats*) { }
    virtual void forget_delay() { }
    virtual bool get_conf_id() { return false; }
    virtual bool something_delayed() { return false; }
    virtual Frat& operator<<(const int32_t) { return *this; }
    virtual Frat& operator<<(const Lit) { return *this; }
    virtual Frat& operator<<(const Clause&) { return *this; }
    virtual Frat& operator<<(const Xor&) { return *this; }
    virtual Frat& operator<<(const vector<Lit>&) { return *this; }
    virtual Frat& operator<<(const char*) { return *this; }
    virtual Frat& operator<<(const FratOutcome) { return *this; }
    virtual Frat& operator<<(const FratFlag) { return *this; }
    virtual void setFile(FILE*) { }
    virtual FILE* getFile() { return nullptr; }
    virtual void flush();
    virtual bool incremental() {return false;}

    int buf_len;
    unsigned char* drup_buf = nullptr;
    unsigned char* buf_ptr = nullptr;
};

template<bool binfrat = false>
class FratFile: public Frat
{
public:
    FratFile(vector<uint32_t>& _inter_to_outerMain) :
        inter_to_outerMain(_inter_to_outerMain)
    {
        drup_buf = new unsigned char[2 * 1024 * 1024];
        buf_ptr = drup_buf;
        buf_len = 0;
        memset(drup_buf, 0, 2 * 1024 * 1024);

        del_buf = new unsigned char[2 * 1024 * 1024];
        del_ptr = del_buf;
        del_len = 0;
    }

    virtual ~FratFile()
    {
        flush();
        delete[] drup_buf;
        delete[] del_buf;
    }

    virtual void set_sumconflicts_ptr(uint64_t* _sumConflicts) override { sumConflicts = _sumConflicts; }
    virtual void set_sqlstats_ptr(SQLStats* _sqlStats) override { sqlStats = _sqlStats; }
    virtual void setFile(FILE* _file) override { drup_file = _file; }
    virtual bool something_delayed() override { return delete_filled; }
    virtual bool enabled() override { return true; }

    virtual Frat& operator<<(const int32_t clauseID) override
    {
        assert(clauseID != 0);
        if (must_delete_next) byteDRUPdID(clauseID);
        else byteDRUPaID(clauseID);
        return *this;
    }

    virtual FILE* getFile() override { return drup_file; }
    virtual void flush() override { frat_flush(); }
    void frat_flush() {
        fwrite(drup_buf, sizeof(unsigned char), buf_len, drup_file);
        buf_ptr = drup_buf;
        buf_len = 0;
    }


    virtual void forget_delay() override
    {
        del_ptr = del_buf;
        del_len = 0;
        must_delete_next = false;
        delete_filled = false;
    }


    int del_len = 0;
    unsigned char* del_buf;
    unsigned char* del_ptr;

    bool delete_filled = false;
    bool must_delete_next = false;

    virtual Frat& operator<<(const Xor& x) override
    {
        if (must_delete_next) {
            byteDRUPdID(x.XID);
            for(uint32_t i = 0; i < x.size(); i++) {
                Lit l = Lit(x[i], false);
                if (i == 0 && !x.rhs) l ^= true;
                byteDRUPd(l);
            }
        } else {
            byteDRUPaID(x.XID);
            for(uint32_t i = 0; i < x.size(); i++) {
                Lit l = Lit(x[i], false);
                if (i == 0 && !x.rhs) l ^= true;
                byteDRUPa(l);
            }
        }

        return *this;
    }

    virtual Frat& operator<<(const Clause& cl) override
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

    virtual Frat& operator<<(const vector<Lit>& cl) override {
        if (must_delete_next) for(const Lit& l: cl) byteDRUPd(l);
        else for(const Lit& l: cl) byteDRUPa(l);
        return *this;
    }

    inline void buf_add(unsigned char x) { *buf_ptr++=x; buf_len++; }
    inline void del_add(unsigned char x) { *del_ptr++=x; del_len++; }
    inline void del_nonbin_move() { if (!binfrat) del_add(' '); }
    inline void buf_nonbin_move() { if (!binfrat) buf_add(' '); }
    virtual Frat& operator<<(const FratFlag flag) override
    {
        switch (flag) {
            case FratFlag::fin:
                if (must_delete_next) {
                    if (binfrat) del_add(0);
                    else { del_add('0'); del_add('\n'); }
                    delete_filled = true;
                } else {
                    if (binfrat) buf_add(0);
                    else { buf_add('0'); buf_add('\n');}
                    if (buf_len > 1048576) { frat_flush(); }
                    if (adding && sqlStats) sqlStats->set_id_confl(cl_id, *sumConflicts);
                    FRAT_PRINT("c set_id_confl (%d, %lld), adding: %d\n", cl_id, *sumConflicts, adding);
                }
                cl_id = 0;
                must_delete_next = false;
                break;

            case FratFlag::deldelay:
                adding = false;
                assert(!delete_filled);
                forget_delay();
                del_add('d');
                del_nonbin_move();
                delete_filled = false;
                must_delete_next = true;
                break;

            case FratFlag::deldelayx:
                adding = false;
                assert(!delete_filled);
                forget_delay();
                del_add('d');
                del_add(' ');
                del_add('x');
                del_nonbin_move();
                delete_filled = false;
                must_delete_next = true;
                break;

            case FratFlag::findelay:
                assert(delete_filled);
                memcpy(buf_ptr, del_buf, del_len);
                buf_len += del_len;
                buf_ptr += del_len;
                if (buf_len > 1048576) { frat_flush(); }
                forget_delay();
                break;

            case FratFlag::add:
                adding = true;
                cl_id = 0;
                buf_add('a');
                buf_nonbin_move();
                break;

            case FratFlag::implyclfromx:
                adding = true;
                cl_id = 0;
                buf_add('i');
                buf_nonbin_move();
                break;

            case FratFlag::implyxfromcls:
                adding = true;
                cl_id = 0;
                buf_add('i');
                buf_add(' ');
                buf_add('x');
                buf_nonbin_move();
                break;

            case FratFlag::addx:
                adding = true;
                cl_id = 0;
                buf_add('a');
                buf_add(' ');
                buf_add('x');
                buf_nonbin_move();
                break;

            case FratFlag::fratchain:
                if (!binfrat) {
                    buf_add('0');
                    buf_add(' ');
                    buf_add('l');
                    buf_add(' ');
                }
                break;

            case FratFlag::weakencl:
            case FratFlag::del:
                adding = false;
                buf_add('d');
                buf_nonbin_move();
                break;

            case FratFlag::delx:
                adding = false;
                buf_add('d');
                buf_add(' ');
                buf_add('x');
                buf_nonbin_move();
                break;

            case FratFlag::reloc:
                adding = false;
                forget_delay();
                buf_add('r');
                buf_nonbin_move();
                break;

            case FratFlag::finalcl:
                adding = false;
                forget_delay();
                buf_add('f');
                buf_nonbin_move();
                break;

            case FratFlag::finalx:
                adding = false;
                forget_delay();
                buf_add('f');
                buf_add(' ');
                buf_add('x');
                buf_nonbin_move();
                break;

            case FratFlag::origcl:
                adding = false;
                forget_delay();
                buf_add('o');
                buf_nonbin_move();
                break;

            case FratFlag::origclx:
                adding = false;
                forget_delay();
                buf_add('o');
                buf_add(' ');
                buf_add('x');
                buf_nonbin_move();
                break;

            default:
  	        __builtin_unreachable();
                break;
        }

        return *this;
    }

private:
    Frat& operator<<(const Lit lit) override
    {
        if (must_delete_next) byteDRUPd(lit);
        else byteDRUPa(lit);
        return *this;
   }

    void byteDRUPa(const Lit l)
    {
        uint32_t v = l.var();
        v = inter_to_outerMain[v];
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

    virtual Frat& operator<<([[maybe_unused]] const char* str) override
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
            for(unsigned i = 0; i < 6; i++) buf_add((id>>(8*i))&0xff);
        } else {
            uint32_t num = sprintf((char*)buf_ptr, "%d ", id);
            buf_ptr+=num;
            buf_len+=num;
        }
    }

    void byteDRUPdID(const int32_t id)
    {
        if (binfrat) {
            for(unsigned i = 0; i < 6; i++) del_add((id>>(8*i))&0xff);
        } else {
            uint32_t num = sprintf((char*)del_ptr, "%d ", id);
            del_ptr+=num;
            del_len+=num;
        }
    }

    void byteDRUPd(Lit l)
    {
        uint32_t v = l.var();
        v = inter_to_outerMain[v];
        if (binfrat) {
            unsigned int u = 2 * (v + 1) + l.sign();
            do {
                del_add((u & 0x7f) | 0x80);
                u = u >> 7;
            } while (u);

            // End marker of this unsigned number
            *(del_ptr - 1) &= 0x7f;
        } else {
            uint32_t num = sprintf((char*)del_ptr, "%s%d ", (l.sign() ? "-": ""), l.var()+1);
            del_ptr+=num;
            del_len+=num;
        }
    }

    bool adding = false;
    int32_t cl_id = 0;
    FILE* drup_file = nullptr;
    vector<uint32_t>& inter_to_outerMain;
    uint64_t* sumConflicts = nullptr;
    SQLStats* sqlStats = nullptr;
};

}
