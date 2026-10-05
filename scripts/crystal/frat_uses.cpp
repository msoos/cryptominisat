// Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2
// of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.

// The proof pass of fix_up_frat.py: trims the FRAT proof from the empty
// clause and writes every use of a tracked clause (or of a descendant of
// one) by a step of the trimmed proof.
//
// usage: frat_uses FRAT CONFL TRACKED OUT
//   CONFL:   int64 pairs (ID, conflicts), set_id_confl
//   TRACKED: int64 pairs (ID, tracked clause it counts as)
//   OUT:     records (int64 clauseID, int64 used_at, double weight)

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using std::vector;

static const uint8_t* dat;
static size_t len;
static bool binary;
static vector<int64_t> offsets; // of each 'a' step, in proof order
static vector<int64_t> ids;
static vector<int64_t> pos_of_id;
static int64_t max_id = 0;
static int64_t empty_cl = -1;

struct Tracked {
    int64_t id;      // the tracked clause this ID counts as
    double weight;
    int64_t confl;   // -1: an original parent
};

static double now() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

[[noreturn]] static void die(const char* msg) {
    printf("ERROR: %s\n", msg);
    exit(255);
}

static inline uint64_t unum(size_t& pos) {
    uint64_t res = 0;
    int mul = 0;
    while (true) {
        if (pos >= len) die("truncated FRAT file");
        const uint8_t c = dat[pos++];
        res |= (uint64_t)(c & 0x7f) << mul;
        mul += 7;
        if ((c & 0x80) == 0) return res;
    }
}

static inline void skip_ivec(size_t& pos) {
    while (true) {
        if (pos >= len) die("truncated FRAT file");
        if (dat[pos++] == 0) return;
    }
}

static void index_binary() {
    size_t pos = 0;
    while (pos < len) {
        const uint8_t k = dat[pos++];
        if (k == 'a' || k == 'o' || k == 'd' || k == 'f' || k == 'i') {
            const size_t start = pos - 1;
            const int64_t cid = unum(pos);
            if (cid == 0) continue; // step header, e.g. 'i 0' before 'x'
            if (k == 'a') {
                offsets.push_back(start);
                ids.push_back(cid);
                if (pos < len && dat[pos] == 0) empty_cl = cid;
            }
            skip_ivec(pos);
        } else if (k == 'l') {
            skip_ivec(pos);
        } else if (k == 'r') {
            while (true) {
                if (unum(pos) == 0) break;
                unum(pos);
            }
        } else if (k == 't') {
            unum(pos);
            unum(pos);
        } else if (k == 'x') {
            unum(pos);
            skip_ivec(pos);
        } else if (k == 'c') {
            skip_ivec(pos);
        } else {
            printf("ERROR: unknown FRAT step '%c' at byte %zu. Use --xor 0 (no XOR/BNN steps)\n", k, pos - 1);
            exit(255);
        }
    }
}

static void index_ascii() {
    size_t pos = 0;
    while (pos < len) {
        const uint8_t* nl = (const uint8_t*)memchr(dat + pos, '\n', len - pos);
        const size_t end = nl ? (size_t)(nl - dat) : len;
        if (dat[pos] == 'a') {
            size_t p = pos + 2;
            int64_t cid = 0;
            while (p < end && dat[p] != ' ') cid = cid * 10 + (dat[p++] - '0');
            offsets.push_back(pos);
            ids.push_back(cid);
            if (p + 2 < len && dat[p + 1] == '0' && dat[p + 2] == ' ') empty_cl = cid;
        }
        pos = end + 1;
    }
}

// hint chain of the i-th add step
static void hints(size_t i, vector<int64_t>& ret) {
    ret.clear();
    size_t pos = offsets[i];
    if (binary) {
        pos++;
        unum(pos);
        skip_ivec(pos);
        if (pos >= len || dat[pos] != 'l') die("add step without hints, need a FRAT with full chains");
        pos++;
        while (true) {
            const uint64_t u = unum(pos);
            if (u == 0) return;
            ret.push_back((u & 1) ? -(int64_t)(u >> 1) : (int64_t)(u >> 1));
        }
    }
    const uint8_t* nl = (const uint8_t*)memchr(dat + pos, '\n', len - pos);
    const size_t end = nl ? (size_t)(nl - dat) : len;
    size_t p = pos;
    bool found = false;
    for (; p + 2 < end; p++) {
        if (dat[p] == ' ' && dat[p + 1] == 'l' && dat[p + 2] == ' ') { found = true; break; }
    }
    if (!found) die("add step without hints, need a FRAT with full chains");
    p += 3;
    while (p < end) {
        while (p < end && dat[p] == ' ') p++;
        if (p >= end) break;
        bool neg = false;
        if (dat[p] == '-') { neg = true; p++; }
        int64_t v = 0;
        while (p < end && dat[p] >= '0' && dat[p] <= '9') v = v * 10 + (dat[p++] - '0');
        ret.push_back(neg ? -v : v);
    }
    if (ret.empty() || ret.back() != 0) die("hint chain does not end in 0");
    ret.pop_back();
}

static vector<int64_t> read_pairs(const char* fname) {
    FILE* f = fopen(fname, "rb");
    if (!f) { printf("ERROR: cannot open %s\n", fname); exit(255); }
    fseek(f, 0, SEEK_END);
    const long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    vector<int64_t> ret(sz / 8);
    if (sz > 0 && fread(ret.data(), 8, ret.size(), f) != ret.size()) die("short read");
    fclose(f);
    if (ret.size() % 2) die("odd number of values in a pair file");
    return ret;
}

int main(int argc, char** argv) {
    if (argc != 5) {
        printf("usage: %s FRAT CONFL TRACKED OUT\n", argv[0]);
        return 255;
    }
    const int fd = open(argv[1], O_RDONLY);
    if (fd < 0) die("cannot open the FRAT file");
    struct stat st;
    fstat(fd, &st);
    len = st.st_size;
    if (len < 2) die("empty FRAT file");
    dat = (const uint8_t*)mmap(nullptr, len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (dat == MAP_FAILED) die("cannot mmap the FRAT file");
    binary = dat[1] != ' ' && dat[1] != '\n';
    printf("FRAT file is %s\n", binary ? "binary" : "ascii");

    double t = now();
    if (binary) index_binary(); else index_ascii();
    printf("Indexed %zu add steps T: %-3.2f s\n", offsets.size(), now() - t);
    if (empty_cl == -1) die("no empty clause in the proof. Was the instance UNSAT?");
    for (const int64_t id : ids) if (id > max_id) max_id = id;
    pos_of_id.assign(max_id + 1, -1);
    for (size_t i = 0; i < ids.size(); i++) pos_of_id[ids[i]] = i;

    // backward: which add steps derive the empty clause
    t = now();
    vector<char> marked(offsets.size(), 0);
    vector<int64_t> todo, h;
    todo.push_back(pos_of_id[empty_cl]);
    marked[todo[0]] = 1;
    size_t num_marked = 1;
    while (!todo.empty()) {
        const int64_t i = todo.back();
        todo.pop_back();
        hints(i, h);
        for (const int64_t x : h) {
            if (x < 0 || x > max_id) continue; // RAT hints are negative
            const int64_t j = pos_of_id[x];
            if (j >= 0 && !marked[j]) { // original clauses are not add steps
                marked[j] = 1;
                num_marked++;
                todo.push_back(j);
            }
        }
    }
    printf("Marked %zu of %zu add steps as part of the proof T: %-3.2f s\n", num_marked, offsets.size(), now() - t);

    vector<int64_t> confl_of;
    {
        const vector<int64_t> p = read_pairs(argv[2]);
        int64_t mx = -1;
        for (size_t i = 0; i < p.size(); i += 2) {
            if (p[i] < 0) die("negative ID in set_id_confl");
            if (p[i] > mx) mx = p[i];
        }
        confl_of.assign(mx + 1, -1);
        for (size_t i = 0; i < p.size(); i += 2) confl_of[p[i]] = p[i + 1];
    }
    std::unordered_map<int64_t, Tracked> tracked;
    {
        const vector<int64_t> p = read_pairs(argv[3]);
        tracked.reserve(p.size());
        for (size_t i = 0; i < p.size(); i += 2) tracked[p[i]] = Tracked{p[i + 1], 1.0, -1};
    }

    // forward: the uses, and the descendants of tracked clauses
    t = now();
    FILE* out = fopen(argv[4], "wb");
    if (!out) die("cannot open the output file");
    uint64_t num_steps = 0, no_confl = 0, num_uses = 0, children_set = 0;
    for (size_t i = 0; i < offsets.size(); i++) {
        if (!marked[i]) continue;
        const int64_t res = ids[i];
        num_steps++;
        bool tracked_already = tracked.count(res);
        if (res >= (int64_t)confl_of.size() || confl_of[res] == -1) {
            no_confl++;
            continue;
        }
        const int64_t confl = confl_of[res];
        hints(i, h);
        for (const int64_t x : h) {
            const auto it = tracked.find(x);
            if (it == tracked.end()) continue;
            const Tracked d = it->second;
            const int64_t used_at = d.confl == -1 ? confl : d.confl;
            fwrite(&d.id, 8, 1, out);
            fwrite(&used_at, 8, 1, out);
            fwrite(&d.weight, 8, 1, out);
            num_uses++;

            // the resolvent becomes a child of ONE tracked clause
            if (tracked_already) continue;
            const double val = 0.5 * d.weight;
            if (val <= 0.05) continue;
            tracked[res] = Tracked{d.id, val, used_at};
            tracked_already = true;
            children_set++;
        }
    }
    if (fclose(out) != 0) die("cannot write the output file");
    printf("Forward pass T: %-3.2f s\n", now() - t);
    printf("Proof steps in proof:   %10llu\n", (unsigned long long)num_steps);
    printf("Steps w/o conflict no.: %10llu\n", (unsigned long long)no_confl);
    printf("Total num uses:         %10llu\n", (unsigned long long)num_uses);
    printf("Children set:           %10llu\n", (unsigned long long)children_set);
    if (num_steps == 0) die("no proof steps found");
    return 0;
}
