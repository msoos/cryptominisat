#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include "Global.h"

//#include <libiberty.h>


#if 0 //(__GNUC__ > 2) || (__GNUC__ >= 2 && __GNUC_MINOR__ > 95)
char* nsprintf(const char* format, ...)
{
    char*       buf;
    va_list     args;
    va_start(args, format);
    vasprintf(&buf, format, args);
    va_end(args);
    return buf;
}
#else
char* nsprintf(const char* format, ...)
{
    char     buf[1024];
    unsigned chars_written;
    va_list args;

    va_start(args, format);
    chars_written = vsprintf(buf, format, args);
    assert(chars_written < sizeof(buf));

    va_end(args);
    return strdup(buf);
}
#endif


char* strnum(int num)
{
    #define MAX 11
    char    buf[11];
    int     i;
    bool    sign;
    char*   ret;

    if (num == 0){
        ret = xmalloc<char>(2);
        ret[0] = '0';
        ret[1] = '\0';
        return ret;
    }else{
        if (num < 0)
            num = -num,
            sign = true;
        else
            sign = false;
        i = MAX-1;

        while(num != 0)
            buf[i] = '0' + (num % 10),
            num /= 10,
            i--;

        if (sign)
            buf[i] = '-',
            i--;

        ret = xmalloc<char>(MAX-i);
        memcpy(ret, &buf[i+1], MAX-i-1);
        ret[MAX-i-1] = '\0';
        return ret;
    }
}


// Call with '&text' repeatedly to iterate over all tokens. Will change the value of ('char*') 'text' as well as modify the string.
char* strtokr(char** src, const char* seps)
{
    if (*src == NULL)
        return NULL;
    else{
        while (**src != 0 && strchr(seps, **src) != NULL) (*src)++;
        if (**src == 0){ *src = NULL; return NULL; }
        char* ret = *src;
        do (*src)++; while (**src != 0 && strchr(seps, **src) == NULL);
        if (**src == 0) *src = NULL;
        else            **src = 0, (*src)++;
        return ret;
    }
}


// Returns -1 if file does not exist.
int fileSize(const char* filename)
{
    FILE*   in;
    int     size;

    in = fopen(filename, "rb");
    if (in == NULL)
        return -1;

    fseek(in, 0, SEEK_END);
    size = (int)ftell(in);
    fclose(in);

    return size;
}


char* readFile(const char* filename, int* size_)
{
    int     size = fileSize(filename);
    if (size_ != NULL)
        *size_ = size;

    if (size == -1)
        return NULL;
    else{
        FILE*   in  = fopen(filename, "rb"); assert(in != NULL);
        char*   ret = xmalloc<char>(size+1);
        fread(ret, 1, size, in);
        fclose(in);
        ret[size] = '\0';
        return ret;
    }
}


#define BLOCK_SIZE 65536
/*
// If you need the code for istreams:
//
char* readFile(ifstream& in, int* size_)
{
    char*   data = xmalloc<char>(BLOCK_SIZE);
    int     cap  = BLOCK_SIZE;
    int     size = 0;

    while (in.good()){
        if (size == cap){
            cap *= 2;
            data = xrealloc(data, cap); }
        in.read(&data[size], BLOCK_SIZE);
        size += in.gcount();
    }
    data = xrealloc(data, size+1);
    data[size] = '\0';

    if (size_ != NULL)
        *size_ = size;

    return data;
}
*/

char* readFile(FILE* in, int* size_)
{
    char*   data = xmalloc<char>(BLOCK_SIZE);
    int     cap  = BLOCK_SIZE;
    int     size = 0;

    while (!feof(in)){
        if (size == cap){
            cap *= 2;
            data = xrealloc(data, cap); }
        size += fread(&data[size], 1, BLOCK_SIZE, in);
    }
    data = xrealloc(data, size+1);
    data[size] = '\0';

    if (size_ != NULL)
        *size_ = size;

    return data;
}


// Read the file named by 'source'. Special name '-' means standard input. A name starting with '^'
// means rest of string is the "file". Several sources can be supplied by separating them with '#'
// In all cases, the returned string should be freed by caller.
//
// If 'output_error' is TRUE, program is halted with an error message if file is missing,
// otherwise 'size' is set to -1 and the name of the (first) missing file is returned.
//
char* extendedReadFile(cchar* sources, int* size, bool output_error)
{
    char*       buf = xstrdup(sources);
    char*       src,* tmp,* orig_buf = buf;;
    int         tmp_size;
    vec<char>   result;

    for (;;){
        src = strtokr(&buf, "#");
        if (src == NULL) break;

        if (src[0] == '-' && src[1] == '\0')
            tmp = readFile(stdin, &tmp_size);
        else if (src[0] == '^'){
            tmp_size = strlen(src+1);
            tmp = xstrdup(src+1); }
        else{
            tmp = readFile(src, &tmp_size);
            if (tmp_size == -1){
                if (output_error)
                    fprintf(stderr, "ERROR! Could not open file '%s'.\n", src),
                    exit(1);
                else{
                    char*   ret = xstrdup(src);
                    assert(size != NULL);
                    *size = -1;
                    xfree(buf);
                    return ret;
                }
            }
        }

        int j = result.size();
        result.growTo(j + tmp_size);
        for (int i = 0; i < tmp_size; i++, j++)
            result[j] = tmp[i];
        xfree(tmp);
    }
    xfree(orig_buf);

    result.push('\0');
    if (size != NULL) *size = result.size();
    return result.release();
}
#undef BLOCK_SIZE


void writeFile(const char* filename, const char* data, int size)
{
    if (size == -1)
        size = strlen(data);

    FILE*   out = fopen(filename, "wb"); assert(out != NULL);
    fwrite(data, 1, size, out);
    fclose(out);
}


/*
     size       total program size
     resident   resident set size
     share      shared pages
     trs        text (code)
     drs        data/stack
     lrs        library
     dt         dirty pages
*/
int memReadStat(int field)
{
    char    name[256];
    pid_t pid = getpid();
    sprintf(name, "/proc/%d/statm", pid);
    FILE*   in = fopen(name, "rb");
    if (in == NULL) PANIC("Linux like '/proc' file system not supported.");
    int     value;
    for (; field >= 0; field--)
        fscanf(in, "%d", &value);
    fclose(in);
    return value;
}


int64 memUsed    (void) { return (int64)memReadStat(0) * (int64)getpagesize(); }
int64 memResident(void) { return (int64)memReadStat(1) * (int64)getpagesize(); }


int64 memPhysical(void)
{
    FILE*   in = fopen("/proc/meminfo", "rb");
    if (in == NULL) PANIC("Linux like '/proc' file system not supported.");
    char    buf[1024];
    for(;;){
        fgets(buf, sizeof(buf), in);
        if (feof(in)) PANIC("Unexpected format of '/proc/meminfo'.");
        if (strncmp(buf, "MemTotal:", 9) == 0){
            fclose(in);
            return (int64)atol(buf+9) * 1024;
        }
    }
}


//=================================================================================================
// Sub implementation files:


