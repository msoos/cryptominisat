/*****************************************************************************
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson

MIT licence
*/

#ifndef STREAMBUFFER_H
#define STREAMBUFFER_H

#define CHUNK_LIMIT 1048576

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

class StreamBuffer
{
    #ifdef DISABLE_ZLIB
    FILE *  in;
    #else
    gzFile  in;
    #endif // DISABLE_ZLIB
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
            #ifdef DISABLE_ZLIB
            size = fread(buf, 1, sizeof(buf), in);
            #else
            size = gzread(in, buf, sizeof(buf));
            #endif // DISABLE_ZLIB
        }
    }

public:
    #ifdef DISABLE_ZLIB
    StreamBuffer(FILE * i) : in(i), pos(0), size(0) {
    #else
    StreamBuffer(gzFile i) : in(i), pos(0), size(0) {
    #endif // DISABLE_ZLIB
        assureLookahead();
    }

    int  operator *  () {
        return (pos >= size) ? EOF : buf[pos];
    }
    void operator ++ () {
        pos++;
        assureLookahead();
    }
};

#endif //STREAMBUFFER_H
