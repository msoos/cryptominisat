/*
 * CryptoMiniSat
 *
 * Copyright (c) 2009-2011, Mate Soos and collaborators. All rights reserved.
 * Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
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

#ifndef STREAMBUFFER_H
#define STREAMBUFFER_H

#define CHUNK_LIMIT 1048576

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif // DISABLE_ZLIB

class StreamBuffer
{
    #ifndef DISABLE_ZLIB
    gzFile  in;
    void assureLookahead() {
        if (pos >= size) {
            pos  = 0;
            #ifdef VERBOSE_DEBUG
            printf("buf = %08X\n", buf);
            printf("sizeof(buf) = %u\n", sizeof(buf));
            #endif //VERBOSE_DEBUG
            size = gzread(in, buf, sizeof(buf));
        }
    }
    #else
    FILE *  in;
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
    #endif
    char    buf[CHUNK_LIMIT];
    int     pos;
    int     size;

public:
    #ifndef DISABLE_ZLIB
    StreamBuffer(gzFile i) : in(i), pos(0), size(0) {
        assureLookahead();
    }
    #else
    StreamBuffer(FILE * i) : in(i), pos(0), size(0) {
        assureLookahead();
    }
    #endif

    int  operator *  () {
        return (pos >= size) ? EOF : buf[pos];
    }
    void operator ++ () {
        pos++;
        assureLookahead();
    }
};

#endif //STREAMBUFFER_H
