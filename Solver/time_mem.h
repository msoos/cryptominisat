#ifndef __TIME_MEM_H__
#define __TIME_MEM_H__

#ifdef _MSC_VER
#include <ctime>

static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
#else
#ifdef CROSS_COMPILE
#include <ctime>

static inline double cpuTime(void)
{
    return (double)clock() / CLOCKS_PER_SEC;
}
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

static inline double cpuTime(void)
{
    struct rusage ru;
    getrusage(RUSAGE_SELF, &ru);
    return (double)ru.ru_utime.tv_sec + (double)ru.ru_utime.tv_usec / 1000000;
}
#endif
#endif


#if defined(__linux__)
static inline int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) return 0;
    int     value;
    for (; field >= 0; field--)
        fscanf(in, "%d", &value);
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

#endif //__TIME_MEM_H__