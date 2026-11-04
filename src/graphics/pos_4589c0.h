// Pos_4589c0: the 12-byte 16.16 position the model renderer passes by value,
// with its constructor. The one declaration of the struct, for
// model_render.cpp and model_render_4589c0.cpp; memory_cache.h keeps its
// forward declaration, so the files that include it pay no extra ids.
#ifndef POS_4589C0_H
#define POS_4589C0_H

struct Pos_4589c0 {
    int x;
    int y;
    int z;

    Pos_4589c0(int a, int b, int c) { x = a; y = b; z = c; }
};

#endif
