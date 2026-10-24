// ByteMap: a player's explored map, a byte per cell with the grid's width and
// height in MapSize behind the pointer. network/player.h spells the same three
// members at +0x7c (explored, exploredWidth, exploredHeight); the files that
// use the helpers include this.
#ifndef BYTE_MAP_H
#define BYTE_MAP_H

struct MapSize {
    unsigned int width;                // +0x0
    unsigned int height;               // +0x4

    int Contains(unsigned int col, unsigned int row)
    {
        return col < width && row < height;
    }
};

struct ByteMap {
    unsigned char* data;               // +0x0
    MapSize size;                      // +0x4

    int Index(int x, int y) { return size.width * y + x; }
    unsigned char Get(int x, int y) { return data[Index(x, y)]; }
    int Contains(int x, int y)
    {
        return (unsigned int)x < size.width && (unsigned int)y < size.height;
    }
    unsigned char& at(int x, int y) { return data[y * size.width + x]; }
};

#endif
