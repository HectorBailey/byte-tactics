// Decompiled by Opus. Names are provisional.
// Same shape as 0x48f330: reads the "unit type passes X" victory condition's
// state from a section.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048f3e0 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48f480
void Class_0048f3e0::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_UnitTypePassesX");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
