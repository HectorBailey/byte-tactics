// Header: the 0x14-byte block at the start of an HPI archive: the magic, the
// version, the size of the whole archive, whether the data is obfuscated, and
// the offset of the directory.
#ifndef HPI_HEADER_H
#define HPI_HEADER_H

struct ArchiveDirectory;

struct Header {
    char magic[4];                     // +0x0
    unsigned char version[4];          // +0x4
    unsigned int size;                 // +0x8
    unsigned char obfuscate;           // +0xc
    char unknown_d[3];
    ArchiveDirectory* list;            // +0x10
};

#endif  // HPI_HEADER_H
