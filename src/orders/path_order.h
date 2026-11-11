#ifndef PATH_ORDER_H
#define PATH_ORDER_H

// PathOrder: the order that walks a unit along a path. One view for the files
// that build one: the 0x36-byte object, the two constructors they call and the
// two setters. data/progress.csv gives six constructors for the class; the other
// four take a HapiBank, a BitReader, a Source_0044e250 or a Source_0044e330, so
// the views that call those stay in their own files.

class Order;
struct Vec3;
struct Unit;

class PathOrder {
public:
    char data[0x36];
    // 0x44e2d0, ?PathOrder@@QAE@PAUOrder@@ABUVec3@@@Z.
    PathOrder(Order*, const Vec3&);
    // 0x44e190, ?PathOrder@@QAE@PAUOrder@@PAUUnit@@@Z.
    PathOrder(Order*, Unit*);
    // 0x44e6c0 is ?SetAltitude@PathOrder@@QAEXH@Z. 0x44e730 is
    // ?SetApproachRadius@PathOrder@@QAEXF@Z, but every caller passes an int and
    // spelling it float changes the conversion at all ten call sites and loses
    // seven of these functions, so the view keeps the int the files used.
    void SetAltitude(int);
    void SetApproachRadius(int);
};

#endif
