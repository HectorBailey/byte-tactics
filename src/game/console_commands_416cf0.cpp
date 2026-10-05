// Decompiled by Haiku. Names are provisional.

extern void* g_game;

class Class_004b73e0 {
public:
    unsigned char GetIntArg(int arg1, int arg2);
};

extern void FUN_00430f00(void);

// FUNCTION: 0x416cf0
void __stdcall FUN_00416cf0(void* param_1)
{
    unsigned char result = ((Class_004b73e0*)param_1)->GetIntArg(1, 0);
    *(unsigned char*)((char*)g_game + 0x1434d) = result;
    FUN_00430f00();
}
