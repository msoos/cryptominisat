/*********************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
CryptoMiniSat -- Copyright (C) 2009-2020 Authors of CryptoMiniSat, see AUTHORS file

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
***********************************************************/

#pragma once

#include <cassert>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <cstdint>

// note: MinGW64 defines both __MINGW32__ and __MINGW64__
#if defined (_MSC_VER) || defined (__MINGW32__) || defined(_WIN32)
#include <ctime>
#include <windows.h>
static inline double cpu_time(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static inline double cpuTimeTotal(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}

static inline double real_time_sec(void)
{
    return (double)GetTickCount64() / 1000.0;
}

#else //Linux or POSIX
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>

static inline double cpu_time(void)
{
    struct rusage ru;
    [[maybe_unused]] int ret = getrusage(RUSAGE_SELF, &ru);
    assert(ret == 0);

    return (double)ru.ru_utime.tv_sec + ((double)ru.ru_utime.tv_usec / 1000000.0);
}

// Returns total CPU time used by all threads in the process (user + system).
static inline double cpuTimeTotal(void)
{
    struct timespec ts;
    [[maybe_unused]] int ret = clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    assert(ret == 0);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static inline double real_time_sec(void)
{
    struct timespec ts;
    [[maybe_unused]] int ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(ret == 0);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

#endif

#if defined(__linux__)
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in KB.
//
// On failure, returns 0.0, 0.0
static inline uint64_t mem_used(double& vm_usage, std::string* max_mem_usage = nullptr)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage = 0.0;

   // 'file' stat seems to give the most reliable results
   ifstream stat_stream("/proc/self/stat", ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   string pid, comm, state, ppid, pgrp, session, tty_nr, tpgid;
   string flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string num_threads, itrealvalue, starttime;

   unsigned long vsize; //Virtual memory size in bytes.
   long rss; //Resident Set Size: number of pages the process has in real memory.

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> num_threads >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE); // in case x86-64 is configured to use 2MB pages
   vm_usage     = (double)vsize;
   double resident_set = (double)rss * (double)page_size_kb;

   if (max_mem_usage != nullptr) {
       //NOTE: we could query the MAXIMUM resident size using
       //   /proc/self/status
       //   as it contains: * VmHWM: Peak resident set size ("high water mark").
       //   but we'd need to parse it, etc.
       //   see man(5) proc for details
       //   This note is related to issue #629 in CryptoMiniSat
       ifstream stat_stream2("/proc/self/status",ios_base::in);
       string tp;
       while(getline(stat_stream2, tp)){
           if (tp.size() > 7 && tp.find("VmHWM:") != std::string::npos) {
               tp.erase(0, 7);
               const auto first_non_ws = tp.find_first_not_of(" \t");
               if (first_non_ws != std::string::npos) {
                   tp.erase(0, first_non_ws);
               }
               *max_mem_usage = tp;
           }
      }
   }

   return (uint64_t)resident_set;
}

static inline uint64_t rss_mem_used(void)
{
    double vm_unused;
    return mem_used(vm_unused);
}

static inline uint64_t memUsedTotal(void)
{
    using std::ios_base;
    using std::ifstream;
    using std::string;

    ifstream stat_stream("/proc/self/stat", ios_base::in);
    string pid, comm, state, ppid, pgrp, session, tty_nr, tpgid;
    string flags, minflt, cminflt, majflt, cmajflt;
    string utime, stime, cutime, cstime, priority, nice;
    string num_threads, itrealvalue, starttime;
    unsigned long vsize;
    long rss;
    stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
                >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
                >> utime >> stime >> cutime >> cstime >> priority >> nice
                >> num_threads >> itrealvalue >> starttime >> vsize >> rss;
    stat_stream.close();
    return (uint64_t)vsize;
}

#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/user.h>
inline uint64_t mem_used(double& vm_usage, [[maybe_unused]] std::string* max_mem_usage = nullptr)
{
    vm_usage = 0;

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024;
}

static inline uint64_t rss_mem_used(void)
{
    double vm_unused;
    return mem_used(vm_unused);
}

static inline uint64_t memUsedTotal(void)
{
    struct kinfo_proc kp;
    size_t len = sizeof(kp);
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)getpid()};
    if (sysctl(mib, 4, &kp, &len, nullptr, 0) == 0)
        return (uint64_t)kp.ki_size;
    return 0;
}

#else //Windows
static inline size_t mem_used(double& vm_usage, std::string* max_mem_usage = nullptr)
{
    vm_usage = 0;
    return 0;
}

static inline uint64_t rss_mem_used(void)
{
    return 0;
}

static inline uint64_t memUsedTotal(void)
{
    return 0;
}
#endif
