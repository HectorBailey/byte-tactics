// Decompiled by Opus. Names are provisional.
// Registers the "plan", "weight" and "limit" console commands.

typedef void (__stdcall* Command_00406f00)(char* args);

void __stdcall RegisterCommand(const char* name, Command_00406f00 fn, int flags);
void __stdcall FUN_00406c90(char* args);
void __stdcall FUN_00406db0(char* args);
void __stdcall FUN_00406e40(char* args);

// FUNCTION: 0x406f00
void FUN_00406f00()
{
    RegisterCommand("plan", FUN_00406c90, 8);
    RegisterCommand("weight", FUN_00406db0, 8);
    RegisterCommand("limit", FUN_00406e40, 8);
}
