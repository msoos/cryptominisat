/******************************************************************************************[Main.C]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

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
