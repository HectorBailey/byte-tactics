// Decompiled by deepseek-v4.1-flash, GPT-5.6-Terra, LongCat 2.5 Preview Free, Sonnet, space-bunny-free, GPT-6, Opus and Haiku. Names are provisional.
//
// The COB script cache: a file-local std::map<int, int> (s_cobScriptCache) from a
// loaded script's address to its checksum, and the three functions that use
// it. LoadCobScript loads a script through HAPI, records its checksum and turns
// the script's offsets into pointers; FreeCobScript erases the entry and
// releases the script; GetCobChecksum looks a script's checksum up.
//
// Everything from 0x4b2840 on is the compiler's out-of-line copy of a member of
// the map's tree (std::_Tree<int, pair<const int, int>, ...>) that those three
// functions use, and has no source of its own. The map's dynamic initialiser
// (0x4b2290) and its atexit destructor (0x4b2340) are generated from the
// definition of s_cobScriptCache.
#include <map>

// The loaded COB script data: the header the file starts with and the tables
// that follow it in the same buffer. The type the other views of it (cob.cpp's
// ScriptTable, units_485010.cpp's) share a name with; its pointers are held as
// ints because Relocate adds the buffer's address to them as integers.
struct ScriptTable {
    int unknown_0;                   // +0x00
    int count;                       // +0x04, of the scripts
    int pieceCount;                  // +0x08
    int unknown_c;                   // +0x0c
    int staticCount;                 // +0x10
    int namedLocalCount;             // +0x14
    int entries;                     // +0x18, each script's code offset
    int names;                       // +0x1c
    int pieceNames;                  // +0x20
    int code;                        // +0x24
    int namedLocals;                 // +0x28
};

// One named local of the script: its index and the name's address in the
// buffer, held as an int because Relocate adds to it as an integer.
struct CobNamedLocalEntry {
    int index;        // +0x00
    int name;         // +0x04
};

extern ScriptTable* __stdcall HAPI_LoadFile(char* name, int reserved);
extern int __stdcall HAPI_FileLengthByName(char* name);
extern int __stdcall ComputeChecksum(unsigned char* data, int len);
extern void __cdecl GameFreeThunk(void* x);

// FUNCTION: 0x4b2290 _$E6
// FUNCTION: 0x4b2340 _$E4
static std::map<int, int> s_cobScriptCache;

// The size of this helper decides whether the compiler expands map::insert
// into LoadCobScript; the original left that call out of line.
static void Relocate(ScriptTable* data)
{
    data->entries += (int)data;
    data->names += (int)data;
    for (int i = 0; i < data->count; i++)
        ((int*)data->names)[i] += (int)data;
    data->pieceNames += (int)data;
    for (int j = 0; j < data->pieceCount; j++)
        ((int*)data->pieceNames)[j] += (int)data;
    data->code += (int)data;
    data->namedLocals += (int)data;
    for (int k = 0; k < data->namedLocalCount; k++)
        ((CobNamedLocalEntry*)data->namedLocals)[k].name += (int)data;
}

// FUNCTION: 0x4b2450
ScriptTable* __stdcall LoadCobScript(char* name)
{
    ScriptTable* data = HAPI_LoadFile(name, 0);
    if (data == 0)
        return 0;
    int sum = ComputeChecksum((unsigned char*)data, HAPI_FileLengthByName(name));
    s_cobScriptCache[(int)data] = sum;
    Relocate(data);
    return data;
}

// FUNCTION: 0x4b2540
void __stdcall FreeCobScript(int key)
{
    if (key != 0) {
        int local_key = key;
        s_cobScriptCache.erase(local_key);
        GameFreeThunk((void*)key);
    }
}

// FUNCTION: 0x4b26f0
int __stdcall GetCobChecksum(int key)
{
    return s_cobScriptCache[key];
}

// The map tree's members the compiler emitted out of line for the functions above:
// FUNCTION: 0x4b2840 ?begin@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AViterator@12@XZ
// FUNCTION: 0x4b2850 ?insert@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AU?$pair@Viterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@_N@2@ABU?$pair@HH@2@@Z
// FUNCTION: 0x4b2ac0 ?erase@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAE?AViterator@12@V312@@Z
// FUNCTION: 0x4b2fb0 ?_Erase@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3000 ??0?$pair@Viterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@_N@std@@QAE@ABViterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@1@AB_N@Z
// FUNCTION: 0x4b3020 ?_Insert@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAE?AViterator@12@PAU_Node@12@0ABU?$pair@HH@2@@Z
// FUNCTION: 0x4b3310 ?_Lrotate@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3370 ?_Min@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@KGPAU_Node@12@PAU312@@Z
// FUNCTION: 0x4b33b0 ?_Rrotate@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEXPAU_Node@12@@Z
// FUNCTION: 0x4b3410 ?_Buynode@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IAEPAU_Node@12@PAU312@W4_Redbl@12@@Z
// FUNCTION: 0x4b3430 ?_Lbound@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IBEPAU_Node@12@ABH@Z
// FUNCTION: 0x4b3490 ?_Ubound@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@IBEPAU_Node@12@ABH@Z
// FUNCTION: 0x4b34f0 ?_Dec@iterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAEXXZ
// FUNCTION: 0x4b3590 ?_Inc@iterator@?$_Tree@HU?$pair@HH@std@@U_Kfn@?$map@HHU?$less@H@std@@V?$allocator@H@2@@2@U?$less@H@2@V?$allocator@H@2@@std@@QAEXXZ
