#ifndef SOMETIME_H
#define SOMETIME_H

#include<cstdlib>
#include <sys/time.h> // To seed random generator

using namespace std;

extern bool diffTimes(timeval& ret, const timeval &tLater, const timeval &tEarlier);

class CStepTime
{
    static int timeVal;

public:

    static void makeStart()
    {
        timeVal = 0;
    }

    static int getTime()
    {
        return timeVal;
    }

    static void stepTime()
    {
        timeVal++;
    }
};



class CStopWatch
{
    timeval timeStart;
    timeval timeStop;

    long int timeBound;

public:

    CStopWatch() {}
    ~CStopWatch() {}

    bool timeBoundBroken()
    {
        timeval actTime;
        gettimeofday(&actTime,NULL);

        return actTime.tv_sec - timeStart.tv_sec > timeBound;
    }

    bool markStartTime()
    {
        return gettimeofday(&timeStart,NULL) == 0;
    }

    bool markStopTime()
    {
        return gettimeofday(&timeStop,NULL) == 0;
    }


    void setTimeBound(long int seconds)
    {
        timeBound = seconds;
    }

    long int getTimeBound()
    {
        return timeBound;
    }

    double getElapsedTime()
    {
        timeval r;
        double retT;
        diffTimes(r,timeStop, timeStart);

        retT = r.tv_usec;
        retT /= 1000000.0;
        retT += (double)r.tv_sec;
        return retT;
    }

    unsigned int getElapsedusecs()
    {
        unsigned int retT;
        timeval r;

        diffTimes(r,timeStop, timeStart);

        retT = r.tv_usec;

        retT += r.tv_sec * 1000000;
        return retT;
    }
};

#endif
