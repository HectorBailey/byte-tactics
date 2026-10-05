// Decompiled by Opus. Names are provisional.
// Same shape as 0x48f160: writes the "kill enemy commander" victory
// condition's state to a section.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4630 {
public:
    void SetIntegerItem(const char* name, int value);
};

extern char DAT_00508f3c[]; // "VictoryCondition_KillEnemyCommander"
extern char DAT_00508f30[]; // "Satisfied"
extern char DAT_00508f24[]; // "Celebrated"

class Class_0048ea00 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f840(HapiBank* obj);
};

// FUNCTION: 0x48eac0
void Class_0048ea00::FUN_0048f840(HapiBank* obj)
{
    obj->OpenAccount(DAT_00508f3c);
    ((Class_004b4630*)obj)->SetIntegerItem(DAT_00508f30, satisfied);
    ((Class_004b4630*)obj)->SetIntegerItem(DAT_00508f24, celebrated);
}
