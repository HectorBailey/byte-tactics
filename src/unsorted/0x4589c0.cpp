// Decompiled by deepseek-v4.1-flash. Names are provisional.
#include <string.h>

struct Child_4589c0;
struct Model_4589c0;

struct Owner_4589c0 {
    char unknown_0[0x6a];
    int x;                          // +0x6a
    int y;                          // +0x6e
    int z;                          // +0x72
    char unknown_76[0x8a - 0x76];
    Child_4589c0* firstChild;       // +0x8a
};

struct Model_4589c0 {
    int pieceCount;                 // +0x00
    char unknown_4[0xc - 0x4];
    Owner_4589c0* owner;            // +0x0c
    void* bitmap;                   // +0x10
};

struct Child_4589c0 {
    char unknown_0[0x6a];
    int x;                          // +0x6a
    int y;                          // +0x6e
    int z;                          // +0x72
    char unknown_76[0x8e - 0x76];
    Child_4589c0* next;             // +0x8e
    char unknown_92[0x9e - 0x92];
    Model_4589c0* model;            // +0x9e
    char unknown_a2[0x110 - 0xa2];
    unsigned int flags;             // +0x110
};

struct Image_4589c0 {
    unsigned short width;           // +0x00
    unsigned short height;          // +0x02
    short dx;                       // +0x04
    short dy;                       // +0x06
    unsigned char colour;           // +0x08
    unsigned char flag9;            // +0x09
    unsigned char count;            // +0x0a
    unsigned char kind;             // +0x0b
    char unknown_c[4];
    unsigned char* pixels;          // +0x10
    unsigned char* shade;           // +0x14
};

struct Src_4589c0 {
    unsigned short width;           // +0x00
    unsigned short height;          // +0x02
    unsigned short x;               // +0x04
    unsigned short y;               // +0x06
    char unknown_8[8];
    int bits;                       // +0x10
};

struct Surface_4589c0 {
    int width;                      // +0x00
    int height;                     // +0x04
    int pitch;                      // +0x08
    void* bits;                     // +0x0c
    int field_10;                   // +0x10
    int field_14;                   // +0x14
    unsigned short x;               // +0x18
    unsigned short y;               // +0x1a
    char unknown_1c[0x10];
    unsigned int flag0 : 1;         // +0x2c
    unsigned int flag1 : 1;
};

struct Bitmap_4589c0 {
    unsigned short width;           // +0x00
    unsigned short height;          // +0x02
    short dx;                       // +0x04
    short dy;                       // +0x06
    unsigned char colour;           // +0x08
    unsigned char flag9;            // +0x09
    unsigned char count;            // +0x0a
    unsigned char kind;             // +0x0b
    int unknown_c;                  // +0x0c
    void* field_10;                 // +0x10
};

class Class_00458310 {
public:
    void FUN_00458310(int* minX, int* maxX, int* minY, int* maxY, Model_4589c0* model,
                      int posX, int posY, int posZ);
};

class Class_00458d30 {
public:
    int FUN_00458dd0(Image_4589c0* image, Model_4589c0* model);
};

class Class_004c6ae0;

void __stdcall FUN_004b8a80(Surface_4589c0* dst, Src_4589c0* src);
void __stdcall FUN_004b7f90(Class_004c6ae0* dst, Bitmap_4589c0* bmp, int x, int y);

class Class_00459200 {
public:
    char unknown_0[0x10];
    Image_4589c0* bitmap;           // +0x10
    void FUN_004589c0(Image_4589c0* src, Model_4589c0* model);
};

// FUNCTION: 0x4589c0
void Class_00459200::FUN_004589c0(Image_4589c0* src, Model_4589c0* model)
{
    int minX = 0;
    int maxX = 0;
    int minY = 0;
    int maxY = 0;
    ((Class_00458310*)this)->FUN_00458310(&minX, &maxX, &minY, &maxY, model, 0, 0, 0);
    Child_4589c0* child = model->owner->firstChild;
    while (child != 0) {
        if ((child->flags & 0x20000) == 0) {
            int cminX = 0;
            int cmaxX = 0;
            int cminY = 0;
            int cmaxY = 0;
            ((Class_00458310*)this)->FUN_00458310(&cminX, &cmaxX, &cminY, &cmaxY,
                                                  child->model, 0, 0, 0);
            Owner_4589c0* owner = model->owner;
            int py = child->y - owner->y;
            int px = child->x - owner->x;
            int pz = child->z - owner->z;
            short ya = (short)(py >> 16);
            short yb = (short)(pz >> 16);
            int xoff = (short)(px >> 16);
            int v = ((ya >> 1) * 0xffff + yb) << 16;
            int yoff = (short)(v >> 16);
            int t;
            t = cminX + xoff;
            if (t < minX) minX = t;
            t = cmaxX + xoff;
            if (t > maxX) maxX = t;
            t = cminY + yoff;
            if (t < minY) minY = t;
            t = cmaxY + yoff;
            if (t > maxY) maxY = t;
        }
        child = child->next;
    }
    Image_4589c0* bmp = src;
    int x1 = bmp->width - bmp->dx;
    int y1 = bmp->height - bmp->dy;
    int x0 = -bmp->dx;
    int y0 = -bmp->dy;
    if (minX < x0) x0 = minX;
    if (x1 < maxX) x1 = maxX;
    if (minY < y0) y0 = minY;
    if (y1 < maxY) y1 = maxY;
    int newW = x1 - x0;
    int newH = y1 - y0;
    this->bitmap->width = (unsigned short)newW;
    this->bitmap->height = (unsigned short)newH;
    this->bitmap->dx = (short)(-x0);
    this->bitmap->dy = (short)(-y0);
    this->bitmap->colour = bmp->colour;
    if (newW == bmp->width && newH == bmp->height) {
        memcpy(this->bitmap->pixels, bmp->pixels, bmp->height * bmp->width);
        memcpy(this->bitmap->shade, bmp->shade, bmp->height * bmp->width);
    } else {
        short savedDx = bmp->dx;
        short savedDy = bmp->dy;
        bmp->dx = 0;
        bmp->dy = 0;
        Surface_4589c0 surface;
        FUN_004b8a80(&surface, (Src_4589c0*)this->bitmap);
        memset(this->bitmap->pixels, this->bitmap->colour,
               this->bitmap->height * this->bitmap->width);
        memset(this->bitmap->shade, 0, this->bitmap->height * this->bitmap->width);
        int dx = this->bitmap->dx - savedDx;
        int dy = this->bitmap->dy - savedDy;
        FUN_004b7f90((Class_004c6ae0*)&surface, (Bitmap_4589c0*)bmp, dx, dy);
        unsigned char* t = bmp->pixels;
        bmp->pixels = bmp->shade;
        bmp->shade = t;
        surface.bits = this->bitmap->shade;
        FUN_004b7f90((Class_004c6ae0*)&surface, (Bitmap_4589c0*)bmp, dx, dy);
        t = bmp->pixels;
        bmp->pixels = bmp->shade;
        bmp->shade = t;
        bmp->dx = savedDx;
        bmp->dy = savedDy;
    }
    ((Class_00458d30*)this)->FUN_00458dd0(this->bitmap, model);
}
