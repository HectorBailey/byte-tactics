# HapiBank accounts, typed items, binary boxes and file format

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and ownership model

The util/hapibank group owns 25 functions in [hapi_bank.cpp](../../../src/util/hapi_bank.cpp), `0x4b3630` to before `0x4b4f10`. HapiBank::InitBank at `0x4b3620` belongs to the preceding [integer-map address group](util-int-map.md) and is documented there. [hapi_bank.h](../../../src/util/hapi_bank.h) supplies the shared interface used by save/load consumers.

HapiBank is an in-memory hierarchy with explicit current account/current box state. Its four-byte object points to a 12-byte AccountList containing account count, account array and current-account index. Each 24-byte account owns its name, item count/array, box count/array and current-box index. Each 16-byte item owns a name and a typed value: integer, double or owned string. Each 20-byte box stores named/numbered identity, byte size, cursor and owned data pointer.

Names are compared case-insensitively. Numbered and named boxes occupy the same account array but are distinct namespaces, so a numeric ID and a matching text representation are not automatically the same box. Items and boxes are also separate collections. This is not the HPI directory/archive tree even though both use HAPI file services.

## 2. Fresh banks, selection and cleanup

InitBank sets the bank pointer null and returns this, without allocation. NewBank (`0x4b3750`) closes any old bank, calloc-allocates a 12-byte list and sets current=-1. It uses that allocation without checking for null. A new bank has no selected account; callers must select/create one before box operations.

OpenAccount (`0x4b4560`) finds a case-insensitive name, selects it, resets its currentBox=-1 and returns one. A missing name grows/zeros a new account, duplicates the name, selects it with currentBox=-1 and returns zero. Zero therefore means newly created, not an allocation-failure status. Selecting an existing account does not discard its contents or reset all stored box cursors.

CloseBank (`0x4b3630`) frees every account name, item name, string-typed value, box name for named boxes, box data, item/box arrays, account array and list. Integer/double values require no separate allocation. It does not set the object's bank pointer null afterward. Repeating CloseBank without reinitialization/replacement can therefore revisit freed memory. NewBank/OpenBank immediately install replacement state after their close, while higher wrappers close then delete fresh objects once.

The header does not declare an automatic C++ destructor that performs these calls; consumers explicitly use CloseBank. [Save orchestration](game-settings-campaign.md) records that ownership order. Closing invalidates borrowed item strings and box contents. The bank does not retain live pointers to unit objects as owning C++ references.

## 3. Finding and setting typed items

FindItem (`0x4b4910`) returns -1 for a null bank or no selected account. Otherwise it scans item names, returning an existing index. A missing name returns -1 unless create is requested; creation grows the array by one, zeros the new item and duplicates its name. New items start with type zero. Allocation/name-copy results are not independently checked before use.

HasItem (`0x4b48f0`) tests whether noncreating FindItem found an index. It does not require a particular item type or nonempty string.

SetIntegerItem (`0x4b4630`), SetDoubleItem (`0x4b46c0`) and SetStringItem (`0x4b4750`) require a bank/current account, find-or-create the named item and free an old string value before replacing its type/value. They return one for the performed assignment or zero when the account prerequisite is absent; the string setter also returns zero for a null supplied value. Empty nonnull strings are stored as copied strings.

The setters do not preserve an old value if allocation of its replacement fails. In particular, SetStringItem frees the old string before duplicating the supplied pointer. Supplying a pointer borrowed from that same old string can therefore make its input dangling during replacement. Callers must copy such an alias before changing the item. Numeric setters do not convert a string's textual number; they replace its type outright.

GetIntegerItem (`0x4b4800`), GetDoubleItem (`0x4b4850`) and GetStringItem (`0x4b48a0`) return the caller's default when bank/account is absent, name is missing or type does not match exactly. They perform no cross-type numeric/string conversion. A returned stored string is borrowed bank storage; a returned default is the caller's original pointer. Neither is a newly allocated copy.

## 4. Finding and opening binary boxes

FindNumberedBox (`0x4b49d0`) and FindNamedBox (`0x4b4a80`) return -1 without bank/account or when a missing identity is requested without creation. Creating grows/zeros a new box and installs its numeric identity or duplicated case-insensitive name. A fresh box has size/cursor zero and no data allocation.

OpenNumberedBox (`0x4b4b50`) and OpenNamedBox (`0x4b4ba0`) always ask the corresponding finder to create if missing, store the resulting index as currentBox and return whether its data pointer is nonnull. They do not return the box index, reset its cursor, clear its contents or distinguish new creation from an empty existing box through a dedicated status. They assume a valid current bank/account; a finder result -1 is not safely propagated before indexing.

This behavior explains common load patterns: opening a missing box can create an empty one, then GetBoxSize returns zero and the subsystem's own size test rejects it. A successful-looking open is not proof that expected persisted bytes exist. Reopening a previously read/written box retains its previous cursor unless SeekBox is called.

## 5. Box cursors, reads, writes and nontruncation

GetBoxSize (`0x4b4bf0`) returns the selected box's byte size without checking bank/account/box validity. SeekBox (`0x4b4c10`) clamps a requested cursor to zero through current size. SeekBoxEnd (`0x4b4c50`) sets cursor=size and returns that endpoint. No function in this group provides a general truncate-box operation.

ReadBox (`0x4b4c80`) computes available=size-cursor. Nonpositive availability returns zero. Otherwise it reduces a requested length to availability when needed, memcpy-copies bytes and advances cursor by that length, returning the copied count. It does not supply a string terminator or require the requested structured record to be complete. Subsystem readers must compare the returned count with their expected size.

WriteBox (`0x4b4cf0`) computes cursor+length, reallocates when that exceeds size, updates size to the larger endpoint, copies bytes and advances cursor, returning length. Writing inside existing contents leaves later bytes and the old overall size intact. Reopening a box and writing a shorter record does not replace/truncate its old payload; without a seek it can instead append from the retained cursor. NewBank/fresh-box save construction is the ordinary way callers avoid such stale tails.

Lengths and arithmetic are signed integers. Negative lengths, overflow, null buffers and invalid selections are not turned into safe error results before memcpy. A negative read request below positive availability is not rejected and can become a large unsigned copy length. Allocation failure on growth is also not separately recovered. These are trusted in-process buffer services, not a checked arbitrary-input stream abstraction.

## 6. On-disk file header and string pool

OpenBank (`0x4b3770`) and SaveBank (`0x4b39c0`) use a packed 34-byte header:

| Byte offset | Field | Meaning |
| ---: | --- | --- |
| 0x00 | Eight-byte magic | HAPIBANK |
| 0x08 | Name offset | Bank identifier within the unpacked string pool |
| 0x0c | Pool file offset | Start of the final string-pool image |
| 0x10 | Header size | Offset of first account; writer uses 0x22 |
| 0x14 | Version | Writer uses one; loader requires one |
| 0x18 | Compressed byte | Whether final string pool is compressed |
| 0x19..0x21 | Unused bytes | Zeroed by the writer |

Names and string values are appended as NUL-terminated byte strings to a common pool. Records store pool-relative integer offsets in fields locally typed as pointers. They are not literal valid process pointers on disk. Saving appends strings as encountered and does not deduplicate repeated equal names or values.

The bank identifier, such as Total Annihilation 3.0, is compared case-insensitively when a nonnull expected name is supplied on load. It is distinct from the fixed HAPIBANK magic and version one. There is no cryptographic integrity/authentication check in this format wrapper. Compression is delegated to the SQSH helpers, whose algorithm is a separate module.

## 7. Account body layout

Each account starts with a 32-byte header containing total encoded account size, pool-relative account-name offset, integer/double/string/blob counts, an account-compressed flag and an unused word. Its body stores item classes in this order, regardless of their insertion order in memory:

| Class | Record size | Contents |
| --- | ---: | --- |
| Integer | 8 | Name pool offset and integer value |
| Double | 12 | Name pool offset and eight-byte double |
| String | 8 | Name pool offset and string-value pool offset |
| Box descriptor | 16 | Name pool offset or negative marker, numeric ID word, data offset, data length |

Box payloads follow the complete descriptor array in descriptor order. A negative name field means a numbered box; a nonnegative field is a pool-relative name offset. Box data offsets are calculated from the account's original uncompressed file position. When an account body is compressed, the loader reconstructs the payload address by subtracting the original account base/header from that stored offset and adding the unpacked body pointer.

For a named box, the writer does not initialize the otherwise unused numeric-ID descriptor word before writing all 16 bytes. That word is not consulted when the name marker is nonnegative. Unknown/type-zero items are not serialized as any of the three typed classes. Only boxes with positive size produce descriptors/payloads.

SaveAccount (`0x4b3c60`) skips an account when both in-memory itemCount and boxCount are nonpositive. An account containing only empty boxes or uninitialized items can still emit a header/name with no body records. LoadAccount does not create an account when body length is nonpositive, so such empty account identity is not guaranteed to survive a round trip. Current account, selected box and box cursors are not stored as separate persistent selection state.

## 8. Opening a bank and preserving/replacing old state

OpenBank opens through HAPI, reads 34 bytes and rejects mismatched magic or version by closing the file and returning zero. Those early failures leave the existing in-memory bank untouched. It then loads all bytes from poolOffset to file end as the pool. A compressed pool is sized/decompressed through SQSH, copied into its final temporary allocation and its intermediate allocations freed. A decompression error formats a message and calls FatalError rather than returning zero.

If a nonnull expected identifier does not match the pool string at nameOffset, it closes/frees temporary data and returns zero while retaining the old bank. Once accepted, it closes the old bank, allocates a fresh list and begins loading accounts from headerSize until the file cursor reaches poolOffset. It is not a transactional parse into a second complete bank before replacing old state.

An optional account-name filter is passed to every LoadAccount call. Nonmatching accounts are skipped by their total size; matching ones are reconstructed. OpenSummaryBank uses this to avoid restoring all binary state just to inspect Summary. Even filtered loading still reads/decompresses the shared string pool and scans account headers.

The loader does not systematically check short reads, offsets against file/pool size, NUL termination of pool strings, nonnegative lengths, count multiplication or progress of every malformed account. For example, an invalid skipped account size can fail to move the cursor forward. Recognized header/version is therefore not complete structural validation. After its loop it closes/frees temporary storage and returns one, including a validly parsed bank with no loaded accounts.

## 9. Reconstructing one account

LoadAccount (`0x4b4270`) records account file base, reads its header and calculates base+size as the next-account endpoint. A filter mismatch seeks directly to that endpoint. For positive body length it reads raw bytes or, only for compressed flag exactly one, unpacks them. A SQSH error is fatal and includes the file-handle name in the error message.

It opens/creates the named account and walks integer, double, string and box descriptors using the counts. Setters duplicate item names/strings into bank-owned storage. Each box is opened, its payload copied from the unpacked body's calculated location and its cursor reset to zero. It then seeks the file to the encoded account endpoint and frees the temporary body.

No independent bounds check verifies each record/payload range against the unpacked length. Duplicate account/item/box identities can merge through the find-or-create methods instead of being rejected. An already existing box in a duplicate account can retain its cursor before another WriteBox, since opening alone does not rewind it. The writer normally emits a consistent hierarchy, but the loader does not enforce a unique-name schema on arbitrary external data.

## 10. Writing accounts and optional compression

SaveAccount writes a placeholder header, appends names/values to the pool, writes typed records and descriptors/payloads, then calculates the uncompressed account size. When its compress argument is nonzero, it temporarily disables the out-of-memory handler, tries to allocate/read the just-written body and compress it using SQSH. A successful smaller result overwrites the raw body, changes size/compressed flag and truncates the file at the shorter endpoint. Failure or a non-smaller result keeps the original body. Temporary compression allocations are freed and the prior out-of-memory handler is restored.

Finally it rewrites the account header at its original offset and seeks to the file end. The named-box offsets retain their original uncompressed coordinate interpretation and are adjusted as described by the loader. Integer/string records and double packing are explicit; this is not a raw dump of BankAccount pointers.

Individual fread/fwrite/fseek/truncation results are not aggregated into a SaveAccount success status; it returns void. Allocation and string growth for ordinary record/pool construction are likewise generally trusted, even though optional compression tries to fall back on allocation failure.

## 11. Writing the full bank and audit side effects

SaveBank returns zero when there is no bank/no account, or when fopen of the destination in w+b mode fails. Otherwise it writes a placeholder file header, calls SaveAccount for every in-memory account and records the final pool offset. It always attempts string-pool compression and chooses it only for successful smaller output; the supplied compress flag governs account compression, not whether the pool attempt occurs.

The pool-compression allocation temporarily disables the out-of-memory handler and restores it before packing. A retained allocation-failure branch assigns err from an uninitialized local; the later compressed-output condition also requires a nonnull allocation and otherwise writes the raw pool. This is recorded as existing source behavior, not repaired documentation pseudocode.

It rewrites the final header, closes the file, frees pool/compression storage and returns one. Write/seek/close failures are not tested along that path, so one is not a verified complete durable disk write. Opening w+b truncates an existing file before construction; there is no backup/temporary-file atomic replacement in this body.

When audit is requested, it first derives a .cpa path by copying the filename, stripping its extension and appending .cpa, then calls WriteAuditFile. Its 256-byte path is built with unbounded string operations. The audit can be attempted even if the later main save fails to open.

WriteAuditFile (`0x4b4d70`) opens text write mode, returns silently on failure, then prints account count, names, typed item values, unknown item types and each named/numbered box's byte count, followed by an EOF marker. It closes the stream. It does not hex-dump box contents, escape strings into a reloadable source format, validate hierarchy consistency or report success to its caller.

## 12. Consumers and completion

[Game save orchestration](game-settings-campaign.md) assembles Summary plus subsystem accounts. [Unit persistence](units-unit-save.md), [COB state](units-cob.md), [players](game-players.md), [mapping](map-line-of-sight.md), [features](map-features.md), [plot maps](map-plot-map.md), [meteors](map-meteors.md), [surfaces](graphics-surface.md) and resource accounts use typed items/boxes for their own records. Their schema and reference checks are not performed automatically by HapiBank.

All 25 owned bodies, storage structs, compression call contracts, cursor/selection state, file layouts, error/ownership paths and audit output were inspected. There is no live unit simulation, network protocol, current audio voice or construction display decision in this persistence container. No bank was created or executed for verification. Documentation uses existing source only, without builds, installations or copied source listings.
