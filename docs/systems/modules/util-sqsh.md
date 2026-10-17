# SQSH chunks, LZSS dictionaries, compression and obfuscation

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and consumers

The util/sqsh interval begins at `0x4d0ab0` and ends before `0x4d80b0`. Its 22 game-source entry bodies are in [sqsh.cpp](../../../src/util/sqsh.cpp). The interval also contains identified linked-library routines: [functions.csv](../../../data/functions.csv) names the zlib 1.0.4 compress/uncompress entries at `0x4d1c80`/`0x4d1d20` and their supporting codec routines. Their existing source is under the main checkout's toolchain/thirdparty/zlib-1.0.4 directory. Its wrappers, algorithm description and relevant stream/block code were inspected read-only; no library was downloaded, built or installed.

This group supplies raw LZSS streams and a separate SQSH chunk wrapper with optional byte obfuscation. [HapiBank](util-hapibank.md) uses SQSH for account bodies/string pools. [HPI reads](../../../src/util/hpi.cpp) unpack compressed archive chunks. [Network condenser code](../../../src/network/net_condenser.cpp) uses raw LZSS with a preset and its own packet framing, not a SQSH header on every packet.

Compression changes representation, not gameplay authority. It does not decide ship targeting, construction progress or whether a received unit command is valid.

## 2. LZSS shared workspace and locking

The raw codec uses a 4096-position ring window, allocated as 0x1011 zeroed bytes, and a flat 4097-node tree allocation of six-byte records. Each node has unsigned-short parent, smaller and larger links. Node 0x1000 supplies the root and zero is the absent-link/reserved position. A match can span at most 17 bytes. The extra allocation space and tree sentinel do not make every arbitrary index valid.

LzssCompress (`0x4d0f60`) and LzssExpand (`0x4d1480`) acquire the global ticket using the current Windows thread ID. A free ticket installs the owner. The same owner is permitted to enter again without arranging a second release; another owner waits on the event and retries. The invocation that acquired an initially free ticket clears owner/ticket and signals the event on completion.

This serializes ordinary use of shared workspace pointers, but does not stack/save those pointers for nested codec invocations. Same-thread reentry can overwrite a prior operation's globals despite the lock permitting it. Preset setup/toggling also has no corresponding outer lock in its own methods. Callers must coordinate their lifecycle rather than infer independent per-call codec objects.

LzssFreeWindow (`0x4d0ab0`) and LzssFreeTree (`0x4d0ae0`) free/null the corresponding CRT allocation and return zero. A null pointer prints a diagnostic and returns -1. Allocation/lock initialization helpers immediately before this module are owned by [sound/samples](sound-samples.md), including InitLzssLock and tree/window allocators. The raw codec also performs its allocations/cleanup directly.

## 3. Dictionary tree operations

LzssContractNode (`0x4d0b10`) replaces a node with its one child, updates the parent's child link and clears the retired node's parent. LzssReplaceNode (`0x4d0b80`) transfers an old node's structural position/children to a replacement and repairs child parent links. These operate on indices into the global flat tree, not allocated C++ child objects.

LzssFindNextNode (`0x4d0c10`) descends to the smaller child then follows larger links to its maximum. Despite successor-oriented comments, this is the largest entry in the smaller subtree, the in-order predecessor. LzssDeleteString (`0x4d0c50`) ignores a node whose parent is zero, contracts a one-child/leaf case or removes/promotes that predecessor for a two-child case.

LzssAddString (`0x4d0de0`) ignores position zero. It compares ring bytes at the new position with existing candidates, masked to 0xfff, up to 17 bytes. It keeps a match at least as long as the previous best, so later equal-length candidates can replace the chosen match index. A full-length match replaces that tree node; otherwise it descends according to the signed-char difference and links a new leaf at an empty child. The output argument receives the matched ring index, not a relative distance from the current cursor. Helpers assume consistent allocated tree/window state and do not perform general index/cycle validation.

## 4. Raw LZSS byte format

A flag byte controls the following eight tokens in low-bit-first order. A clear flag bit represents one literal byte. A set bit represents a little-endian 16-bit word: its high twelve bits select an absolute ring-window index, and its low four bits encode length minus two. Lengths are therefore two through seventeen. A zero high-twelve-bit index terminates the stream, regardless of that word's low length bits.

The source overview calls this field a distance back, but expansion actually reads window[(encodedIndex+i)&4095], without subtracting it from the current cursor. This is an absolute ring coordinate. Output bytes are inserted into the ring beginning at position one, wrapping at 4096. Match copies can overlap the newly written window region, reproducing repeated sequences as the copy progresses.

The raw format has no length/checksum/header of its own. LzssExpand receives no source-end or destination-capacity argument and reads until its terminator. Truncated flags/words, a missing terminator or insufficient output storage are not rejected before out-of-bounds accesses. A surrounding network/archive wrapper's size fields do not automatically become bounds inside this entry.

## 5. Compressing and expanding raw streams

Compression allocates both workspaces, installs a default zeroed dictionary or copies an enabled preset, and reads up to 17 bytes into look-ahead positions beginning at one. It emits a literal when the best match is at most one byte, otherwise the index/length word. It clamps a previously found match to remaining look-ahead count before emission. After consumed bytes it removes the ring position about to be reused, advances/refills look-ahead, wraps the cursor and inserts the next candidate string.

Full eight-token groups are emitted as one flag byte plus their literal/word data. At completion it inserts a zero-index terminator into a final group. The final token-count calculation emits one additional token after that terminator: a normally uninitialized literal slot, or for a terminator in bit seven an out-of-range local literal index 256. Expansion stops at the terminator before consuming that extra token, but the compressor's reported byte count includes it. Those trailing bytes can therefore affect wrapped checksums/output representation despite not adding decoded content.

On completion compression frees/nulls tree/window and returns bytes written. Expansion allocates only the window, loads an enabled preset when available, decodes, frees/nulls the window and returns output length. A window allocation failure returns -1 after releasing a newly acquired lock. Compressor tree-allocation failure also returns -1/releases its lock but leaves the previously allocated window unfreed. There is no destination-capacity test inside either raw codec. These failure results must be interpreted by callers; merely subtracting pointers is not a complete checked stream API.

## 6. Preset dictionaries

LzssSetPreset (`0x4d1670`) allocates fresh global window/tree, limits positive supplied length to at most 4000, installs its root seed, copies preset bytes beginning at ring offset 0x13 and inserts their positions into the tree. It snapshots the window and completed tree into static preset buffers, sets ready and frees the temporary workspaces. A negative length is not rejected before memcpy's unsigned size conversion. Tree allocation failure leaves the already allocated window and returns -1.

LzssEnablePreset (`0x4d1810`) sets the use flag; LzssDisablePreset (`0x4d1800`) clears it. Both return zero, and neither erases the saved dictionary or clears ready. Compress/expand use the preset only when both ready and use are set. Compatible sender/receiver preset bytes and ring positions are therefore necessary for raw compressed packets to decode consistently.

The network condenser brackets raw codec calls with enable/disable and supplies its own checksumming/obfuscation/framing. That does not install a SQSH header or turn these global flags into thread-local state. Preset construction and codec workspace mutation are not independently safe to overlap just because each regular compression call takes its lock.

## 7. SQSH packed header

SquashPack (`0x4d1820`) and SquashUnpack (`0x4d1970`) use this packed 19-byte prefix:

| Offset | Size | Field |
| ---: | ---: | --- |
| 0x00 | 4 | SQSH magic |
| 0x04 | 1 | Version, writer uses two |
| 0x05 | 1 | Compression method |
| 0x06 | 1 | Encryption/obfuscation flag |
| 0x07 | 4 | Compressed payload length, excluding this header |
| 0x0b | 4 | Expected decoded length |
| 0x0f | 4 | Sum of stored payload bytes |

Method one uses the raw LZSS codec above. Method two delegates to zlib. Although the method check rejects values at least four, zero/three have no implemented switch case and leave the local computed length uninitialized. Negative pack-method integers also pass that upper-only check before being truncated into the header byte. They are not a documented supported raw/no-compression alternative.

The unpacker checks magic, method bound, stored-payload checksum and final decoded-length difference. It does not validate the version byte against two. It receives no total source-buffer length, and caller-supplied destination storage is not paired with an independent capacity argument. Header lengths are trusted for checksum/decode work rather than range-checked against a containing allocation.

## 8. Packing and capacity failure semantics

The packer rejects null data, zero size and method at least four. Its attempted output validation tests whether chunk+1, the payload pointer, is null; a null chunk itself becomes payload address 19 and is not caught by that condition. It does not verify chunkSize's pointer or reject every negative size.

It compresses into the memory immediately following the header, then checks whether encodedLength+19 exceeds the input capacity in *chunkSize. The raw LZSS encoder has no capacity parameter. For zlib it initializes output length with the entire supplied chunk capacity rather than subtracting the 19-byte prefix. Therefore the post-compression check can report insufficient space after writes already occurred beyond the intended allocation; it is not a preventative bounded-write contract.

The zlib compression return status is ignored through its void-shaped local declaration. A failure that leaves length unchanged can become the generic space-error result. A raw LZSS -1 result is also not explicitly rejected; length+19 can become eighteen and proceed into invalid size/header behavior. Optional encryption of a negative interpreted length can further enter an unintended large unsigned loop.

After optional byte transformation, the packer fills magic/version/method/flags/sizes and sums the stored payload bytes into its checksum, copies the header and returns total size through chunkSize. Normal success is zero. Payload encryption is requested for any nonzero integer but the stored flag is one byte; callers must use compatible values rather than assume arbitrary nonzero integers survive truncation identically.

## 9. Unpacking, in-place decryption and error results

The unpacker copies the header first, verifies magic/method bound and sums exactly compressedSize stored payload bytes. A mismatch returns before decryption. With its flag set, it decrypts the payload in the caller's source buffer, then calls the selected decoder and compares its output length with the expected header size. It does not restore encrypted source bytes afterward. Repeating an encrypted chunk decode on the same mutated buffer can fail the original ciphertext checksum.

For method one the comparison uses LzssExpand's returned count. For method two it initializes length to expected size and calls zlib uncompress, ignoring its return status. The existing uncompress wrapper updates *destLen only after a successful complete stream; early errors leave it unchanged. Consequently this SQSH path can return success for a failed zlib decode because the unchanged length still equals expected size. It does not faithfully expose every zlib data/memory/buffer error as a SQSH error.

| SQSH result | Concrete branch meaning |
| ---: | --- |
| 0 | Normal completion, or the retained uninitialized/ignored-status cases described above |
| 1 | Wrong magic during unpack |
| 2 | Stored payload byte-sum mismatch |
| 3 | Decoded length differs from header length |
| 4 | Method byte/input at least four |
| 5 | Packer's post-write total exceeds supplied capacity |
| 6 | Packer's limited pointer/zero-input checks reject input |

These are wrapper results, not a complete exception-safe validation schema. Malformed method zero/three, source lengths and decode overrun can fail outside these explicit branches.

## 10. zlib method-two behavior

The existing zlib 1.0.4 compress.c initializes a fresh stream with default compression, finishes it in one request, records total output only after stream end and releases stream state. Default level maps to six in deflate.c. uncompr.c similarly initializes inflate, requests a complete stream, records output length only at stream end and frees state on completion/error. Buffer, memory and data errors are explicit library statuses even though the game wrappers do not consistently honor them.

This path uses zlib-framed DEFLATE, rather than the game's 4096-byte absolute-index LZSS stream. The existing algorithm description specifies a 32-KiB prior-data window, matches up to 258 bytes, three-byte hash-chain candidate lookup, bounded searches and lazy match selection at the default level. Literal/length symbols and distances use separate Huffman encodings. Blocks can be stored, fixed-code or dynamic-code; the block reader rejects the fourth block type and checks stored-block length complements.

The zlib stream layer checks its method/window/header fields, supports the preset-dictionary request path and verifies its Adler check through the inflate state machine. Those library format checks are separate from the SQSH wrapper's simple stored-byte sum and from the raw game's preset dictionary. The game-level method-two wrapper does not itself supply a requested zlib preset dictionary. Existing source/library identification is the evidence here; no external current zlib implementation was substituted or tested.

## 11. Header, size, checksum and obfuscation helpers

SquashMaxPackedSize (`0x4d1aa0`) estimates 120 percent of input plus 119 for mode one, or 110 percent plus 119 for mode two; other modes return zero. Multiplication is unsigned 32-bit and can wrap before division. It is a sizing heuristic used by callers, not a runtime bound enforced inside raw compression.

SquashGetPackType (`0x4d1b00`) returns one for wrong magic, otherwise the method byte clamped to four. Wrong magic therefore shares its numeric result with method one. SquashUnpackedSize (`0x4d1b40`) returns the integer at header offset 0x0b for correct magic, otherwise zero. Neither validates total header length, version, payload checksum or the plausibility of the returned size.

SquashEncrypt (`0x4d1b70`) transforms byte i by XOR with i, then addition of i, retaining the low byte. SquashDecrypt (`0x4d1ba0`) subtracts that low-byte index and XORs it back. Both mutate their buffer and return zero. This deterministic 256-byte repeating index transform has no secret key or authentication and is representation obfuscation, not a cryptographic protection layer.

SquashChecksum (`0x4d1bd0`) sums unsigned payload bytes into an integer. It is distinct from [ComputeChecksum's four byte lanes](util-math.md). SquashDumpHeader (`0x4d1c00`) prints method, encryption flag and packed/unpacked sizes to stdout; it does not validate or dump the entire payload. SquashErrorString (`0x4d1c60`) returns a table pointer for code<7 and null otherwise. A negative code also passes that signed upper-only test and can index before the table; it is not a checked arbitrary-integer lookup.

## 12. Coverage, lifetime and limits

All 22 game-source entries were read and globally reconciled, together with the linked-library identification, existing zlib one-shot wrappers/algorithm/stream-block code and bank/HPI/network consumers. Tree/workspace allocations use CRT calloc/free, caller buffers remain borrowed and decrypt mutates source. SQSH has no automatic destination allocation or general malformed-stream containment boundary.

The module's raw formats, match indices, final-token behavior, chunk schema, ignored library errors and preset/lock limits are documented as retained implementation. There is no match save orchestration or replicated gameplay decision here. This reference uses existing files only, without builds, installation, execution, source listings or claims of successful round-trip testing.
