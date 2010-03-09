#include "TmpFiles.h"

#include "string.h"
#include "Vec.h"

static vec<FILE*>  tmp_fps;
static vec<const char*> tmp_files;

#ifdef NoExceptions
#define memAssert(cond) assert(cond)
#else
class Exception_MemOut {};
#define memAssert(cond) if (!(cond)) throw Exception_MemOut()
#endif

//template<class T> macro T* xmalloc(size_t size) ___malloc;
template<class T> T* xmalloc(size_t size)
{
    T*   tmp = (T*)malloc(size * sizeof(T));
    memAssert(size == 0 || tmp != NULL);
    return tmp;
}
    
//template<class T> macro T* xrealloc(T* ptr, size_t size) ___malloc;
template<class T> T* xrealloc(T* ptr, size_t size)
{
    T*   tmp = (T*)realloc((void*)ptr, size * sizeof(T));
    memAssert(size == 0 || tmp != NULL);
    return tmp;
}

template<class T> void xfree(T* ptr)
{
    if (ptr != NULL) free((void*)ptr);
}

// 'out_name' should NOT be freed by caller.
FILE* createTmpFile(const char* prefix, const char* mode, char* out_name)
{
    char*   name = xmalloc<char>(strlen(prefix) + 6 + 1);
    strcpy(name, prefix);
    strcat(name, "XXXXXX");

    int fd = mkstemp(name);
    if (fd == -1){
        fprintf(stderr, "ERROR! Could not create temporary file with prefix: %s\n", prefix);
        exit(1); }
    FILE* fp = fdopen(fd, mode); assert(fp != NULL);

    tmp_fps  .push(fp);
    tmp_files.push(name);
    if (&out_name != NULL) out_name = name;
    return fp;
}


// If 'exact' is set, 'prefix' is the full name returned by 'createTmpFile()'.
void deleteTmpFile(const char* prefix, bool exact)
{
    uint    len = strlen(prefix);
    for (int i = 0; i < tmp_files.size();){
        if (!exact && (strlen(tmp_files[i]) == len + 6 && strncmp(tmp_files[i], prefix, len) == 0)
        ||   exact && (strcmp(tmp_files[i], prefix) == 0))
        {
            fclose(tmp_fps[i]);
            remove(tmp_files[i]);
            xfree(tmp_files[i]);
            tmp_fps[i] = tmp_fps.last();
            tmp_fps.pop();
            tmp_files[i] = tmp_files.last();
            tmp_files.pop();
        }else
            i++;
    }
}


// May be used to delete all temporaries. Will also be called automatically upon (normal) program exit.
void deleteTmpFiles(void)
{
    for (int i = 0; i < tmp_fps.size(); i++){
        fclose(tmp_fps[i]);
        remove(tmp_files[i]);
        xfree(tmp_files[i]);
    }
}


//void FINALIZER(TmpFiles) {
//    deleteTmpFiles();
//}
