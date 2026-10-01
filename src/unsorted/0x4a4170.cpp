// Decompiled by longcat-2.5-preview-free. Names are provisional.
#include <string.h>

#pragma pack(push, 1)
struct Entry_0049fc50 {                // 0x15b bytes
    unsigned char type;                // +0x0
    char unknown_1[0x13 - 0x1];
    short x1;                          // +0x13
    short y1;                          // +0x15
    short x2;                          // +0x17
    short y2;                          // +0x19
    unsigned char flags;               // +0x1b
    char unknown_2[0xb6 - 0x1c];
    char text[0x15b - 0xb6];           // +0xb6
};
#pragma pack(pop)

struct Holder_0049fc50 {
    int unknown_0;                     // +0x0
    Entry_0049fc50* entries;           // +0x4
};

struct Point_0049fc50 {
    int x;                             // +0x0
    int y;                             // +0x4
    int unknown_8[4];                  // +0x8
};

#pragma pack(push, 1)
struct Object_0049fc50 {
    char unknown_0[0x18];
    Holder_0049fc50* holder;           // +0x18
    char unknown_1c[0x3c - 0x1c];
    Point_0049fc50 point;              // +0x3c to +0x50
    char unknown_54[0x64 - 0x54];
    int focus;                         // +0x64
    char unknown_68[0x78 - 0x68];
    int field_78;                      // +0x78
    Point_0049fc50 field_7c;           // +0x7c to +0x94
    char unknown_94[0x94 - 0x94];
    short field_94;                    // +0x94
};
#pragma pack(pop)

struct Rect_0049fc50 {
    int x1;
    int y1;
    int x2;
    int y2;
};

int __stdcall FUN_0049fc50(Object_0049fc50* obj, int index);
int __stdcall FUN_004ab510(Object_0049fc50* obj, unsigned char buttons);
int __stdcall FUN_004ab5b0(Object_0049fc50* obj, unsigned int mask);
void __stdcall FUN_004ab690(Object_0049fc50* obj, int param_2);
int __stdcall FUN_004a23b0(Entry_0049fc50* entries, int index, Rect_0049fc50* out1, Rect_0049fc50* out2);
int __stdcall FUN_004a2580(Object_0049fc50* obj, int index);
int __stdcall FUN_004a2be0(Object_0049fc50* obj, int index);

// FUNCTION: 0x4a4170
void __stdcall FUN_004a4170(Object_0049fc50* obj, int index)
{
    Entry_0049fc50* entries = obj->holder->entries;
    Entry_0049fc50* entry = &entries[index];
    Point_0049fc50 point;
    Rect_0049fc50 out1;
    Rect_0049fc50 out2;
    int rel_x, rel_y;

    if ((entry->flags & 0x10) || *(int*)&entry->text[0x157 - 0xb6])
        return;

    point = obj->point;
    rel_x = point.x - entries->x1;
    rel_y = point.y - entries->y1;

    FUN_004a23b0(entries, index, &out1, &out2);

    if (obj->focus == index) {
        if (FUN_004ab5b0(obj, 3) == 0) {
            obj->focus = -1;
            obj->field_78 = 0;
        }
        if (obj->field_78 != 0) {
            if (entry->flags & 1)
                entry->text[0x140 - 0xb6] = obj->field_94 - *(short*)&obj->field_7c.x + rel_x;
            else
                entry->text[0x140 - 0xb6] = obj->field_94 - *(short*)&obj->field_7c.y + rel_y;
        } else {
            if (entry->flags & 1) {
                if (rel_x < out2.x1) entry->text[0x140 - 0xb6]--;
                else if (rel_x > out2.x2) entry->text[0x140 - 0xb6]++;
            } else {
                if (rel_y < out2.y1) entry->text[0x140 - 0xb6]--;
                else if (rel_y > out2.y2) entry->text[0x140 - 0xb6]++;
            }
        }
        if (entry->text[0x140 - 0xb6] > *(short*)&entry->text[0x136 - 0xb6] - 1)
            entry->text[0x140 - 0xb6] = *(short*)&entry->text[0x136 - 0xb6] - 1;
        if (entry->text[0x140 - 0xb6] < 0)
            entry->text[0x140 - 0xb6] = 0;
        if (entry->text[0x140 - 0xb6] != entry->text[0x140 - 0xb6]) {
            if (obj->holder)
                obj->holder->unknown_0 = 1;
            FUN_004a2580(obj, index);
            FUN_004a2be0(obj, index);
            if (*(int*)&entry->text[0x144 - 0xb6]) {
                ((int(__stdcall*)(Object_0049fc50*, int)) *(int*)&entry->text[0x144 - 0xb6])(obj, *(int*)&entry->text[0x14a - 0xb6]);
                return;
            }
        }
    } else {
        if (obj->field_78 == 0) {
            if (FUN_004ab510(obj, 1)) {
                obj->field_78 = 0;
                if (rel_x < out1.x1 || rel_x > out1.x2 || rel_y < out1.y1 || rel_y > out1.y2)
                    return;
                FUN_0049fc50(obj, index);
                FUN_004ab690(obj, 1);
                if (rel_x < out2.x1 || rel_x > out2.x2 || rel_y < out2.y1 || rel_y > out2.y2)
                    return;
                obj->field_78 = 1;
                obj->field_7c = point;
                obj->field_94 = entry->text[0x140 - 0xb6];
                return;
            }
            if (FUN_004ab510(obj, 2)) {
                obj->field_78 = 0;
                if (rel_x < out1.x1 || rel_x > out1.x2 || rel_y < out1.y1 || rel_y > out1.y2)
                    return;
                FUN_0049fc50(obj, index);
                FUN_004ab690(obj, 2);
                if (rel_x < out2.x1 || rel_x > out2.x2 || rel_y < out2.y1 || rel_y > out2.y2)
                    return;
                obj->field_78 = 1;
                obj->field_7c = point;
                obj->field_94 = entry->text[0x140 - 0xb6];
                return;
            }
        }
    }
}
