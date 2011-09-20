#include "Basics.h"


bool CSolverConf::analyzeConflicts = true;
bool CSolverConf::doNonChronBackTracking = true;

bool CSolverConf::allowComponentCaching = true;
bool CSolverConf::allowImplicitBCP = true;

bool CSolverConf::allowPreProcessing = true;

bool CSolverConf::quietMode = false;

unsigned int CSolverConf::secsTimeBound = 100000;

unsigned int CSolverConf::maxCacheSize = 0;

int CSolverConf::nodeCount = 0;


char TriValuetoChar(TriValue v)
{
    switch (v)
    {
    case W:
        return '1';
    case F:
        return '0';
    case X:
        return '.';
    };
    return '.';
}
