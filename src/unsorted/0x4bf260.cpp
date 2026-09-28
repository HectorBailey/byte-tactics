// Decompiled by Sonnet 5.5. Names are provisional.
// Draws a grid of connected lines into `surface`, or into the screen (locked
// with FUN_004c5e70, unlocked with FUN_004c5fa0) when `surface` is null:
// `rows` polylines, where counts[r] is the number of vertices of row r and
// the vertices follow one another in `points`. Every segment is clipped by
// FUN_004bea20 and drawn by FUN_004cc7ab. Like 0x4bf060 it returns the lock
// result on the screen path (a failed lock returns 0 without unlocking) and 1
// on the caller-surface path, and the locked loop carries an inlined copy of
// the single segment drawer with its own null-surface lock, which stays
// because the compiler tests the address of the local `screen`.

// NOT MATCHED: 84.7%, size exact. The four clipped coordinates land in the
// dead argument slots in a different order than the original's (original: x0
// at [esp+0x78], x1 0x7c, y1 0x80, y0 0x84; here y1 0x7c, y0 0x80, x1 0x84),
// and `counts` is loaded before the `rows > 0` test instead of after it. All
// 24 assignment orders and 24 declaration orders were tried (best 84.7%, the
// order used by 0x4bf060); function-scope declarations score 56.8%.

struct Surface_004bf260 {
    int unknown_0[2];
    int pitch;                         // +0x8
    unsigned char* pixels;             // +0xc
    int unknown_10[8];                 // 0x30 bytes, the lock descriptor
};

struct Point_004bf260 {
    int x;                             // +0x0
    int y;                             // +0x4
};

int __stdcall FUN_004c5e70(Surface_004bf260* out);
int __stdcall FUN_004c5fa0(Surface_004bf260* s);
int __stdcall FUN_004bea20(Surface_004bf260* dst, int* x0, int* y0, int* x1, int* y1);
void __cdecl FUN_004cc7ab(Surface_004bf260* dst, int x0, int y0, int x1, int y1, int color);

// FUNCTION: 0x4bf260
int __stdcall FUN_004bf260(Surface_004bf260* surface, Point_004bf260* points, int* counts,
                           int rows, int color)
{
    int result;
    if (surface == 0) {
        Surface_004bf260 screen;
        result = FUN_004c5e70(&screen);
        if (result != 0) {
            int r = rows;
            Point_004bf260* p = points;
            while (r > 0) {
                for (int j = 0; j < *counts - 1; j++) {
                    int x0, y0, x1, y1;
                    y1 = p[1].y;
                    x1 = p[1].x;
                    y0 = p[0].y;
                    x0 = p[0].x;
                    if (&screen == 0) {
                        Surface_004bf260 inner;
                        if (FUN_004c5e70(&inner)) {
                            if (FUN_004bea20(&inner, &x0, &y0, &x1, &y1))
                                FUN_004cc7ab(&inner, x0, y0, x1, y1, color);
                            FUN_004c5fa0(&inner);
                        }
                    } else {
                        if (FUN_004bea20(&screen, &x0, &y0, &x1, &y1))
                            FUN_004cc7ab(&screen, x0, y0, x1, y1, color);
                    }
                    p++;
                }
                p++;
                counts++;
                r--;
            }
            FUN_004c5fa0(&screen);
        }
    } else {
        int r = rows;
        Point_004bf260* p = points;
        while (r > 0) {
            for (int j = 0; j < *counts - 1; j++) {
                int x0, y0, x1, y1;
                y1 = p[1].y;
                x1 = p[1].x;
                y0 = p[0].y;
                x0 = p[0].x;
                if (FUN_004bea20(surface, &x0, &y0, &x1, &y1))
                    FUN_004cc7ab(surface, x0, y0, x1, y1, color);
                p++;
            }
            p++;
            counts++;
            r--;
        }
        result = 1;
    }
    return result;
}
