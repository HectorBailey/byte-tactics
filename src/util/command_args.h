// CommandArgs: one tokenised console command, the argument pointers, the
// text they point into and the count. util/commands.cpp defines its methods;
// every file that runs a command or reads its arguments includes this.
#ifndef COMMAND_ARGS_H
#define COMMAND_ARGS_H

class CommandArgs {
public:
    char* args[0x14];                  // +0x00 token pointers
    char buffer[0x7e];                 // +0x50 the tokens' text
    char unknown_ce[2];
    int count;                         // +0xd0 token count

    void Tokenize(char* text, char* end);
    CommandArgs* InitArgs();
    // One function: commands.cpp defines it returning the token as an int, and
    // the files that only read the text declare it returning the pointer.
    int GetArg(int index, int default_val);
    char* GetArg(int index, char* fallback);
    int GetIntArg(int index, int default_val);
    float GetFloatArg(int index, float default_val);
    void ShiftArgs(int n);

    // The range-checked getter: its redundant range check folds away, and the
    // extra use of the index decides the caller's registers.
    char* GetArgPtr(int index, char* def)
    {
        if (index < 0 || index >= count) return def;
        return args[index];
    }

    // Replaces each "%N" argument with argument N of another list.
    void SubstituteArgs(CommandArgs* other);
};

#endif
