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

class Class_004cfe80 {
public:
    void Enable3D();
};

void FUN_00430f00();

// FUNCTION: 0x416820
void __stdcall FUN_00416820(int unused)
{
    if ((*(Class_004cfea0**)((char*)g_game + 0x10))->Is3DEnabled()) {
        (*(Class_004cfe90**)((char*)g_game + 0x10))->Disable3D();
        FUN_00430f00();
    } else {
        (*(Class_004cfe80**)((char*)g_game + 0x10))->Enable3D();
        FUN_00430f00();
    }
}
