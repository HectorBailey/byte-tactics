// Decompiled by Opus. Names are provisional.
struct Game;
extern Game* g_game;

class Class_004cfea0 {
public:
    int Is3DEnabled();
};

class Class_004cfe90 {
public:
    void Disable3D();
};

class SJE_CdPlayerClass {
public:
    void Enable3D();
};

void SaveSettings();

// FUNCTION: 0x416820
void __stdcall CmdSound3D(int unused)
{
    if ((*(Class_004cfea0**)((char*)g_game + 0x10))->Is3DEnabled()) {
        (*(Class_004cfe90**)((char*)g_game + 0x10))->Disable3D();
        SaveSettings();
    } else {
        (*(SJE_CdPlayerClass**)((char*)g_game + 0x10))->Enable3D();
        SaveSettings();
    }
}
