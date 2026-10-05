// Decompiled by Opus. Names are provisional.
// Reads the "any unit passes X" defeat condition's state from a section
// (same shape as 0x48f0b0).

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048fb60 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48fc00
void Class_0048fb60::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesX");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
