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

#ifndef TIME_MEM_H
#define TIME_MEM_H
#include "constants.h"
#include <cassert>
#include <time.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <signal.h>

// note: MinGW64 defines both __MINGW32__ and __MINGW64__
#if defined (_MSC_VER) || defined (__MINGW32__) || defined(_WIN32) || defined(EMSCRIPTEN)
#include <ctime>
static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
static inline double cpuTimeTotal(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
static inline double realTimeSec() {
    return 0;
}

static inline double real_time_sec() {
    return 0;
}


#else //Linux or POSIX
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline long realTimeMicros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

static inline double real_time_sec() {
    return (double) realTimeMicros() / 1000000;
}

static inline double cpuTime(void)
{
    struct rusage ru;
    #ifdef RUSAGE_THREAD
    int ret = getrusage(RUSAGE_THREAD, &ru);
    #else
    int ret = getrusage(RUSAGE_SELF, &ru);
    #endif
    assert(ret == 0);

    return (double)ru.ru_utime.tv_sec + ((double)ru.ru_utime.tv_usec / 1000000.0);
}

static inline double cpuTimeTotal(void)
{
    struct rusage ru;
    int ret = getrusage(RUSAGE_SELF, &ru);
    assert(ret == 0);

    return (double)ru.ru_utime.tv_sec + ((double)ru.ru_utime.tv_usec / 1000000.0);
}

#endif

#if defined(__linux__)
// process_mem_usage(double &, double &) - takes two doubles by reference,
// attempts to read the system-dependent data for a process' virtual memory
// size and resident set size, and return the results in KB.
//
// On failure, returns 0.0, 0.0
static inline uint64_t memUsedTotal(double& vm_usage, std::string* max_mem_usage = NULL)
{
   //double& vm_usage
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid; //                        The process ID.
   string comm;  //The  filename  of the executable, in parentheses.
   string state; //One of the following characters, indicating process (see man stat(2))
   string ppid; //The PID of the parent of this process.
   string pgrp; //The process group ID of the process.
   string session; //The session ID of the process.

   //The  controlling  terminal of the process.  (The minor device number is contained in the combina‐
   //tion of bits 31 to 20 and 7 to 0; the major device number is in bits 15 to 8.)
   string tty_nr;

   //The ID of the foreground process group of the controlling terminal of the process.
   string tpgid;


   string flags;
   string minflt;
   string cminflt;
   string majflt;
   string cmajflt;
   string utime;
   string stime;
   string cutime;
   string cstime;
   string priority;
   string nice;

   //Number of threads in this process (since Linux 2.6).  Before kernel  2.6,  this  field  was  hard
   //coded to 0 as a placeholder for an earlier removed field.
   string num_threads;

   string itrealvalue;
   string starttime;

   /**** the two fields we want *****/
   unsigned long vsize; //Virtual memory size in bytes.

   //Resident Set Size: number of pages the process has in real memory.  This is just the pages  which
   //count toward text, data, or stack space.  This does not include pages which have not been demand-
   //loaded in, or which are swapped out.
   long rss;
   /**** the two fields we want *****/

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> num_threads >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE); // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize;
   double resident_set = (double)rss * (double)page_size_kb;

   if (max_mem_usage != NULL) {
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
               tp.erase(tp.begin(),
                        std::find_if(tp.begin(), tp.end(), std::bind1st(std::not_equal_to<char>(), '\t')));
               tp.erase(tp.begin(),
                        std::find_if(tp.begin(), tp.end(), std::bind1st(std::not_equal_to<char>(), ' ')));
               *max_mem_usage = tp;
           }
      }
   }

   return resident_set;
}
#elif defined(__FreeBSD__)
#include <sys/types.h>
inline uint64_t memUsedTotal(double& vm_usage, std::string* max_mem_usage = NULL)
{
    vm_usage = 0;

    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024;
}
#else //Windows
static inline size_t memUsedTotal(double& vm_usage, std::string* max_mem_usage = NULL)
{
    vm_usage = 0;
    return 0;
}
#endif

#endif //TIME_MEM_H
