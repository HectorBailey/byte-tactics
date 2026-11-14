// ArchiveDirectory: the header of an HPI archive directory, at the offset the
// archive keeps in its index. count is the number of entries and entries the
// offset of the first one, from the archive base. Each file keeps its own view
// of the archive itself; this is the one declaration of the directory for the
// files that read it.
#ifndef ARCHIVE_DIRECTORY_H
#define ARCHIVE_DIRECTORY_H

#include "archive_entry.h"

struct ArchiveDirectory {
    int count;                           // +0x0
    ArchiveEntry* entries;               // +0x4
};

#endif  // ARCHIVE_DIRECTORY_H
