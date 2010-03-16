#ifndef TmpFiles_h
#define TmpFiles_h

#include <stdio.h>

//#include "Global.h"
//=================================================================================================


FILE* createTmpFile(const char* prefix, const char* mode, char* out_name = NULL);
void  deleteTmpFile(const char* prefix, bool exact = false);
void  deleteTmpFiles(void);


//=================================================================================================
#endif
