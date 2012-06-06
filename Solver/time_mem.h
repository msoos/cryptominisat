/*
This file is part of CryptoMiniSat2.

CryptoMiniSat2 is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

CryptoMiniSat2 is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with CryptoMiniSat2.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TIME_MEM_H
#define TIME_MEM_H

#ifdef _MSC_VER
#include <msvc/stdint.h>
#else
#include <stdint.h>
#endif //_MSC_VER

#if defined (_MSC_VER) || defined(CROSS_COMPILE)
#include <ctime>

static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
#else //_MSC_VER
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline double cpuTime(void)
{
    struct rusage ru;
    #ifdef RUSAGE_THREAD
    getrusage(RUSAGE_THREAD, &ru);
    #else
    getrusage(RUSAGE_SELF, &ru);
    #endif
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000.0;
}

static inline double cpuTimeTotal(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000.0;
}
#endif //CROSS_COMPILE


#if defined(__linux__)
#include <stdio.h>
static inline int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) return 0;
    int     value;

    int rvalue= 1;
    for (; (field >= 0) && (rvalue == 1); field--)
        rvalue = fscanf(in, "%d", &value);

    fclose(in);
    return value;
}
static inline uint64_t memUsed()
{
    return (uint64_t)memReadStat(0) * (uint64_t)getpagesize();
}


#elif defined(__FreeBSD__)
static inline uint64_t memUsed(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return ru.ru_maxrss*1024;
}


#else
static inline uint64_t memUsed()
{
    return 0;
}
#endif

#endif //TIME_MEM_H
