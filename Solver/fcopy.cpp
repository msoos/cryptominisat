#include <stdio.h>
#include <string.h>

#include "fcopy.h"

#define BUFSZ 16000

int FileCopy ( const char *src, const char *dst )
{
    char            *buf;
    FILE            *fi;
    FILE            *fo;
    unsigned        amount;
    unsigned        written;
    int             result;

    buf = new char[BUFSZ];

    fi = fopen( src, "rb" );
    fo = fopen( dst, "wb" );

    result = COPY_OK;
    if  ((fi == NULL) || (fo == NULL) ) {
        result = COPY_ERROR;
        if (fi != NULL) fclose(fi);
        if (fo != NULL) fclose(fo);
    }

    if (result == COPY_OK) {
        do {
            amount = fread( buf, sizeof(char), BUFSZ, fi );
            if (amount) {
                written = fwrite( buf, sizeof(char), amount, fo );
                if (written != amount)
                    result = COPY_ERROR; // out of disk space or some other disk err?
            }
        } // when amount read is < BUFSZ, copy
        while ((result == COPY_OK) && (amount == BUFSZ));
        fclose(fi);
        fclose(fo);
    }

    delete[] buf;

    return(result);
}

