// Decompiled by Haiku. Names are provisional.

extern void* DAT_0050b9e0[];

// FUNCTION: 0x4d1c60
void* __stdcall SquashErrorString(int param_1)
{
    if (param_1 < 7) {
        return DAT_0050b9e0[param_1];
    }
    return 0;
}
