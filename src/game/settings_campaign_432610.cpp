// Decompiled by Opus. Names are provisional.

class HapiBank {
public:
    void OpenAccount(char* section);
};

class Class_004b48f0 {
public:
    int HasItem(char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048fe60 {
public:
    void FUN_0048fe60(HapiBank* file);
};

#pragma pack(push, 1)
struct Game {
    char unknown_0[0x37ebe];
    unsigned short flags_0 : 4;      // +0x37ebe
    unsigned short loaded : 1;       // +0x37ebe, bit 4
    unsigned short flags_5 : 11;
    char unknown_37ec0[0x37eec - 0x37ec0];
    short maxUnits;                  // +0x37eec
    char unknown_37eee[0x391ed - 0x37eee];
    Class_0048fe60* unknown_391ed;   // +0x391ed
};
#pragma pack(pop)

extern Game* g_game;
extern char* DAT_00503320;           // "Summary"

void __stdcall FUN_00466050(HapiBank* file);
void __stdcall FUN_0041d2b0(HapiBank* file);
void __stdcall FUN_00424c00(HapiBank* file);
void __stdcall FUN_00484d60(HapiBank* file);
void __stdcall FUN_00484e80(HapiBank* file);
void __stdcall FUN_00484fa0(HapiBank* file);
void __stdcall LoadUnits(HapiBank* file);
void __stdcall FUN_00438250(HapiBank* file);

// Reads the game summary section and every subsystem's saved state.
// FUNCTION: 0x432610
int __stdcall FUN_00432610(HapiBank* file)
{
    file->OpenAccount(DAT_00503320);
    if (((Class_004b48f0*)file)->HasItem("maxunits"))
        g_game->maxUnits = ((Class_004b4800*)file)->GetIntegerItem("maxunits", 0);
    FUN_00466050(file);
    FUN_0041d2b0(file);
    FUN_00424c00(file);
    FUN_00484d60(file);
    FUN_00484e80(file);
    FUN_00484fa0(file);
    LoadUnits(file);
    FUN_00438250(file);
    g_game->unknown_391ed->FUN_0048fe60(file);
    g_game->loaded = 1;
    return 1;
}
