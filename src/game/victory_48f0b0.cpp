// Decompiled by Opus. Names are provisional.
// Same shape as 0x48ef40.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4800 {
public:
    int GetIntegerItem(char* name, int def);
};

// The "kill all of type" victory condition; reads its state from a section.
class Class_0048efb0 {
public:
    int satisfied;                       // +0x4
    int celebrated;                      // +0x8

    virtual void FUN_0048f880(HapiBank* obj);
};

// FUNCTION: 0x48f0b0
void Class_0048efb0::FUN_0048f880(HapiBank* obj)
{
    obj->OpenAccount("VictoryCondition_KillAllOfType");
    satisfied = ((Class_004b4800*)obj)->GetIntegerItem("Satisfied", 0);
    celebrated = ((Class_004b4800*)obj)->GetIntegerItem("Celebrated", 0);
}
