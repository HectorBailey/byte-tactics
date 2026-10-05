// Decompiled by Opus. Names are provisional.
// Reads the "all units killed" defeat condition's state from a section;
// the writing counterpart is 0x48f840, compare 0x48ef40.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048f840 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48f880
void Class_0048f840::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_AllUnitsKilled");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
