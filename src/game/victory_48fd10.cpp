// Decompiled by Opus. Names are provisional.
// Reads the "any unit passes Z" defeat condition's state from a section
// (same shape as 0x48f480).

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048fc70 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48fd10
void Class_0048fc70::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AnyUnitPassesZ");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
