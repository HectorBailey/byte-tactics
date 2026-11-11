#ifndef POINT_44EEC0_H
#define POINT_44EEC0_H

// Point_0044eec0: the (x, y) pair the order-target area classes keep in a
// std::vector. data/progress.csv spells the element UPoint_0044eec0 in the
// vector types of 0x44da00, 0x44eef0 and 0x44ec30, so the three files that
// touch that vector share this one view.

struct Point_0044eec0 {
    short x;
    short y;
};

#endif
