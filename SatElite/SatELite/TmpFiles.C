#include "TmpFiles.h"

static vec<FILE*>  tmp_fps;
static vec<cchar*> tmp_files;


// 'out_name' should NOT be freed by caller.
FILE* createTmpFile(cchar* prefix, cchar* mode, char* out_name)
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
void deleteTmpFile(cchar* prefix, bool exact)
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


FINALIZER(TmpFiles) {
    deleteTmpFiles();
}
