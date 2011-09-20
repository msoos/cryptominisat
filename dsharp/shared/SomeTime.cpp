#include "SomeTime.h"

int CStepTime::timeVal = 0;



bool diffTimes(timeval& ret, const timeval &tLater, const timeval &tEarlier)
{
    long int ad = 0;
    long int bd = 0;

    if (tLater.tv_usec < tEarlier.tv_usec)
    {
        ad = 1;
        bd = 1000000;
    }
    ret.tv_sec = tLater.tv_sec - ad - tEarlier.tv_sec;
    ret.tv_usec = tLater.tv_usec + bd - tEarlier.tv_usec;
    return true;
}
