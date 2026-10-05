// Decompiled by DeepSeek V4.1 Flash. Names are provisional.
// Writes the meteor shower state to the "Meteor" section of a parsed text file.

class HapiBank {
public:
    void OpenAccount(const char* name);
};

class Class_004b4630 {
public:
    void SetIntegerItem(const char* name, int value);
};

struct Point16_00438180 {
    short x;
    short y;
};

extern int DAT_0051232c;
extern int DAT_00512318;
extern int DAT_005122e8;
extern int DAT_0051231c;
extern int DAT_00512330;
extern Point16_00438180 DAT_00512320;
extern Point16_00438180 DAT_00512334;

// FUNCTION: 0x438180
void __stdcall FUN_00438180(HapiBank* file)
{
    file->OpenAccount("Meteor");
    ((Class_004b4630*)file)->SetIntegerItem("Enabled", DAT_0051232c);
    ((Class_004b4630*)file)->SetIntegerItem("Active", DAT_00512318);
    ((Class_004b4630*)file)->SetIntegerItem("Next Strike Time", DAT_005122e8);
    ((Class_004b4630*)file)->SetIntegerItem("Time Strike Ends", DAT_0051231c);
    ((Class_004b4630*)file)->SetIntegerItem("Next Hit Time", DAT_00512330);
    ((Class_004b4630*)file)->SetIntegerItem("Origin X", DAT_00512320.x);
    ((Class_004b4630*)file)->SetIntegerItem("Origin Z", DAT_00512320.y);
    ((Class_004b4630*)file)->SetIntegerItem("Target X", DAT_00512334.x);
    ((Class_004b4630*)file)->SetIntegerItem("Target Z", DAT_00512334.y);
}
