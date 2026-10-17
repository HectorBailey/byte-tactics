# HPI archives, file handles, discovery and package writing

## Scope and implementation ownership

The `util/hpi` group owns 58 annotated entries from `0x4bb190` up to the next group at `0x4be950`. It provides a combined view of ordinary files and mounted HPI archives, directory enumeration, whole-file loading, drive/path helpers, archive lifetime, shadow resolution and package generation. It supplies assets to definition parsers, maps, graphics, sounds and scripts. It does not interpret those assets or choose the startup archive search order.

The implementation is [hpi.cpp](../../../src/util/hpi.cpp), with separate bodies for [entry lookup](../../../src/util/hpi_4bb4e0.cpp), [directory lookup](../../../src/util/hpi_4bc800.cpp), [archive data writing](../../../src/util/hpi_4bd830.cpp), [shadow resolution](../../../src/util/hpi_4be320.cpp) and [StringRef vector insertion](../../../src/util/hpi_4be6c0.cpp). Shared declarations are [FileHandle](../../../src/util/file_handle.h) and [ArchiveEntry](../../../src/util/archive_entry.h). The companion insertion method belongs to this address group despite being generic container support.

Compression is delegated to [SQSH](util-sqsh.md). The [TDF parser](util-tdf.md) consumes returned asset bytes; the [HapiBank persistence format](util-hapibank.md) is a separate format and API. Similar names do not imply that bank accounts are HPI archive members.

This reference describes source behavior, including unchecked operations and provisional declaration inconsistencies. The division by 65,536 in the archive code counts byte blocks. Comments describing those quantities as pixels or texture blocks do not describe the actual file contract.

## State, layout and ownership

| Object | Relevant representation | Ownership and purpose |
| --- | --- | --- |
| Archive header | 20 bytes: four-byte `HAPI` magic, version bytes `00 00 01 00`, metadata allocation size at offset 8, stored obfuscation byte at 12, root-directory offset at 16 | The loaded allocation contains the header and all directory/name/file-info metadata. File contents remain on disk. |
| Directory | Eight bytes: entry count and entries offset/pointer | Offsets are relative to the beginning of the metadata allocation before relocation. |
| Archive entry | Packed nine bytes: name offset/pointer, data offset/pointer, flags | Flag value 1 denotes a directory, value 2 denotes a shadowed entry. Data points to another directory or a file-info record. |
| File info | Packed nine bytes: physical file-data offset, logical uncompressed size, compression byte | Its physical data offset remains an on-disk offset after metadata relocation. |
| Mounted archive | `0x118` bytes: shared CRT stream, cached physical position, metadata allocation, open-member count, open mode, 260-byte full name | Owns metadata and its stream. Member handles borrow metadata and the shared archive object. |
| File handle | `0x118` bytes: loose stream, archive pointer, file-info pointer, unsigned logical cursor, block-size array, decoded-block cache, 256-byte diagnostic name | Owns its block array/cache. A loose handle owns its stream; an archive member contributes to the archive's member count. |
| Display context | Archive-pointer list at `+0x618`, count at `+0x61c`, saved start directory at `+0x628`, last directory at `+0x728` | Owns archive registration order and two 256-byte directory buffers. |

The member's logical cursor and the mounted archive's physical cursor are different state. Several members can share one archive stream, so reads reposition that stream when necessary. There is no synchronization in this layer around its shared stream, cursor or mount list. The implementation assumes access is coordinated by callers.

Removing or closing a mounted archive does not wait for its member count to reach zero. Doing so while a member survives invalidates its borrowed archive and metadata pointers. Conversely, closing a member does not unregister an archive. Archive teardown and member teardown have separate contracts.

## Opening files and selecting an asset

`HAPI_OpenFile` at `0x4bb2e0` allocates and clears a handle, copies the diagnostic name with a forced terminator at byte 255, and first attempts ordinary `fopen` using the requested mode. A successful loose open wins immediately. If it fails, the function searches mounted archives in registration order.

For each archive it looks up the requested path, rejects missing entries and directories, and accepts the first file entry. Lookup is case insensitive and splits components on backslashes. Each component scans directory entries backwards, so a duplicate name within one directory resolves to its last matching entry. There is no slash conversion, parent-directory interpretation or canonical path normalization in the archive lookup body.

If the winning archive stream is closed, it is reopened in binary read mode. A reopen failure frees the candidate handle and returns null immediately, even if a later archive might contain the same path. On success, the archive member count increases, its file-info pointer is borrowed and its logical cursor starts at zero. Compression causes immediate allocation/read of the block-size table. Neither that table read nor the allocation is fully validated here.

Direct opens do not test the shadow flag. Their precedence follows loose open success and archive order independently of enumeration flags. This matters after changes to loose files or mounted archives: stale shadow flags can affect discovery while direct opening still follows its normal priority.

`HAPI_OpenFileRead` at `0x4bb5b0` forwards binary-read mode; `HAPI_OpenFileAppend` at `0x4bb2c0` forwards append/update mode. Their local definitions are `void` wrappers, whereas callers elsewhere declare the read entry as returning a handle. The practical forwarding path is visible, but the source declarations are inconsistent and should not be treated as a clean typed API. Append mode normally creates or opens a loose file, so archive fallback does not make archived bytes writable.

`HAPI_CreateFile` at `0x4bb6a0` explicitly creates a loose file with binary write/update mode, clearing its handle and terminating the copied name. It returns null if the stream cannot open. `IsInArchive` at `0x4bb650` tests the shared-archive pointer. `FileHandle::SetFileName` at `0x4bb670` only changes the bounded diagnostic name; it does not move or rename the underlying file.

## Seeking, reading and block caches

| Operation | Loose file | Archive member |
| --- | --- | --- |
| Seek, `0x4bb710` | Absolute `fseek`, except argument -1 seeks to end; returns new `ftell` position or -1 | Argument -1 selects logical size; other signed arguments become the unsigned cursor without bounds checking; returns zero |
| Tell, `0x4bb7a0` | `ftell` | Logical cursor |
| Length, `0x4bbd00` | CRT `_filelength` | Cached file-info logical size |
| Read, `0x4bb7c0` | `fread` byte count | Logical-size clamping followed by raw or block decoding |
| Write, `0x4bbbe0` | `fwrite` byte count | Returns all-one bits, conventionally -1 |

An archive seek discards its decoded block when the upper 16 bits of the logical position change. It does not load the destination block. Thus seeking inside the cached 64 KiB block retains it; crossing a block boundary frees it. The caller must not interpret the archive seek return as the new position.

An uncompressed member maps its logical position to the file-info physical offset plus that position. If this differs from the shared stream's cached physical position, the stream seeks. It reads the requested bytes, reverses outer archive obfuscation if enabled, and advances both logical and physical positions by the actual byte count.

A compressed member begins with four bytes per logical 64 KiB block, rounded up from its uncompressed size. Each table value is the packed byte length of one SQSH chunk. The first chunk follows the table; later chunk positions are found by summing previous packed lengths. Opening the member caches the entire table, not its contents.

For an uncached block, reading allocates a 64 KiB decoded buffer plus a temporary buffer of the table's packed length. It seeks and reads exactly that packed length, updates the shared physical cursor, reverses outer obfuscation, and invokes SQSH unpacking. A short packed read frees the buffers and returns -1. SQSH failure takes the fatal-error path using a 1000-byte local diagnostic buffer and messages identifying the member, archive and block. Successful packed input is freed while the decoded block remains cached.

The reader copies bytes up to the block boundary, then uses its seek logic to advance and evict when necessary. One block can serve repeated small reads. A request spanning blocks repeats allocation/decode for each block; there is no multi-block cache or prefetch. A failure after earlier blocks were copied can leave a changed cursor and partially filled caller buffer even though the call returns failure.

Requests are clamped against logical size, but invalid positions and zero-byte reads are not generally safe no-ops. The block-count expression includes position plus requested length minus one. At a block boundary a zero-length request can underflow into a very large iteration count; at EOF it can select a block outside the table. Seeking beyond EOF can also make the remaining-size arithmetic negative. Normal callers must maintain a valid logical range. Corrupt archive sizes, block lengths and SQSH output are not comprehensively bounded by this layer.

### Two independent transformations

Outer archive obfuscation uses the low byte of the absolute archive position, a runtime key byte and bitwise inversion. It applies to metadata after the first 20 bytes, compressed block tables, packed chunks and raw file contents. An all-zero runtime key disables this transformation.

The stored header key is transformed on load by rotating it left two bits and complementing it, when the stored byte is nonzero. The writer derives a stored byte by complementing a right rotation of the requested key. A requested key byte of 255 produces stored key zero, which the reader treats as disabled, although the writer still transforms with its nonzero requested key. That edge value therefore does not round-trip through this convention. The compressed writer also enables SQSH's inner payload transformation. A compressed read therefore reverses archive-position obfuscation first and then lets SQSH verify/decrypt/decompress its own chunk. These are separate stages; the archive layer has no independent checksum for an entire member or directory tree. See [SQSH error and checksum behavior](util-sqsh.md) for the inner format's limits.

## Closing and whole-file adapters

`HAPI_CloseFile` at `0x4bb5d0` expects a valid non-null handle. For a member it decrements the archive count. When the count reaches zero and archive mode is zero, it closes the shared stream and clears that stream pointer. Nonzero mode retains the stream for subsequent members. For a loose file it closes the private stream. Both paths free the block table, decoded cache and handle. The returned close status does not provide general archive-integrity validation.

Several adapters inline the same cleanup rather than calling this entry. Their buffer contracts differ:

| Entry | Result and ownership details |
| --- | --- |
| Length by name, `0x4bbc40` | Opens, obtains size and closes; zero conflates missing files with empty files. |
| Read at offset, `0x4bbd30` | Opens and seeks, reads into caller storage, closes; any positive read returns that storage, even if shorter than requested. Failure does not transfer or free the caller's buffer. |
| Load file, `0x4bbe50` | Initializes optional output length to -1, rejects missing or nonpositive-sized files, allocates exactly logical size, rewinds/reads/closes. Any positive read is accepted; output length reports the declared full length even after a partial read. Returned bytes have no extra NUL terminator. |
| Load open file, `0x4bbff0` | Similar allocation/read behavior but keeps the supplied handle open, moving its cursor. Failure length is unsigned all-one bits. |
| Load into caller buffer, `0x4bc120` | Has no supplied buffer capacity. An early missing/empty-file failure retains caller storage. A negative read retains it, but a zero read after opening/reading frees it. A positive partial read returns it. |
| Write buffer by name, `0x4bc290` | Opens loose binary-write mode, writes and closes. Returns -1 on open failure or the bytes written; close errors are ignored. |

These helpers do not all mean â€œcomplete successful transfer.â€ In particular, a positive partial read can be promoted to apparent whole-file success. LoadFile's allocation-tag preparation also copies the input path into a fixed local buffer without a bounded copy before selecting its final backslash-separated component.

Rename at `0x4bbc10` and remove at `0x4bbc30` are thin CRT filesystem wrappers with discarded results. They do not edit archive metadata, rename archive members, refresh shadow flags or update existing handle names.

`HAPI_CopyIntoFile` at `0x4bcf80` opens a source through the combined file view, measures it, rejects zero size, rewinds and allocates a 100 KiB copy buffer. Each iteration reads up to that fixed buffer size and requires a positive read plus an exactly matching destination write count. The destination write therefore must support loose-file writing. Failure releases the buffer and source handle and returns zero, leaving any partial destination bytes. Success returns the original source length and closes the source, but does not free the copy buffer in the traced body. It also assumes the measured source length remains consistent while copying.

## Wildcards and directory enumeration

`HAPI_MatchWildcard` at `0x4bc370` performs case-insensitive matching with `?` for one character and `*` for zero or more. It maintains a bounded set of 100 active pattern positions. A star retains its state and opens a following state; final acceptance permits pattern end or a trailing single star. There are no bracket classes, escape grammar or Unicode rules. Branches beyond the fixed state capacity are ignored, so unusually complex patterns need not behave as an unbounded matcher would.

FindFirst at `0x4bc4b0` splits the last backslash into directory and leaf pattern and allocates a search record containing two 256-byte strings, source state, a continuation flag and a physical-search handle or archive directory/index. For archive matching, `*.*` becomes `*`. A starting state of -1 begins with CRT physical enumeration. If that fails, continuation-enabled searches advance to archive zero. A nonnegative starting state begins at that archive index.

FindNext at `0x4bc640` drains the physical enumeration first, then closes it and switches to archives if continuation permits. For each archive it finds the directory, walks entries and accepts wildcard matches whose shadow flag is clear. Archive files report attribute value 1 and their logical size; directories report attribute value `0x11` and zero size. Archive timestamps are zero. Returned names are leaf names, not complete paths.

The argument often named `recursive` here controls continuation through sources, including physical-to-archive fallback and later archives. It does not itself traverse subdirectories. With continuation disabled, archive enumeration stops after its selected archive. Actual directory recursion belongs to a different helper.

Directory lookup at `0x4bc800` descends through components preceding backslashes and returns the list containing the final component. It deliberately leaves the final component unconsumed, because that component can be the wildcard. Entry lookup at `0x4bb4e0` consumes the final component and returns its entry. Confusing the two would incorrectly turn a filename or pattern into another directory.

FindClose at `0x4bc8d0` rejects null/-1 search handles, closes a live physical enumeration when applicable, and frees the search record. CountDirectoryEntries at `0x4bc930` skips `.` and `..`; its nonzero selection flag counts directories only. ListDirectory at `0x4bca30` similarly selects entries and appends leaf StringRefs to an existing vector. It neither clears nor sorts that vector. GetDirectoryEntry at `0x4bcd10` walks to an index through the same search machinery but exposes no boolean success result, so an absent index cannot be identified from a returned status.

FindFilesRecursive at `0x4bcb50` enumerates a path with `*`, descends into non-dot directories, matches files against its supplied pattern and appends full paths. It passes source state into child searches to control which source is being traversed. Its fixed local path buffer and recursive depth are not independently guarded.

The owned StringRef vector insertion at `0x4be6c0` supplies the collection operation used by discovery. It retains/copies the four-byte string handles, shifts elements or allocates larger storage, and releases old elements/storage as required. Growth follows the existing MSVC vector implementation, using the old capacity plus the larger of old size and insertion count when reallocation is necessary. Its ownership is strings and vector storage, not loaded file contents.

## Archive registration, metadata loading and shadowing

OpenArchive at `0x4bdd70` opens a physical archive in binary-read mode, allocates its record, records a full path, and reads the 20-byte header. It checks the magic/version and a trailing copyright string whose four year characters are normalized before comparison with `Copyright 0000 Cavedog Entertainment`. This suffix check is a format gate, not a cryptographic authentication check.

It allocates the header's declared metadata size, rereads that many bytes, derives the runtime obfuscation key, decodes bytes after the header, and relocates root/entry/name/data offsets into pointers. RelocateDirectory at `0x4be010` recursively adds the metadata allocation base to directory entry arrays and entry fields. File-info data offsets themselves remain physical file offsets. Relocation is destructive and intended to occur once.

Header/table reads, metadata sizes, directory counts and offsets are not comprehensively checked before dereferencing. A matching magic/version/footer does not establish that all internal pointers or block lengths are valid. Mode zero closes the physical stream after metadata is loaded, leaving member opens to reopen it lazily; nonzero mode retains it.

CloseArchive at `0x4be070` accepts null, otherwise closes its stream and frees metadata plus the archive record. It does not unlink the pointer from the display's mount list. AddArchive at `0x4be0b0` resolves a full path, compares case-insensitively against registered names, opens and appends a new archive. Duplicate and failed additions both return zero. It preserves insertion order and does not automatically resolve shadows.

DropMissingArchives at `0x4be180` tests archives whose streams are currently closed by reopening them. A failed reopen removes/frees that record and shifts following pointers. Successfully tested streams are closed again. Archives with already-open streams are not revalidated. RemoveArchive at `0x4be270` removes by pointer identity, frees the matching archive and shifts the list without shrinking its allocation or checking live members.

Shadow resolution at `0x4be320` first clears shadow flags throughout every mounted tree, then enumerates from the combined root. ClearShadowFlags at `0x4be3b0` recursively clears flag value 2. MarkShadowedFiles at `0x4be400` walks directories and, for each visible file, marks matching file entries in later sources. A loose file shadows all archives; a file in archive N shadows matches in archives after N. Directory entries are used for traversal rather than marked as competing file payloads.

The resulting priority is loose files, earlier mounted archives, then later mounted archives. Enumeration suppresses marked duplicates. Opening applies the same intended priority by searching sources directly, without consulting those flags. Additions, removals and loose-file changes need an explicit shadow-resolution pass when the caller wants discovery flags refreshed.

## Package generation

PackDirectory at `0x4bd160` builds metadata in a growing allocation, reserves the header, recursively describes entries, opens the destination, writes placeholder metadata, writes file data, rewrites the completed metadata and appends a copyright suffix with the local calendar year. It reports callback progress at zero, five and 95 around these stages. Failure to open the destination frees metadata and returns zero. The eventual success return does not prove all writes or inner compression calls succeeded.

BuildArchiveDirectory at `0x4bd3b0` enumerates each directory twice: first to count entries and reserve packed entry slots, then to append names and recursively build directories or nine-byte file-info records. These records initially carry observed sizes and zero physical offsets. Its enumeration uses the combined loose/archive view, so packaging a directory can include mounted content, not only physical files beneath that directory. Two passes assume the source set remains stable; changes between them are not reconciled by a snapshot.

WriteArchiveData at `0x4bd830` reopens each recorded source through HAPI, obtains its current size, records the destination physical offset and writes according to the low byte of the flags argument:

| Compression byte | Data layout |
| --- | --- |
| Zero | Raw source copied in chunks of up to 4 KiB, with outer archive obfuscation when requested |
| One | Block-size table followed by SQSH LZSS chunks of at most 64 KiB logical input each |
| Two | Same block/table layout with SQSH zlib chunks |
| Other nonzero values | Still select the compressed branch, but rely on SQSH methods that are not generally valid; this is not an extensible codec dispatch with validation |

The compressed path writes a placeholder size table, allocates maximum packed storage plus a 64 KiB source buffer, packs each block with inner transformation enabled, writes the packed chunks, then seeks back and rewrites the completed table. Outer position-based obfuscation is applied separately to both table and chunks. It does not fall back to raw data when compression expands the source.

The writer ignores source-read counts, SQSH pack results and destination-write counts. It also dereferences the reopened source without a null guard. A source disappearing or changing size between metadata discovery and writing therefore is not handled as a clean transactional failure. The raw path shares the ignored-read/write behavior. Package output is written directly to its destination, with no temporary-file replacement or rollback.

After each file, its source handle is released and the progress callback receives a value derived from that file's output start offset, the metadata size and the total input bytes counted earlier. That is not a decoded-byte completion ratio. Compression and input changes can distort it; a zero total input size leaves a division-by-zero possibility when an empty file reaches this callback. The callback should not be interpreted as a validated monotonic percentage.

Once file data is written, PackDirectory encodes metadata bytes after the header, writes the final metadata over the placeholder, appends the year-bearing footer and frees the allocation. PackageBuildNullStub at `0x4bd150` simply returns zero; it adds no alternate writer or recovery path.

## Drive and working-directory helpers

FindNextCdDrive at `0x4bb190` uppercases its argument, begins after a valid supplied A-to-Z drive or at A otherwise, and returns the first Windows CD-ROM drive letter or zero. Volume helpers at `0x4bb200` and `0x4bb260` query Windows volume information and return a serial initialized to zero. The former also fills a caller name buffer. They ignore the Windows success flag, so zero can indicate query failure as well as a serial value.

Current-drive retrieval at `0x4bc2e0` converts the CRT drive number to a letter. ChangeDrive at `0x4bc300` rejects null but otherwise derives a CRT drive number from the first character without uppercasing it. GetDriveDirectory at `0x4bc320` uses the requested drive or current drive, calls `_getdcwd`, ignores its status and returns the supplied buffer. ChangeDirectory at `0x4bc360` discards the CRT change result.

SaveStartDirectory at `0x4bce10` records the current drive's directory in the display context. SetLastDirectory at `0x4bce60` changes directory and copies its argument into the last-directory field only on success. GetLastDirectory at `0x4bcea0` and GetStartDirectory at `0x4bcec0` copy their respective 256-byte fields. These bounded copies do not force a final NUL when input fills the field. RestoreStartDirectory at `0x4bcee0` changes to the saved directory and ignores failure.

MakeDirectoryPath at `0x4bcf00` copies a path into a 260-byte local buffer without bounded copying. At every slash or backslash it temporarily terminates the prefix, attempts to create that directory and restores a backslash, then attempts the complete path. It ignores creation results and does not special-case drive prefixes or supply path normalization. These routines mutate process-wide filesystem state rather than any per-member archive cursor.

## Completion boundary and unresolved inputs

All 58 owned entries are accounted for across file/drive helpers, lookup, compressed/raw transfer, discovery/container support, package creation, metadata/mount lifetime and shadowing. This module has no unit targeting, simulation-tick, particle, wireframe or multiplayer protocol policy. Its contribution to those systems is supplying bytes and directory discovery under the priority rules above.

Archive ordering and search locations belong to startup data-file management, while asset validity and interpretation belong to consumers. Actual installed archive inventories, compression ratios, CD volume labels and runtime filesystem contents cannot be inferred from this source alone. No archive was generated, mounted or executed to prepare this reference, and no claim is made that the unchecked paths have been dynamically tested.
