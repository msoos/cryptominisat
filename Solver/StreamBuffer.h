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

#ifndef STREAMBUFFER_H
#define STREAMBUFFER_H

#define CHUNK_LIMIT 1048576
#include "constants.h"

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB


namespace CMSat
{

class StreamBuffer
{
    #ifndef DISABLE_ZLIB
    gzFile  gzin;
    void assureLookaheadZip() {
        if (pos >= size) {
            pos  = 0;
            #ifdef VERBOSE_DEBUG
            printf("buf = %08X\n", buf);
            printf("sizeof(buf) = %u\n", sizeof(buf));
            #endif //VERBOSE_DEBUG
            size = gzread(gzin, buf, sizeof(buf));
        }
    }
    #endif

    FILE *  in;
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
            #ifdef VERBOSE_DEBUG
            printf("buf = %08X\n", buf);
            printf("sizeof(buf) = %u\n", sizeof(buf));
            #endif //VERBOSE_DEBUG
            size = fread(buf, 1, sizeof(buf), in);
        }
    }

public:
    #ifndef DISABLE_ZLIB
    StreamBuffer(gzFile i) :
        gzin(i)
        , in(NULL)
        , pos(0)
        , size(0)
    {
        assureLookaheadZip();
    }
    #endif

    StreamBuffer(FILE * i) :
        #ifndef DISABLE_ZLIB
        gzin(NULL)
        , in(i)
        #else
        in(i)
        #endif
        , pos(0)
        , size(0)
    {
        assureLookahead();
    }

    int  operator *  () {
        return (pos >= size) ? EOF : buf[pos];
    }
    void operator ++ () {
        pos++;
        #ifndef DISABLE_ZLIB
        if (in == NULL)
            assureLookaheadZip();
        else
            assureLookahead();
        #else
        assureLookahead();
        #endif
    }
};

}

#endif //STREAMBUFFER_H
