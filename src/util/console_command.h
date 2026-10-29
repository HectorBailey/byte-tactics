// ConsoleCommand: one record in the command tables, the name, the handler that
// runs it and the mask the handler wants. util/commands.cpp defines
// RegisterCommands, which walks such a table; game/console_commands.cpp, which
// defines the console, cheat and debug tables themselves, keeps a view of its
// own because including this header moves four of its functions.
#ifndef CONSOLE_COMMAND_H
#define CONSOLE_COMMAND_H

class CommandArgs;

struct ConsoleCommand {
    const char* name;                              // +0x0
    void (__stdcall* run)(CommandArgs* args);      // +0x4
    int mask;                                      // +0x8
};

#endif
