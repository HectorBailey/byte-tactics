// Decompiled by Opus. Names are provisional.
// Same shape as 0x48f670: reads the "death timer runs out" defeat
// condition's state from a section (its writer is 0x48fd70).

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

class Class_0048fd50 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48fdb0
void Class_0048fd50::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("DefeatCondition_DeathTimerRunsOut");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
