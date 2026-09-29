// Decompiled by space-bunny-free. Names are provisional.
// Scales a (possibly skewed) source rect onto a destination surface one column
// at a time. It asks the surface for its clip rect, moves the left edge of the
// destination rect forward to it (advancing the two fixed-point source steps by
// the same amount, so the source follows the clip), clips the right edge, and
// then blits: 8 bits per pixel is an inline loop, 0x10/0x20/0x40/0x80 go to
// helpers, and anything else is an inline loop with a per-row byte stride.
// NOT MATCHING (86.1%). Everything matches byte for byte except the body of
// the left-edge clip, which is 13 instructions in the original and 15 here,
// leaving us 4 bytes long. The original keeps ONE accumulator (eax) for both
// products and the difference in ecx, and does both read-modify-writes straight
// to memory:
//     sub ecx, eax / mov eax, ebp / imul eax, ecx / add [edi+8], eax
//     mov eax, [esp+0x30] / imul eax, ecx / add [edi+0xc], eax
// Here the scheduler hoists the colstep product above the rowstep store and
// uses two accumulators, so the second update becomes load/add/store:
//     sub eax, ecx / mov ecx, eax / imul eax, [esp+0x30] / imul ecx, ebp
//     add [edi+8], ecx / mov ecx, [edi+0xc] / add ecx, eax / mov [edi+0xc], ecx
// Tried and did NOT change it: swapping the multiply operand order, hoisting
// `skip` to a function-scope local, `int dy`/`int dx` product temporaries (both
// before and interleaved with the stores), one reused product temporary, an
// explicit pointer to each updated element, `int old = rect[0]` before the
// store, computing `skip` before the `if` and testing `skip > 0`, and moving
// the `rect[0] = bounds.left` store to the end of the block (that one is the
// best of the lot, and is what is here, worth 4 bytes over the natural order).
// 6 real check runs; every variant above was scored with the free `--sym` path.
#include <stdio.h>

struct Rect_4c7310 {
    int left;
    int top;
    int right;
    int bottom;
};

struct Info_4c7310 {
    unsigned short bits;
    char unknown_2[0x10 - 2];
    unsigned char* data;
};

class Class_004c6ae0 {
public:
    char unknown_0[8];
    int field_8;
    int field_c;
    char unknown_10[0x1c - 0x10];
    Rect_4c7310 field_1c;                 // +0x1c

    Rect_4c7310* FUN_004c6ae0(Rect_4c7310* out);
};

void __cdecl FUN_004cd896(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl FUN_004cd8da(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl FUN_004cd91e(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);
void __cdecl FUN_004cd962(unsigned char* dest, unsigned char* src, int width, int y, int x, int rowstep, int colstep);

// FUNCTION: 0x4c7310
void __stdcall FUN_004c7310(int param_1, int* rect, Class_004c6ae0* surf, Info_4c7310* info)
{
    unsigned char* dest = (unsigned char*)surf->field_c;
    unsigned char* src = info->data;
    int rowstep = (rect[4] - rect[2]) / (rect[1] - rect[0]);
    int colstep = (rect[5] - rect[3]) / (rect[1] - rect[0]);
    Rect_4c7310 bounds;
    int width;
    int y;
    int x;

    surf->FUN_004c6ae0(&bounds);
    if (rect[0] < bounds.left) {
        int skip = bounds.left - rect[0];
        rect[2] += rowstep * skip;
        rect[3] += colstep * skip;
        rect[0] = bounds.left;
    }
    if (rect[1] > bounds.right)
        rect[1] = bounds.right;
    width = rect[1] - rect[0];
    if (width > 0) {
        y = rect[2];
        x = rect[3];
        dest += surf->field_8 * param_1 + rect[0];
        switch (info->bits) {
        case 0x80:
            FUN_004cd896(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x40:
            FUN_004cd8da(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x20:
            FUN_004cd91e(dest, src, width, y, x, rowstep, colstep);
            return;
        case 0x10:
            FUN_004cd962(dest, src, width, y, x, rowstep, colstep);
            return;
        case 8: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + ((x >> 13) & ~7)];
                y += rowstep;
                x += colstep;
            } while (--n);
            break; }
        default: {
            int n = width;
            do {
                *dest++ = src[(y >> 16) + (x >> 16) * info->bits];
                y += rowstep;
                x += colstep;
            } while (--n);
            break; }
        }
    }
}
