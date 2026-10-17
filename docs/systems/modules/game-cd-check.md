# Archive mounting, CD identification and content gates

## Scope and shared state

The `game/cd_check` group owns six entries in `0x41d4c0..0x41d920`, all in [cd_check.cpp](../../../src/game/cd_check.cpp). It selects startup archives, identifies campaign/multiplayer CD contents, builds one alternate data path, exposes two content-check globals and contains a separate padded allocation helper. The group is a source/address ownership unit, not six versions of one CD authentication mechanism.

Its three controls are distinct. `DAT_0050289c` enables archive registration here, makes CD identification return the current-directory marker, and changes downstream content acceptance. `g_cdBypassDriveScan` chooses a hardcoded drive probe inside actual CD scanning. `g_cdPathMismatch` is a sticky status read by unit compatibility code. The first global remains unnamed in the source; this reference does not assign it an unsupported original name.

The underlying [HPI layer](util-hpi.md) owns mounted metadata, first-source precedence, stream lifetime and shadow flags. [Data-file loading](game-data-files.md) owns ordinary language-sensitive paths. The module does not decompress archives itself, choose weapon targets or transmit a CD result over the network.

## Registration sequence and effective precedence

RegisterDataArchives (`0x41d4c0`) does nothing when `DAT_0050289c` is zero. When nonzero, it drops inaccessible registered archives, changes the process directory to the executable directory and performs these searches in order:

| Stage | Files and registration mode |
| --- | --- |
| Revision package | `rev31.GP3`, mode 1 |
| Expansion packages | Every visible `*.CCX`, mode 1 |
| Download packages | Every visible `*.UFO`, mode 0 |
| Main local archives | `*.HPI`, mode 0, until ten additions succeed or enumeration ends |
| CD archive roots | Every `*.hpi` at the root of each enumerated Windows CD-ROM drive, mode 0 |

Each enumeration is closed. After all stages, the routine explicitly resolves archive shadow flags. It does not restore the previous working directory. The integer passed to AddArchive is stream retention: mode 1 keeps an archive's stream open after metadata/member use, while mode 0 allows it to close and reopen lazily. It is not an explicit priority number. Priority comes from insertion order.

Duplicate paths and failed archive opens both return zero from AddArchive. They therefore do not consume the ten-success local-HPI allowance. Other stages have no corresponding ten-file limit. Existing valid mounts survive the initial drop-missing pass and retain their order, so repeating registration is not a clean rebuild of the mount list.

Searches use HPI's combined discovery API with source continuation enabled, rather than a direct isolated filesystem listing. Enumeration order within a pattern is not sorted here. The source establishes pattern-stage order, but an exact installation's same-pattern archive order depends on discovery. Loose asset files still win ordinary opens before mounted content. Among archives, earlier mounts win, while shadow resolution suppresses later duplicates during discovery.

CD archive enumeration mounts all matching root archives on every detected CD-ROM drive. It does not first require that drive's TOTALA.ID to identify a campaign or multiplayer disc. Archive discovery and content identification are separate calls.

## Identifying a requested disc

FindGameCdDrive (`0x41d6a0`) returns the character `.` immediately when `DAT_0050289c` is nonzero, before checking its side argument. Otherwise argument zero requests the Contents/Campaign integer and argument one requests Contents/Multiplayer; other values return zero.

Ordinary scanning calls FindNextCdDrive repeatedly, so drives are tested in ascending letter order. With `g_cdBypassDriveScan` enabled it instead probes lowercase `h` once and then ends, without asking Windows whether that letter is a CD-ROM drive. This flag does not mean “skip all content checks and succeed.”

For each candidate it formats a root TOTALA.ID path, constructs a TdfFile, loads it, selects Contents and obtains the requested integer with default zero. A nonzero value returns the drive character. Exhaustion returns zero. The check uses TDF/HPI file access, not a binary disc signature or volume-serial comparison.

There is also a test comparing the formatted path's first byte with the drive variable. A mismatch sets `g_cdPathMismatch` to one. Under ordinary successful formatting, the first byte comes directly from that same drive argument, so this body does not establish a normal route to mismatch. It never clears the flag. The getter at `0x41d8b0` returns its current value unchanged.

## Alternate path building and consumer checks

BuildCdFilePath (`0x41d7b0`) has a noteworthy branch direction. With `DAT_0050289c` zero it delegates directly to BuildDataPath, including ordinary language fallback. With the flag nonzero it asks FindGameCdDrive for request zero and then one until a nonzero character is returned. Because that callee returns `.` immediately in the same flag state, the normal result is the current-directory marker, without actual CD scanning.

This alternate branch formats the marker followed by two backslashes, the directory, a backslash and the name. It strips the existing extension unconditionally, then appends a dot and requested extension only when that extension is nonempty. This differs from BuildDataPath, which leaves an existing extension when no replacement is requested. A null extension is not guarded before strlen here. The nominal no-drive path clears the output and returns null, although the current nonzero-flag callee behavior ordinarily prevents reaching it.

The caller declaration in [frontend.cpp](../../../src/frontend/frontend.cpp) is void, while the implementation returns a character pointer; the caller uses the output buffer. This is another provisional declaration mismatch, not evidence of a second path-building implementation.

The raw global getter at `0x41d8a0` returns `DAT_0050289c`. Its consumers establish a broader content gate:

- [Weapon enumeration](../../../src/weapons/weapon_types.cpp) loads a TDF only when its parser flags identify accepted archive-backed content or this global is zero.
- [Unit type discovery](../../../src/units/unit_types_42a8d0.cpp) applies a related parser-origin test. It also clears compatibility and marks bad content when the global is nonzero and the definition is loose, or when the mismatch getter is nonzero.
- Unit version/copyright checks remain additional tests in that consumer. Passing the CD helper does not bypass those checks or guarantee multiplayer compatibility.

These are source-based acceptance rules. The exact accepted unit set still depends on mounted data, individual definitions and other unit compatibility checks described in [unit types](units-unit-types.md).

## Padded zero-allocation helper

AllocZeroedWithTickOffset (`0x41d8c0`) is owned by this group even though it does not read CD state. It computes padding as the Windows tick count modulo 1000, multiplied by seven. It allocates requested size plus that padding through operator new, zeroes the entire allocation, calls AllocNotifyNop on the allocation base, and returns base plus padding.

The returned pointer is an interior pointer unless padding is zero. This function does not return the original base or store a visible header for recovering it. Callers cannot infer that an ordinary delete/free of the returned pointer is valid. Padding can range from zero to 6993 bytes and need not preserve the allocation base's alignment. Total-size arithmetic has no explicit overflow guard, and allocation failure is not checked before memset. It introduces time-dependent address placement, not additional requested usable bytes beyond the declared size.

An unused declaration in GUI source is not proof of an active allocation call. No stronger purpose or actual freeing convention is established by this body alone. [Allocation adapters](util-operator-new.md) describe the underlying allocation family without changing this interior-pointer contract.

## Completion and runtime-dependent details

All six entries are covered: mounting, disc identification, alternate path construction, the two getters and padded allocation. No installed drive, archive inventory or disc contents were probed for this documentation. Exact archive order and content acceptance require those inputs; the branch direction, retention modes, search limits and error returns above follow directly from the existing source.
