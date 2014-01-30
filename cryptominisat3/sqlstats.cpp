#include "sqlstats.h"
using namespace CMSat;


#ifndef _MSC_VER
#include <fcntl.h>
void SQLStats::getRandomID()
{
    //Generate random ID for SQL
    int randomData = open("/dev/urandom", O_RDONLY);
    if (randomData == -1) {
        cout << "Error reading from /dev/urandom !" << endl;
        exit(-1);
    }
    ssize_t ret = read(randomData, &runID, sizeof(runID));

    //Can only be <8 bytes long, some PHP-related limit
    //Make it 6-byte long then (good chance to collide after 2^24 entries)
    runID &= 0xffffffULL;

    if (ret != sizeof(runID)) {
        cout << "Couldn't read from /dev/urandom!" << endl;
        exit(-1);
    }
    close(randomData);
}
#else
#include <ctime>
void SQLStats::getRandomID()
{
    srand((unsigned) time(NULL));
    int numbers[10];
    runID = rand();
}
#endif
