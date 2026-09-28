// Decompiled by deepseek-v4.1-flash. Names are provisional.
//
// Firing angle for a ballistic weapon. The discriminant of the launch-angle
// quadratic is built from the horizontal distance (_hypot of the two ground
// offsets), the game's gravity field and the vertical offset; the two roots
// come out of acos; the one that lies between the given angle and pi/4 is
// converted to a 16-bit angle (radians * 32768 / pi). 0x8000 means no solution.
//
// PARTIAL (best 72.7%). What still differs: MSVC's floating point schedule.
// The original loads g and dy first and forms g*dy before it converts `speed`
// (fild g; fild dy; add esp,0x10; fstp [esp]; fmul [esp]; fild speed), keeps
// every intermediate in the six 0x30 frame slots and cleans the _hypot
// argument area up immediately. Our build converts dy, g and speed first (the
// shared (double)dy conversion is hoisted), forms g*dy, and spills a temporary
// into the still-live _hypot argument area, so `add esp, 0x10` comes much
// later. The tail also duplicates the radian-to-angle multiply into the first
// branch where the original shares one `jmp` to a single copy. Header sets and
// many equivalent spellings of the discriminant were tried; all stay at 72.7%.
#include <math.h>

#pragma pack(push, 1)
struct Game_0049a890 {
    char unknown_0[0x14263];
    int gravity;                       // +0x14263
};
#pragma pack(pop)

extern Game_0049a890* g_game;

// FUNCTION: 0x49a890
short __stdcall FUN_0049a890(int dx, int dy, int dz, int speed, float angle)
{
    int g = g_game->gravity;
    int g2 = g * g;
    double dist = _hypot((double)dx, (double)dz);
    double s = (double)g * (double)dy;
    double vsq = (double)speed * (double)speed;
    double disc = ((vsq - s * -2.0) * vsq + (double)dy * (double)dy * (double)g2)
                    * (dist * dist) * (dist * dist)
                - (dist * dist) * (dist * dist) * (double)g2
                  * ((double)dy * (double)dy + dist * dist);
    if (disc < 0.0)
        return (short)0x8000;
    double root = sqrt(disc);
    double c1 = ((vsq + s) * (dist * dist) + root)
              / (((double)dy * (double)dy + dist * dist) * 2.0);
    double c2 = ((vsq + s) * (dist * dist) - root)
              / (((double)dy * (double)dy + dist * dist) * 2.0);
    double a1 = c1 > 0.0 ? acos(sqrt(c1) / (double)speed) : 1.570796326794895;
    double a2 = c2 > 0.0 ? acos(sqrt(c2) / (double)speed) : 1.570796326794895;
    double r;
    if (angle < a1 && a1 <= 0.7853981633974475)
        r = a1;
    else if (a2 > angle && a2 <= 0.7853981633974475)
        r = a2;
    else
        return (short)0x8000;
    return (short)(r * 32768.0 * 0.318309886183791);
}
