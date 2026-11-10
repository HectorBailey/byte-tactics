// MapSize: a map's width and height in tiles, with the bounds test the
// explored- and byte-map lookups run before indexing. Its own header because
// several files read a map's size and its bounds without wanting map/byte_map.h
// and the ByteMap helpers that come with it; byte_map.h includes this.
#ifndef MAP_SIZE_H
#define MAP_SIZE_H

struct MapSize {
    unsigned int width;                // +0x0
    unsigned int height;               // +0x4

    int Contains(unsigned int col, unsigned int row)
    {
        return col < width && row < height;
    }
};

#endif
