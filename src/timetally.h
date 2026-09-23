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

#include <map>
#include <optional>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "time_mem.h"

namespace CMSat {

// Exclusive time: while a nested scope runs, its parent does not accrue, so
// the entries never overlap and sum to the time spent inside outermost scopes.
class TimeTally {
public:
    void push(const std::string& name) {
        const double now = cpu_time();
        if (!stack.empty()) data[stack.back()].time += now - last;
        stack.push_back(name);
        last = now;
    }

    void pop() {
        const double now = cpu_time();
        auto& d = data[stack.back()];
        d.time += now - last;
        d.calls++;
        stack.pop_back();
        last = now;
    }

    void print(const std::string& prefix, const double total) const {
        std::vector<std::pair<std::string, Data>> v(data.begin(), data.end());
        std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
            return a.second.time > b.second.time; });
        double sum = 0;
        for(const auto& x: v) sum += x.second.time;
        v.push_back({"other", Data{total - sum, 0}});

        std::cout << prefix << "------- TIME BREAKDOWN -------" << std::endl;
        for(const auto& x: v) {
            std::cout << prefix << std::left << std::setw(28) << x.first
                << std::right << std::fixed << std::setprecision(2)
                << std::setw(10) << x.second.time << " s "
                << std::setw(6) << (total > 0 ? x.second.time/total*100.0 : 0.0) << " %";
            if (x.second.calls) std::cout << std::setw(9) << x.second.calls << " calls";
            std::cout << std::endl;
        }
    }

private:
    struct Data {
        double time = 0;
        uint64_t calls = 0;
    };
    std::map<std::string, Data> data;
    std::vector<std::string> stack;
    double last = 0;
};

struct TimeScope {
    TimeScope(TimeTally& _t, const std::string& name): t(_t) { t.push(name); }
    ~TimeScope() { t.pop(); }
    TimeScope(const TimeScope&) = delete;
    TimeScope& operator=(const TimeScope&) = delete;
    TimeTally& t;
};

}
