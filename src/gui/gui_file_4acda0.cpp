// Decompiled by Opus. Names are provisional.
// Writes `depth` tabs and then a closing brace line.

struct Class_004bbbe0;

extern unsigned int __stdcall HAPI_WriteFile(Class_004bbbe0* file, void* data, unsigned int size);

// FUNCTION: 0x4acda0
void __stdcall FUN_004acda0(Class_004bbbe0* file, int depth)
{
    char tab = '\t';
    for (int i = 0; i < depth; i++)
        HAPI_WriteFile(file, &tab, 1);
    HAPI_WriteFile(file, "}\n", 2);
}
