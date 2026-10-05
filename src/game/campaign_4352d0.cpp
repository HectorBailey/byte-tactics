// Decompiled by Opus. Names are provisional.
// Returns 1 when camps\<name>.TDF can be opened (the file is closed again).

struct File_004bb5d0;

void __stdcall FUN_004290f0(char* out, const char* dir, const char* name, const char* ext);
File_004bb5d0* __stdcall HAPI_OpenFileAppend(char* path);
int __stdcall HAPI_CloseFile(File_004bb5d0* file);

// FUNCTION: 0x4352d0
int __stdcall FUN_004352d0(char* name)
{
    char path[256];
    FUN_004290f0(path, "camps", name, "TDF");
    File_004bb5d0* file = HAPI_OpenFileAppend(path);
    if (file) {
        HAPI_CloseFile(file);
        return 1;
    }
    return 0;
}
