// Decompiled by deepseek-v4.1. Names are provisional.
// (Earlier credit, same file: started by space-bunny-free, continued by deepseek-v4.1-flash,
//  then GPT-6, whose 59.8 percent body the deepseek-v4.1 pass re-verified and left in place.)
// PARTIAL 59.8%, 608 of 613 bytes. Cache edge start/end heights before testing descent, keep the
// row countdown separate from the divisor, and share the height locals across both edge walks.
// The scan uses a guarded do-while with explicit index and point increments. This improves the
// previous 41.0% and reproduces the original 0x14024-byte stack allocation.
//
// What still differs (deepseek-v4.1 pass, all differences are register allocation; the
// instruction sequence and the nine local slots are identical):
//  * original: ebx=pts, esi=ymin, edi=xmax, ymax in [esp+0x14], count reloaded into eax/edx/ecx
//    scratch, ebp free for the edge-loop pointers.
//    ours:     ebx is scratch (count reloads), esi=ymin, edi=pts, ebp=ymax, xmax in [esp+0x14].
//    So the -999999 pair is swapped (xmax <-> ymax) and pts moves edi -> ebx, which also swaps
//    surf/color (ebp/ebx) in the final call block and the ecx/eax/eax picks there.
//  * the prologue loads differ from that same cause: original `mov eax,[esp+0x14030]` (count)
//    before `push ebx`, then `mov ebx,[esp+0x14030]` (pts); ours loads count into ebx after
//    `push ebx` and pts into edi after `push edi`.
// Tried, all compiling to the very same 608-byte body (no effect): every declaration order of
// the four extrema (y-first, x-first, one-line, split lines), the sibling 0x4c0c70 order
// (maxX, minY, maxY, minX), temps at function scope or at point of use, the scan index
// declared/initialised before the count guard (moves only the `xor ecx,ecx` slot), a
// function-scope point cursor, `int y = p->y` temporaries in the scan, and the 0x4c0c70
// edge-loop shape (function-scope i/j plus shared temps), which drops to 58.8%.
// Also tried earlier by GPT-6: 768 header sets, count representations and paired extremum
// indices; none improved the saved version.
//
// Count <= 0 still reaches uninitialized extremum indices, as does the original (0x4c08b7 and
// 0x4c0962).

struct Point_004c0820 {
    int x;
    int y;
    int z;
};

struct Span_004c0a90 {
    int x1; // +0x0
    int x2; // +0x4
    char unknown_8[0x18 - 0x8];
    int z1; // +0x18 (16.16)
    int z2; // +0x1c (16.16)
    char unknown_20[0x28 - 0x20];
};

struct Surface_004c0a90 {
    unsigned short pitch; // +0x0
    char unknown_2[0x10 - 0x2];
    unsigned char* bits;  // +0x10
    unsigned char* depth; // +0x14
};

void __stdcall FUN_004c0a90(int row, Span_004c0a90* span, Surface_004c0a90* surf,
                            unsigned char color);

// FUNCTION: 0x4c0820
int __stdcall FUN_004c0820(Surface_004c0a90* surf, Point_004c0820* pts, int count,
                           unsigned char color) {
    Span_004c0a90 spans[2048];
    Span_004c0a90* out;
    Point_004c0820* a;
    Point_004c0820* b;
    int ay;
    int y0, y1;
    int ymin = 999999;
    int ymax = -999999;
    int xmin = 999999;
    int xmax = -999999;
    int iymin, iymax;
    if (count > 0) {
        Point_004c0820* p = pts;
        int i = 0;
        do {
            if (p->y < ymin) {
                ymin = p->y;
                iymin = i;
            }
            if (p->y > ymax) {
                ymax = p->y;
                iymax = i;
            }
            if (p->x > xmax)
                xmax = p->x;
            if (p->x < xmin)
                xmin = p->x;
            i++;
            p++;
        } while (i < count);
    }
    if (ymax == ymin)
        return 0;
    {
        int i = iymin;
        out = spans;
        do {
            int j = i - 1;
            if (j < 0)
                j = count - 1;
            a = &pts[i];
            b = &pts[j];
            y0 = a->y;
            y1 = b->y;
            if (y0 < y1) {
                int h = y1 - y0;
                int x = a->x;
                int dx = ((b->x - x) << 16) / h;
                x = (x << 16) + 0xffff;
                int y = a->z << 16;
                int dy = ((b->z << 16) - y) / h;
                int rows = b->y - y0;
                do {
                    out->x1 = x >> 16;
                    out->z1 = y;
                    x += dx;
                    y += dy;
                    out++;
                } while (--rows);
            }
            i = i - 1;
            if (i < 0)
                i = count - 1;
        } while (i != iymax);
    }
    {
        int i = iymin;
        out = spans;
        do {
            int j = i + 1;
            if (j >= count)
                j = 0;
            a = &pts[i];
            b = &pts[j];
            y0 = a->y;
            y1 = b->y;
            if (y0 < y1) {
                int h = y1 - y0;
                int x = a->x;
                int dx = ((b->x - x) << 16) / h;
                x = (x << 16) + 0xffff;
                int y = a->z << 16;
                int dy = ((b->z << 16) - y) / h;
                int rows = b->y - y0;
                do {
                    out->x2 = x >> 16;
                    out->z2 = y;
                    x += dx;
                    y += dy;
                    out++;
                } while (--rows);
            }
            i = i + 1;
            if (i >= count)
                i = 0;
        } while (i != iymax);
    }
    {
        int y = ymin;
        Span_004c0a90* s = spans;
        while (y < ymax) {
            if (s->x2 - s->x1 > 0)
                FUN_004c0a90(y, s, surf, color);
            s++;
            y++;
        }
    }
    return 1;
}
