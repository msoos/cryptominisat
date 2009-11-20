#ifndef TmpFiles_h
#define TmpFiles_h

#include "Global.h"
//=================================================================================================


FILE* createTmpFile(cchar* prefix, cchar* mode, char* out_name = *(char**)NULL);
void  deleteTmpFile(cchar* prefix, bool exact = false);
void  deleteTmpFiles(void);


//=================================================================================================
#endif
