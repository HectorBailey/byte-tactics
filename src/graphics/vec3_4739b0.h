// Vec3_004739b0: the whole-number position a particle step moves by, with its
// operator+=. The one declaration of the struct, for particles.cpp,
// particles_472630.cpp and particles_473a00.cpp; the files that only pass it by
// reference keep their forward declarations.
#ifndef VEC3_4739B0_H
#define VEC3_4739B0_H

struct Vec3_004739b0 {
    int x, y, z;
    Vec3_004739b0& operator+=(const Vec3_004739b0& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
};

#endif
