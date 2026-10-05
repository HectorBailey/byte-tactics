// Decompiled by Opus. Names are provisional.
// Appends a string (with its terminating zero) to DEBUG.FIL, creating the
// file if it cannot be opened for appending.
#include <string.h>

struct File_004bb5d0;

File_004bb5d0* __stdcall HAPI_OpenFileAppend(char* path);
File_004bb5d0* __stdcall HAPI_CreateFile(char* path);
unsigned int __stdcall HAPI_WriteFile(File_004bb5d0* file, void* data, unsigned int size);
int __stdcall HAPI_CloseFile(File_004bb5d0* file);

// FUNCTION: 0x49e640
void __stdcall FUN_0049e640(char* text)
{
    File_004bb5d0* file = HAPI_OpenFileAppend("DEBUG.FIL");
    if (!file)
        file = HAPI_CreateFile("DEBUG.FIL");
    HAPI_WriteFile(file, text, strlen(text) + 1);
    HAPI_CloseFile(file);
}
