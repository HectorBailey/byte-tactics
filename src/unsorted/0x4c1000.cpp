// Decompiled by space-bunny-free, finished by deepseek-v4.1-flash, finished by GPT-6, finished by
// deepseek-v4.1. Names are provisional. PARTIAL 76.0%, 797 of 791 bytes. Share y0/y1 between the
// two edge walks alongside the previously shared pointer/index/dx locals. This improves the earlier
// 73.3%, 809-byte version.
// Remaining differences are slot allocation and its fallout, not control flow:
//   - two stack-slot swaps: ours ymin@[esp+0x20] / out@[esp+0x18] (original ymin@0x18 / out@0x20),
//     and ours lasty@[esp+0x2c] / imin@[esp+0x34] (original imin@0x2c / lasty@0x34);
//   - 6 extra bytes of `mov reg,reg` copies (the clip checks and both loop tails) that follow from
//     those swaps: surf wants ECX and the 16-bit scratch EAX in the three clip tests, ours is the
//     other way round; loop 2 uses [esp+0x24] for the vertex pointer and [esp+0x28] for the next
//     index, the original swaps those two for the second loop only.
// Tried this session, no change (all still 76.0%, 797 bytes): hoisting `Span* out` to the first
// local slot, hoisting `int lasty` next to the other scalars. Giving loop 2 its own i/j/k/a/b
// variables did reshuffle the allocator (ymin then landed on 0x18) but dropped to 72.0% and pushed
// out to 0x10, so more variables is not the lever. Partial temporary-sharing combinations, all 24
// bound declaration orders, 768 header sets and surface getters did not improve this version.

struct Surface_004c1000 {
    unsigned short pitch;   // +0x0, also the clip width
    unsigned short field_2; // +0x2, the clip height
    char unknown_4[0x10 - 0x4];
    unsigned char* bits;  // +0x10
    unsigned char* depth; // +0x14
};

struct Vertex_004c1000 {
    int x; // +0x0
    int y; // +0x4
    int z; // +0x8
};

struct Span_004c1000 {
    int x1; // +0x0
    int x2; // +0x4
    char unknown_8[0x18 - 0x8];
    int z1; // +0x18
    int z2; // +0x1c
    char unknown_20[0x28 - 0x20];
};

void __stdcall FUN_004c06e0(int row, Span_004c1000* span, Surface_004c1000* surf, int color);

// FUNCTION: 0x4c1000
int __stdcall FUN_004c1000(Surface_004c1000* surf, Vertex_004c1000* verts, int count, int color) {
    int y0, y1;
    Span_004c1000 spans[2048];
    int ymin = 999999;
    int ymax = -999999;
    int xmin = 999999;
    int xmax = -999999;
    int imin;
    int imax;
    int i;
    Span_004c1000* out;
    int j;
    int k;
    Vertex_004c1000* a;
    Vertex_004c1000* b;
    int dxdy;
    for (i = 0; i < count; i++) {
        int y = verts[i].y;
        if (y < ymin) {
            ymin = y;
            imin = i;
        }
        if (y > ymax) {
            ymax = y;
            imax = i;
        }
        int x = verts[i].x;
        if (x > xmax)
            xmax = x;
        if (x < xmin)
            xmin = x;
    }
    if (xmin > (int)surf->pitch - 1) {
        return 0;
    }
    if (ymax < 0) {
        return 0;
    }
    int lasty = (int)surf->field_2 - 1;
    if (ymin > lasty) {
        return 0;
    }
    if (ymin < 0) {
        ymin = 0;
    }
    if (ymax > lasty) {
        ymax = lasty;
    }
    if (ymax == ymin) {
        return 0;
    }
    out = spans;
    i = imin;
    for (;;) {
        j = i - 1;
        k = j;
        if (k < 0) {
            k = count - 1;
        }
        a = &verts[i];
        b = &verts[k];
        y0 = a->y;
        y1 = b->y;
        if (y0 < y1) {
            int dy = y1 - y0;
            int x = a->x;
            dxdy = ((b->x - x) << 16) / dy;
            x = (x << 16) + 0xffff;
            int z = a->z << 16;
            int dzdy = ((b->z << 16) - z) / dy;
            if (y0 < 0) {
                x -= dxdy * y0;
                z -= dzdy * y0;
                y0 = 0;
            }
            if (y1 > lasty) {
                y1 = lasty;
            }
            if (y0 < y1) {
                int n = y1 - y0;
                do {
                    out->x1 = x >> 16;
                    out->z1 = z;
                    x += dxdy;
                    z += dzdy;
                    out++;
                } while (--n);
            }
        }
        i = j;
        if (i < 0) {
            i = count - 1;
        }
        if (i == imax) {
            break;
        }
    }
    out = spans;
    i = imin;
    for (;;) {
        j = i + 1;
        k = j;
        if (k >= count) {
            k = 0;
        }
        a = &verts[i];
        b = &verts[k];
        y0 = a->y;
        y1 = b->y;
        if (y0 < y1) {
            int dy = y1 - y0;
            int x = a->x;
            dxdy = ((b->x - x) << 16) / dy;
            x = (x << 16) + 0xffff;
            int z = a->z << 16;
            int dzdy = ((b->z << 16) - z) / dy;
            if (y0 < 0) {
                x -= dxdy * y0;
                z -= dzdy * y0;
                y0 = 0;
            }
            if (y1 > lasty) {
                y1 = lasty;
            }
            if (y0 < y1) {
                int n = y1 - y0;
                do {
                    out->x2 = x >> 16;
                    out->z2 = z;
                    x += dxdy;
                    z += dzdy;
                    out++;
                } while (--n);
            }
        }
        i = j;
        if (i >= count) {
            i = 0;
        }
        if (i == imax) {
            break;
        }
    }
    out = spans;
    for (i = ymin; i < ymax; i++) {
        if (out->x2 - out->x1 > 0) {
            FUN_004c06e0(i, out, surf, color);
        }
        out++;
    }
    return 1;
}
