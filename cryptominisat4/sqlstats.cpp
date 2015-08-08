/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2015, Mate Soos. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.0 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301  USA
*/

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
        std::exit(-1);
    }
    ssize_t ret = read(randomData, &runID, sizeof(runID));

    //Can only be <8 bytes long, some PHP-related limit
    //Make it 6-byte long then (good chance to collide after 2^24 entries)
    runID &= 0xffffffULL;

    if (ret != sizeof(runID)) {
        cout << "Couldn't read from /dev/urandom!" << endl;
        std::exit(-1);
    }
    close(randomData);

    if (runID == 0)
        runID = 1;
}
#else
#include <ctime>
void SQLStats::getRandomID()
{
    srand((unsigned) time(NULL));
    runID = rand();
    if (runID == 0) {
        runID = 1;
    }
}
#endif
