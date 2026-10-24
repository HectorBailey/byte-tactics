// Decompiled by Opus. Names are provisional.
// strcpy and strlen are declared here, not through <string.h>: the two headers
// below cost symbol ids, and this shorter list is what keeps 0x4a0880 matching.
extern "C" char* __cdecl strcpy(char*, const char*);
extern "C" unsigned int __cdecl strlen(const char*);

#include "gadget.h"
#include "gui.h"
#include "layer.h"

// FUNCTION: 0x4a0880
void __stdcall SetGadgetText(Gui* obj, int index, char* text)
{
    strcpy(obj->layer->entries[index].u.text, text);
    if (obj->focus == index) {
        obj->cursor = strlen(text);
    }
}
