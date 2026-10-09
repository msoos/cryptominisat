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
#include <cstdio>
#include <charconv>

#include "clause.h"
#include "sqlstats.h"
#include "xor.h"

using std::vector;


namespace CMSat {

  enum FratFlag{fin, deldelay, deldelayx, del, delx, findelay, add, addx, origcl, origclx, fratchain, finalcl, finalx, reloc,
      implyclfromx, implyxfromcls, weakencl, restorecl, assump, unsatcore, modelF};
  enum FratOutcome{satisfiable, unsatisfiable, unknown};

class Frat
{
public:
    Frat() = default;
    virtual ~Frat() = default;
    virtual bool enabled() { return false; }
    virtual void set_sumconflicts_ptr(uint64_t*) { }
    virtual void set_sqlstats_ptr(SQLStats*) { }
    virtual void forget_delay() { }
    virtual bool get_conf_id() { return false; }
    virtual bool something_delayed() { return false; }
    virtual Frat& operator<<(const int32_t) { return *this; }
    Frat& operator<<(const vector<int32_t>& ids) {
        if (!enabled()) return *this;
        for(const auto& i: ids) { assert(i != 0); *this << i; }
        return *this;
    }
    virtual Frat& operator<<(const Lit) { return *this; }
    virtual Frat& operator<<(const Clause&) { return *this; }
    virtual Frat& operator<<(const Xor&) { return *this; }
    virtual Frat& operator<<(const vector<Lit>&) { return *this; }
    virtual Frat& operator<<(const char*) { return *this; }
    virtual Frat& operator<<(const FratOutcome) { return *this; }
    virtual Frat& operator<<(const FratFlag) { return *this; }
    virtual void setFile(FILE*) { }
    virtual FILE* getFile() { return nullptr; }
    virtual void flush() {}
    virtual bool incremental() {return false;}
};

//Emits the XLRUP format directly, so `cake_xlrup` can check the proof with
//no elaboration. Buffers one step at a time; `o`/`f` clause steps and
//relocs are dropped (inputs are numbered by file position).
//Text is for `cake_xlrup --no-binary`, binary is the encoding of
//frat-xor's cake_xlrup/BINARY_FORMAT.md.
class XLRUPFile: public Frat
{
public:
    XLRUPFile(vector<uint32_t>& _inter_to_outerMain, const bool _binary) :
        binary(_binary),
        inter_to_outer(_inter_to_outerMain)
    {}
    ~XLRUPFile() override { flush(); }

    bool enabled() override { return true; }
    void setFile(FILE* _file) override { file = _file; }
    void set_sumconflicts_ptr(uint64_t* _sum_conflicts) override { sum_conflicts = _sum_conflicts; }
    void set_sqlstats_ptr(SQLStats* _sql_stats) override { sql_stats = _sql_stats; }
    FILE* getFile() override { return file; }
    void flush() override { if (file) { write_buf(); fflush(file); } }
    bool something_delayed() override { return delayed_filled; }
    void forget_delay() override { delayed_filled = false; filling_delayed = false; }

    Frat& operator<<(const int32_t id) override
    {
        Step& st = cur();
        assert(id != 0);
        if (st.in_hints) st.hints.push_back(id);
        else { assert(!st.have_id); st.id = id; st.have_id = true; }
        return *this;
    }

    Frat& operator<<(const Lit l) override { cur().lits.push_back(l); return *this; }

    Frat& operator<<(const vector<Lit>& cl) override
    {
        for(const Lit l: cl) cur().lits.push_back(l);
        return *this;
    }

    Frat& operator<<(const Clause& cl) override
    {
        Step& st = cur();
        assert(!st.have_id);
        st.id = cl.stats.id;
        st.have_id = true;
        for(const Lit l: cl) st.lits.push_back(l);
        return *this;
    }

    Frat& operator<<(const Xor& x) override
    {
        Step& st = cur();
        assert(!st.have_id);
        st.id = x.xid;
        st.have_id = true;
        for(uint32_t i = 0; i < x.size(); i++) {
            Lit l = Lit(x[i], false);
            if (i == 0 && !x.rhs) l ^= true;
            st.lits.push_back(l);
        }
        return *this;
    }

    Frat& operator<<([[maybe_unused]] const char* str) override
    {
        #ifdef DEBUG_FRAT
        //the binary encoding has no comments
        if (file && !binary) { put_s("c "); put_s(str); flush(); }
        #endif
        return *this;
    }
    Frat& operator<<(const FratOutcome) override { return *this; }

    Frat& operator<<(const FratFlag flag) override
    {
        switch (flag) {
            case FratFlag::deldelay:
                assert(!delayed_filled);
                delayed = Step();
                delayed.kind = del;
                filling_delayed = true;
                break;
            case FratFlag::deldelayx:
                assert(!delayed_filled);
                delayed = Step();
                delayed.kind = delx;
                filling_delayed = true;
                break;
            case FratFlag::findelay:
                assert(delayed_filled);
                write_step(delayed);
                forget_delay();
                break;
            case FratFlag::fratchain:
                cur().in_hints = true;
                break;
            case FratFlag::fin:
                if (filling_delayed) {
                    filling_delayed = false;
                    delayed_filled = true;
                } else {
                    write_step(current);
                    if (sql_stats && is_addition(current.kind))
                        sql_stats->set_id_confl(current.id, *sum_conflicts);
                    current = Step();
                }
                break;
            default:
                //step-opening flags
                if (filling_delayed) break; //deldelay only streams id+lits
                current = Step();
                current.kind = flag;
                break;
        }
        return *this;
    }

private:
    struct Step {
        FratFlag kind = fin;
        int32_t id = 0;
        bool have_id = false;
        bool in_hints = false;
        vector<Lit> lits;
        vector<int32_t> hints;
    };

    Step& cur() { return filling_delayed ? delayed : current; }
    static bool is_addition(const FratFlag k)
    {
        return k == add || k == addx || k == implyclfromx || k == implyxfromcls;
    }

    void put_c(const char c) { buf.push_back(c); }
    void put_s(const char* s) { while (*s) buf.push_back(*s++); }
    void put_dec(const int64_t n)
    {
        char tmp[24];
        const auto r = std::to_chars(tmp, tmp + sizeof(tmp), n);
        buf.insert(buf.end(), tmp, r.ptr);
    }
    void put_varbyte(uint64_t u)
    {
        while (u > 0x7f) { buf.push_back((u & 0x7f) | 0x80); u >>= 7; }
        buf.push_back(u);
    }
    void put_id(const int32_t id)
    {
        if (binary) { assert(id > 0); put_varbyte(2*(uint64_t)id); }
        else put_dec(id);
    }
    void end_list() { if (binary) put_c(0); else put_s(" 0"); }
    void put_lits(const vector<Lit>& lits)
    {
        for(const Lit l: lits) {
            const uint64_t v = inter_to_outer[l.var()] + 1;
            if (binary) put_varbyte(2*v + l.sign());
            else { put_s(l.sign() ? " -" : " "); put_dec(v); }
        }
        end_list();
    }
    void put_hint(const int32_t id) { if (!binary) put_c(' '); put_id(id); }
    void put_hints(const vector<int32_t>& hints)
    {
        for(const auto& h: hints) put_hint(h);
        end_list();
    }
    void end_step()
    {
        if (!binary) put_c('\n');
        if (buf.size() > 1024*1024) write_buf();
    }
    void write_buf()
    {
        fwrite(buf.data(), 1, buf.size(), file);
        buf.clear();
    }

    void write_derived(const char* txt_tag, const char* bin_tag, const Step& st)
    {
        put_s(binary ? bin_tag : txt_tag);
        put_id(st.id);
        put_lits(st.lits);
        put_hints(st.hints);
    }

    void write_step(const Step& st)
    {
        if (done) return;
        switch (st.kind) {
            case FratFlag::add:
                write_derived("", "a", st);
                end_step();
                if (st.lits.empty()) done = true;
                break;
            case FratFlag::addx:
                write_derived("x ", "xa", st);
                //unit-clause hint list, never used
                if (binary) put_c(0);
                end_step();
                break;
            case FratFlag::implyclfromx:
                write_derived("i cx ", "xc", st);
                end_step();
                if (st.lits.empty()) done = true;
                break;
            case FratFlag::implyxfromcls:
                write_derived("i x ", "xi", st);
                end_step();
                break;
            case FratFlag::del:
            case FratFlag::weakencl:
                //the text line leads with an id the checker ignores
                if (!binary) { put_dec(st.id); put_c(' '); }
                put_c('d');
                put_hint(st.id);
                end_list();
                end_step();
                break;
            case FratFlag::delx:
                put_s(binary ? "xd" : "x d");
                put_hint(st.id);
                end_list();
                end_step();
                break;
            case FratFlag::origclx:
                put_s(binary ? "xo" : "o x ");
                put_id(st.id);
                put_lits(st.lits);
                end_step();
                break;
            //dropped: inputs are numbered by position, no finalization
            case FratFlag::origcl:
            case FratFlag::finalcl:
            case FratFlag::finalx:
            case FratFlag::restorecl:
            case FratFlag::assump:
            case FratFlag::unsatcore:
            case FratFlag::modelF:
            case FratFlag::fin:
                break;
            default:
                release_assert(false && "step not supported in XLRUP mode");
        }
        //the proof is complete, let API users read it without deleting the solver
        if (done) flush();
    }

    const bool binary;
    vector<char> buf;
    uint64_t* sum_conflicts = nullptr;
    SQLStats* sql_stats = nullptr;
    FILE* file = nullptr;
    Step current;
    Step delayed;
    bool filling_delayed = false;
    bool delayed_filled = false;
    bool done = false;
    vector<uint32_t>& inter_to_outer;
};

}
