/******************************************
Copyright (C) 2009-2024 Authors of CryptoMiniSat, see AUTHORS file

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

#include "clause.h"
#include "frat.h"
#include "sqlstats.h"

#include <vector>
#include <cstdio>

using std::vector;
#define DEBUG_IDRUP

#if 0
#define IDRUP_PRINT((...) \
    do { \
        const uint32_t tmp_num = sprintf((char*)buf_ptr, __VA_ARGS__); \
        buf_ptr+=tmp_num; \
        buf_len+=tmp_num; \
    } while (0)
#else
#define IDRUP_PRINT(...) do {} while (0)
#endif


namespace CMSat {

template<bool binidrup = false>
class IdrupFile: public Frat
{
  const int flush_bound = 32768; // original value: 1048576;
public:
    IdrupFile(vector<uint32_t>& _interToOuterMain) :
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

    ~IdrupFile() override {
        flush();
        delete[] drup_buf;
        delete[] del_buf;
    }

    void set_sumconflicts_ptr(uint64_t* _sumConflicts) override
    {
        sumConflicts = _sumConflicts;
    }

    void set_sqlstats_ptr(SQLStats* _sqlStats) override
    {
        sqlStats = _sqlStats;
    }

    FILE* getFile() override
    {
        return drup_file;
    }

    void flush() override
    {
      binDRUP_flush();
    }

    void binDRUP_flush() {
        fwrite(drup_buf, sizeof(unsigned char), buf_len, drup_file);
        fflush (drup_file);
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

    bool incremental() override
    {
        return true;
    }
    int del_len = 0;
    unsigned char* del_buf;
    unsigned char* del_ptr;

    bool delete_filled = false;
    bool must_delete_next = false;

    virtual Frat& operator<<(const int32_t) override
    {
#if 0 // clauseID
        if (must_delete_next) {
            byteDRUPdID(clauseID);
        } else {
            byteDRUPaID(clauseID);
        }
#endif
        return *this;
    }

    Frat& operator<<(const Clause& cl) override
    {
        if (skipnextclause) return *this;
        if (must_delete_next) {
            byteDRUPdID(cl.stats.ID);
            for(const Lit l: cl) byteDRUPd(l);
        } else {
            byteDRUPaID(cl.stats.ID);
            for(const Lit l: cl) byteDRUPa(l);
        }

        return *this;
    }

    Frat& operator<<(const vector<Lit>& cl) override {
      if (skipnextclause) return *this;
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

    Frat& operator<<(const FratOutcome o) override
    {
        uint32_t num;
	switch(o) {
	    case unsatisfiable:
	      this->flush();
	      num = sprintf((char*)buf_ptr, "s UNSATISFIABLE\n");
	      buf_ptr+=num;
	      buf_len+=num;
	      this->flush();
	      break;
	    case satisfiable:
	      this->flush();
	      num = sprintf((char*)buf_ptr, "s SATISFIABLE\n");
	      buf_ptr+=num;
	      buf_len+=num;
	      this->flush();
	      break;
	    case unknown:
	      this->flush();
	      num = sprintf((char*)buf_ptr, "s UNKNOWN\n");
	      buf_ptr+=num;
	      buf_len+=num;
	      this->flush();
	      break;
	}
        return *this;
    }

    Frat& operator<<(const FratFlag flag) override
    {
        const bool old = skipnextclause;
        skipnextclause = false;

        switch (flag)
        {
            case FratFlag::fin:
                if (old) break;
                if (must_delete_next) {
                    if (binidrup) {
                        *del_ptr++ = 0;
                        del_len++;
                    } else {
                        *del_ptr++ = '0';
                        *del_ptr++ = '\n';
                        del_len+=2;
                    }
                    delete_filled = true;
                } else {
                    if (binidrup) {
                        *buf_ptr++ = 0;
                        buf_len++;
                    } else {
                        *buf_ptr++ = '0';
                        *buf_ptr++ = '\n';
                        buf_len+=2;
                    }
                    if (buf_len > flush_bound) {
                        binDRUP_flush();
                    }
                    if (adding && sqlStats) sqlStats->set_id_confl(cl_id, *sumConflicts);
                    IDRUP_PRINT("c set_id_confl (%d, %lld), adding: %d\n", cl_id, *sumConflicts, adding);
                }
                cl_id = 0;
                must_delete_next = false;
  	            if (flushing)
		                this->flush(), --flushing;
                break;

            case FratFlag::deldelay:
                adding = false;
                assert(!delete_filled);
                forget_delay();
                *del_ptr++ = 'd';
                del_len++;
                if (!binidrup)  {
                    *del_ptr++ = ' ';
                    del_len++;
                }
                delete_filled = false;
                must_delete_next = true;
                break;

            case FratFlag::findelay:
                assert(delete_filled);
                memcpy(buf_ptr, del_buf, del_len);
                buf_len += del_len;
                buf_ptr += del_len;
                if (buf_len > flush_bound) {
                    binDRUP_flush();
                }

                forget_delay();
                break;

            case FratFlag::add:
                adding = true;
                cl_id = 0;
                *buf_ptr++ = 'l';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case FratFlag::fratchain:
                break;

            case FratFlag::del:
                adding = false;
                *buf_ptr++ = 'd';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;

            case FratFlag::reloc:
  	        skipnextclause = true;
                // adding = false;
                // forget_delay();
                // *buf_ptr++ = 'r';
                // buf_len++;
                // if (!binidrup) {
                //     *buf_ptr++ = ' ';
                //     buf_len++;
                // }
	      break;

            case FratFlag::finalcl:
	      assert (false);
              break;

            case FratFlag::origcl:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'i';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

                break;
            case FratFlag::weakencl:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'w';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

                break;
            case FratFlag::restorecl:
                adding = false;
                forget_delay();
                *buf_ptr++ = 'r';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

                break;
            case FratFlag::assump:
                this->flush();
                adding = false, flushing = 2;
                forget_delay();
                *buf_ptr++ = 'q';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

                break;
            case FratFlag::modelF:
                this->flush();
                adding = false, flushing = 2;
                forget_delay();
                *buf_ptr++ = 'm';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }
                break;
            case FratFlag::unsatcore:
                this->flush();
                adding = false, flushing = 2;
                forget_delay();
                *buf_ptr++ = 'u';
                buf_len++;
                if (!binidrup) {
                    *buf_ptr++ = ' ';
                    buf_len++;
                }

              break;
	    case FratFlag::deldelayx:
	    case FratFlag::delx:
	    case FratFlag::finalx:
	    case FratFlag::implyclfromx:
	    case FratFlag::implyxfromcls:
	    case FratFlag::origclx:
  	      skipnextclause = true;
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
        if (skipnextclause) return *this;
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
        if (binidrup) {
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
#ifdef DEBUG_IDRUP
        this->flush();
        uint32_t num = sprintf((char*)buf_ptr, "c %s", str);
        buf_ptr+=num;
        buf_len+=num;
        this->flush();
#endif

        return *this;
    }



    void byteDRUPaID(const int32_t)
    {
#if 0
        if (adding && cl_id == 0) cl_id = id;
        if (binidrup) {
            for(unsigned i = 0; i < 6; i++) {
                *buf_ptr++ = (id>>(8*i))&0xff;
                buf_len++;
            }
        } else {
            uint32_t num = sprintf((char*)buf_ptr, "%d ", id);
            buf_ptr+=num;
            buf_len+=num;
        }
#endif
    }

    void byteDRUPdID(const int32_t)
    {
#if 0
        if (binidrup) {
            for(unsigned i = 0; i < 6; i++) {
                *del_ptr++ = (id>>(8*i))&0xff;
                del_len++;
            }
        } else {
            uint32_t num = sprintf((char*)del_ptr, "%d ", id);
            del_ptr+=num;
            del_len+=num;
        }
#endif
    }

    void byteDRUPd(Lit l)
    {
        uint32_t v = l.var();
        v = interToOuterMain[v];
        if (binidrup) {
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
    int flushing = 0;
    int32_t cl_id = 0;
    FILE* drup_file = nullptr;
    vector<uint32_t>& interToOuterMain;
    uint64_t* sumConflicts = nullptr;
    SQLStats* sqlStats = NULL;
    bool skipnextclause = false;
};

}
