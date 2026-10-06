// Decompiled by Haiku. Names are provisional.

class Class_00473560 {
public:
    char unknown_0[0x30];
    int field_30;

    int IsExpired(int param_1);
};

// FUNCTION: 0x4736c0
int Class_00473560::IsExpired(int param_1) {
    return param_1 > field_30;
}
