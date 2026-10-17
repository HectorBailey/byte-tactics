# TDF parsing, record access, translations and shared field storage

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

## 1. Scope and address ownership

The util/tdf group contains 62 annotated entries across two intervals: `0x4c2ea0` to before `0x4c4fa0`, then `0x4c5170` to before `0x4c5d90`. Ownership was checked globally, not inferred from filenames. Implementations occupy [tdf.cpp](../../../src/util/tdf.cpp), [file loading](../../../src/util/tdf_4c2f60.cpp), [buffer loading](../../../src/util/tdf_4c3120.cpp), [destruction](../../../src/util/tdf_4c51b0.cpp), [translation tables](../../../src/util/tdf_4c54f0.cpp), [field insertion](../../../src/util/tdf_4c59d0.cpp) and four emitted methods in [runtime/basic_string.cpp](../../../src/runtime/basic_string.cpp).

The group includes a parser, file/current-record interface, typed field reads, record checksums, global localization, StringRef field-pair lifetime, vector support and emitted exception/string methods. The adjacent basic_string::_Copy gap starts a different module and is not counted here. [tdf.h](../../../src/util/tdf.h) is the limited interface most game modules see.

TDF supplies configuration for units, weapons, terrain features, maps, scripts, sound categories, sides and UI data. Parsing those files does not itself execute their game behavior. Naval hull settings, target masks and shipyard rendering rules are interpreted by their individual consumers.

## 2. Tree and storage model

TdfFile contains root, current and flags in twelve bytes. TdfRecord is a packed 0x29-byte object containing an owned name, child-pointer vector, sorted field vector behind a comparator byte, and a trailing field provisionally named text. That last field stores a computed checksum through a pointer-shaped declaration, not a surviving pointer into the temporary parse buffer.

Children remain in source order. Fields are TdfField pairs of StringRef handles, eight bytes per key/value pair, sorted by a case-insensitive key comparison. The record name is duplicated separately through the game allocator. Parsed names/values own copied/reference-counted character storage, so freeing the temporary input copy after construction does not invalidate a successfully constructed tree.

TdfFile's constructor (`0x4c2ea0`) clears root/current/flags. Its destructor (`0x4c2eb0`) and Unload (`0x4c3240`) delete the root and clear all three fields. TdfRecord's destructor (`0x4c42a0`) frees its name and recursively deletes children; automatic member destruction releases child-vector storage and both string handles of every field. The deleting-record entry at `0x4c32f0` additionally handles the containing allocation. The field deleting/destructor entries at `0x4c51b0`/`0x4c5190` supply the analogous generated cleanup for pair storage.

## 3. File loading, buffer loading and failure replacement

LoadFile (`0x4c2f60`) opens through HAPI and returns zero on open failure. A nonpositive reported file length also yields zero after closing. For positive length it allocates a raw buffer, seeks to zero and reads. A negative read result leaves the prior tree intact and returns zero; any nonnegative result enters parsing, even a short read or zero bytes read into the requested allocation.

On that path it records HAPI_IsInArchive into flags, deletes the old root, clears root/current, allocates size+1 text bytes, copies the entire requested size and appends NUL. It strips comments, constructs a root record named root, frees text/raw buffers, closes the file and returns one. Since the received byte count is not required to equal size, unread raw bytes can enter the parse. Root allocation failure can leave root null while the local result still becomes one. It does not parse into a second complete tree and atomically swap on success.

LoadBuffer (`0x4c3120`) deletes the old root immediately, clears selection, installs its caller-supplied flag and performs the same copied/NUL-terminated/comment-stripped construction from data/size. It has no result value, total-allocation bound or general negative-size validation. The supplied flag is not automatically proof of HAPI provenance; this method accepts the caller's integer.

Both paths expect trusted asset bytes and successful ordinary allocations. Parser syntax errors call FatalError rather than return a partial recoverable result. The archive/file service and memory-allocation modules determine their own failures; this layer does not install retries or a parser cancellation mechanism.

## 4. Comment removal

StripComments (`0x4c33a0`) mutates its NUL-terminated buffer. Double-slash comments are blanked up to newline or NUL; newline is retained. Slash-star comments are blanked through the matching closing pair, including that pair. An unclosed block is consumed through end of text without a distinct unterminated-comment error.

The scan does not recognize quoted-string boundaries. A URL-like double slash or slash-star sequence inside a supposed value can therefore be removed as a comment. Block-comment replacement can also blank newline characters. The parser does not preserve a separate original-source buffer for precise line/column error reporting.

## 5. Recursive grammar and error paths

TdfRecord's constructor (`0x4c3e40`) skips only space, tab, carriage return and newline. A leading opening bracket finds the next closing bracket, trims the enclosed subrecord name and requires an opening brace after whitespace. It recursively constructs that child and appends its pointer in source order. A child returns through a supplied nextblock pointer positioned immediately after its closing brace.

Other ordinary text is interpreted as a field: strchr finds an equals sign, the preceding slice becomes a trimmed key, then the next semicolon terminates a trimmed value. The parser inserts/updates that pair and continues. It does not implement quoted escapes, semicolons protected by quotes, include directives, expression evaluation or a schema of permitted keys. Newlines are whitespace rather than field terminators. Empty trimmed keys/values are not rejected by a dedicated nonempty rule.

A closing brace ends the current record. When parsing the synthetic root without nextblock, that branch can also return early and ignore trailing text; it does not independently reject every unmatched top-level closing brace. NUL at a nested level is fatal because the child has not supplied its closing block boundary. NUL at root is ordinary completion.

The explicit fatal cases are missing closing bracket, missing opening brace after a subrecord name, nested end-of-file, missing equals sign and missing semicolon. Their message includes record name and filename when provided. The 2000-byte error buffer is extended by unbounded string formatting, so very long names/paths are not covered by a safe diagnostic-length guarantee. Searches for delimiters are not separately bounded to a lexical field/block before strchr finds them.

## 6. Trimming, duplicate fields and record checksums

MakeTrimmedString (`0x4c4340`) skips the same four whitespace bytes, returns an empty handle for an empty slice, finds the final non-whitespace character and temporarily writes NUL after it while copying into StringRef, then restores the original byte. This temporary mutation occurs in the copied parse text for ordinary loaders. It does not remove quote characters or decode escape sequences.

Field insertion lower-bound searches case-insensitively, but reuse tests exact strcmp equality. Exact repeated keys at the selected lower-bound position replace their value. Differently cased equivalents can coexist in the sorted equivalence range; later lookup returns its first member. This resembles the retained [command-registry case mismatch](util-commands.md), and must not be described as uniformly last-wins case-insensitive merging.

At a record end the constructor sets end=current-1 and, when its pointer test passes, computes the byte checksum with length end-text-1, effectively excluding the last two bytes before the delimiter/end position. Very short spans yield nonpositive lengths and therefore zero through ComputeChecksum. ComputeRecordChecksum (`0x4c43f0`) separately accepts start/end and supplies end-start-1 when start<=end. These exact length expressions are not a generic hash of every semantic key/value.

The checksum is calculated after comments were blanked and retains sensitivity to other textual bytes/order/spacing within the included range. It is not a cryptographic digest or a recomputed tree-content hash. The pointer-shaped text field stores that integer result; input text is then freed by the loader.

## 7. Current-record navigation

SelectRecord (`0x4c3410`) searches a named child of current, or of root when current is null. Matching ignores case. It replaces current with the result and returns whether found. SelectRecordAt (`0x4c3490`) similarly indexes that node's child vector. Their prechecked child accessor normally returns null for an invalid/negative unsigned-converted index; the presence of emitted vector::at exception code is not proof that every ordinary out-of-range call throws.

A successful selection changes the base for the next selection. Iterating root siblings therefore requires ResetCurrentRecord (`0x4c3e10`) between indexed selects, as asset loaders do. A failed select clears current, making the following select start from root again. There is no parent-navigation operation in this interface.

GetCurrentRecord (`0x4c3e20`) returns the raw current pointer through an integer. SetCurrentRecord (`0x4c3e30`) installs an arbitrary integer-shaped pointer without verifying membership. Immediately after load, root exists but current remains null; selecting/resetting does not transfer ownership. Calling selectors with no valid root can dereference null.

## 8. Record/field enumeration and typed getters

CopyRecordName (`0x4c4420`) uses strncpy with the supplied count and does not force termination on a full copy. GetRecordName (`0x4c4440`) returns the borrowed name pointer through an integer. GetSubRecordCount (`0x4c4450`) returns vector size. FindSubRecord (`0x4c4470`) scans source-order children for the first case-insensitive name. GetSubRecord (`0x4c44c0`) returns null for an unsigned out-of-range index before vector::at.

GetFieldCount (`0x4c45c0`) returns the field count. GetFieldName (`0x4c45e0`) rejects negative/too-large indices and returns the indexed key while iterating the sorted field vector. It therefore enumerates stored key order, not original text insertion order. Returned names/values are borrowed and tied to tree lifetime.

FindFieldValue (`0x4c4630`) case-insensitively lower-bound searches a key and returns its stored value pointer or zero. GetFieldInt (`0x4c46c0`) and GetFieldDouble (`0x4c4760`) apply atoi/atof when a field exists; the caller default is used only for absence. A present malformed numeric field is not replaced with that default by explicit validation here.

GetFieldFixed (`0x4c4800`) converts a found value with atof, multiplies by 65536 and casts to int, writing through its output pointer and returning that pointer. Its absent-field default is already the raw integer/fixed-point value supplied by the caller; it is not multiplied again. Finite/range checks are not added before conversion.

GetFieldString (`0x4c48c0`) copies a found value with strncpy, writes NUL at dst[size-1] and returns one, including an existing empty value. Size zero underflows that terminator index. For absence it strcpy-copies a nonnull default without enforcing size, otherwise leaves destination unchanged, then returns zero. Thus the found and default branches have different capacity rules. [Sound-choice loading](sound-sound-types.md) depends on the found/missing result, while many unit/weapon consumers supply known bounded defaults.

## 9. Global translation loading and cache behavior

LoadTranslations (`0x4c54f0`) compares requested section/language against g_language case-insensitively and returns immediately when equal, before checking filename or table availability. A same-language request for a different file is not a reload. For a changed language it destroys/frees the old table, allocates an empty replacement, copies the language into its 256-byte global and tries to load the supplied TDF.

It iterates the file's top-level records. Each record name becomes a translation key, and its field named by the current language supplies the value. Missing/empty values are omitted. The translation table is sorted with plain strcmp, and exact duplicate keys replace the selected value. It resets the TDF current pointer between records and unloads the temporary file afterward.

The table/language are replaced before file success is known. Failure leaves an empty table and the new remembered language, so repeating the same language can return without retrying that failed file. The function does not restore the old language/map. Names/values use 256-byte locals with copy limits 255; CopyRecordName does not independently force termination at that limit. Global language copying itself uses unbounded strcpy. No separate translation-shutdown function appears in this module.

## 10. Translation, reverse lookup and localized fields

Translate (`0x4c5740`) returns null for null input. With no table or no exact case-sensitive match it returns the original input pointer. A found key returns borrowed table value storage. It performs no allocation, Unicode conversion or fallback-language search. Reloading translations invalidates assumptions about previously borrowed table pointers.

TranslationTable::FindLowerBound (`0x4c5c60`) implements the same case-sensitive sorted search. This differs from the parser's case-insensitive field lookup. FindTranslation (`0x4c5840`) instead scans values case-insensitively and returns the first matching key pointer, or zero. It does not guarantee unique translated text or an inverse mapping for every visible caption.

GetLocalizedString (`0x4c58a0`) builds language+key with no inserted separator in a 256-byte local and asks the current TDF record for that field first. Only a missing result triggers the plain-key attempt. A present empty localized field returns success and suppresses that fallback. A supplied default, or the shared empty default when null, is used for both reads. Unbounded concatenation and GetFieldString's default-copy rules remain relevant limits; the current record must already be valid.

## 11. Field-pair and vector support

ReleasePair (`0x4c5170`) releases value then key. TdfField destruction at `0x4c5190`, its deleting wrapper at `0x4c51b0`, AssignPair (`0x4c5470`), copy construction (`0x4c54a0`) and two-handle construction (`0x4c54d0`) preserve the shared ownership of both strings. None frees the underlying character allocation twice merely because keys/values share handles; reference counting is implemented by [StringRef](graphics-surface.md).

The child-pointer vector insertion (`0x4c4d70`) grows/shifts pointer entries but child deletion remains the record destructor's responsibility. Field-vector insertion (`0x4c51e0`) grows/shifts owning handle pairs. Translation insertion (`0x4c59d0`) preserves an index across reallocation and returns the inserted slot. GetPairCount (`0x4c5ba0`) reads vector length, returning zero for null begin.

The explicit vector helpers are destruction (`0x4c5b70`), uninitialized copy (`0x4c5bc0`), uninitialized fill (`0x4c5c20`), forward pair assignment (`0x4c5cd0`), backward pair assignment (`0x4c5d10`) and placement copy construction (`0x4c5d60`). Reallocation copies/constructs into new storage, destroys old pairs and frees old storage. In-place insertion uses backward assignment to avoid overwriting its source range. These behaviors were traced in the source companions and existing MSVC VECTOR header, not reconstructed by compiling.

## 12. Emitted exception and standard-string support

Eight owned exception entries come from the existing STDEXCEPT header: logic_error construction (`0x4c35c0`), what (`0x4c3730`), virtual raise (`0x4c3740`), deleting destruction (`0x4c38f0`) and copy (`0x4c3950`), plus out_of_range raise (`0x4c3af0`), deleting destruction (`0x4c3c60`) and copy (`0x4c3cc0`). They retain/copy an error message string, expose its c_str through what, raise the appropriate concrete exception and release message/object storage on destruction. They are library support, distinct from the parser's FatalError process-exit path.

Five owned basic_string entries were traced in the existing XSTRING header: copy constructor (`0x4c49a0`), _Tidy (`0x4c4ac0`), erase (`0x4c4b10`), _Eos (`0x4c4c30`) and _Grow (`0x4c4c50`). The library string has its own byte reference count before character data, length and capacity; it is not the game's integer-count StringRef type. Copy can share compatible unfrozen storage. _Tidy releases/decrements an owned reference and resets fields; _Eos updates length and writes the terminating character.

erase validates its start, detaches shared storage, bounds removal to the remaining length, moves the suffix and updates termination. _Grow checks maximum size, detaches shared buffers, handles zero-size release/empty behavior and calls the adjacent _Copy helper when capacity/trim policy needs new storage. The adjacent helper and other runtime string routines remain separately inventoried; documenting this ownership boundary avoids silently promoting their module as well.

## 13. Coverage and limits

All 62 owned entries were covered through the seven game/runtime files, shared declarations and the existing VECTOR/STDEXCEPT/XSTRING source. The reference distinguishes tree/translation/string ownership, parser failure, typed-default behavior, current-record selection and generated support. Input-dependent asset values remain the responsibility of consumers and actual files.

There is no TDF text writer, persisted parser cursor, multiplayer packet or unit-target decision here. Parsed trees and localization pointers are runtime assets rebuilt by their callers. Documentation uses static source tracing without builds, installs, matching checks, game execution or copied source listings.
