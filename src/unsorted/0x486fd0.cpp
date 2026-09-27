// Decompiled by space-bunny-free. Names are provisional.

// Reads the "Units" chunk of a save file: every unit record read here is
// handed to FUN_00487080, which looks the unit up by the id in the record. A
// record two bytes short (0xb6 instead of 0xb8) is used too, but with the low
// word of its id field cleared to 0 first.

extern char DAT_005077ec[];   // "Units"
extern char DAT_00503358[];   // "Version"
extern char DAT_00508c14[];   // "Number of Units"

// Chunked file reader.
class Class_004b4560 {
public:
    int FUN_004b4560(char* name);
};

class Class_004b4800 {
public:
    int FUN_004b4800(char* name, int param_2);
};

class Class_004b4b50 {
public:
    int FUN_004b4b50(int index);
};

class Class_004b4c10 {
public:
    int FUN_004b4c10(int pos);
};

class Class_004b4c80 {
public:
    unsigned int FUN_004b4c80(void* buf, int size);
};

void __stdcall FUN_00487080(int unit, Class_004b4560* file);

#pragma pack(push, 1)
// The id is read as a dword (0x487080 only uses its low word), but the short
// read path clears just that low word. The test is written as read + 2 because
// the original computes it that way.
struct UnitRec_00486fd0 {              // 0xb8 bytes
    char unknown_0[0x21];
    union {
        int id;                        // +0x21
        struct {
            short unit;                // +0x21
            short flag;                // +0x23
        } id_flag;
    };
    char unknown_25[0xb8 - 0x25];
};
#pragma pack(pop)

// FUNCTION: 0x486fd0
void __stdcall FUN_00486fd0(Class_004b4560* file)
{
    if (file->FUN_004b4560(DAT_005077ec)
        && ((Class_004b4800*)file)->FUN_004b4800(DAT_00503358, 0) == 0x11) {
        int count = ((Class_004b4800*)file)->FUN_004b4800(DAT_00508c14, 0);
        for (int i = 0; i < count; i++) {
            if (((Class_004b4b50*)file)->FUN_004b4b50(i)) {
                ((Class_004b4c10*)file)->FUN_004b4c10(0);
                UnitRec_00486fd0 rec;
                unsigned int read = ((Class_004b4c80*)file)->FUN_004b4c80(&rec, 0xb8);
                if (read != 0xb8) {
                    if (read + 2 != 0xb8)
                        continue;
                    rec.id_flag.unit = 0;
                }
                FUN_00487080(rec.id, file);
            }
        }
    }
}
