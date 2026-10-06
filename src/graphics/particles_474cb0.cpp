// Decompiled by Sonnet. Names are provisional.

class Class_00474b00 {
public:
    char unknown_0[0x10];
    int a;  // +0x10
    int b;  // +0x14
    int IsExpired(int unused);
};

// FUNCTION: 0x474cb0
int Class_00474b00::IsExpired(int unused)
{
    return b >= a;
}
