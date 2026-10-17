# Asset paths, shared resources and model texture binding

## Scope and dependencies

The `game/data_files` group owns 22 entries from `0x429000` to `0x42a8d0`, all annotated in [data_files.cpp](../../../src/game/data_files.cpp). It converts logical resource names into language-sensitive paths, loads several common resource types, caches shared animation files, binds model texture names, prepares global effect/interface references and computes supplemental unit checksums. It also owns an emitted TdfFile deleting destructor at its final address.

This module uses [HPI file resolution](util-hpi.md), [TDF records](util-tdf.md), [GAF frames/sequences](graphics-gaf.md), [PCX decoding](graphics-pcx.md), [model rendering and 3DO loading](graphics-model-render.md), [sample loading](sound-samples.md) and the [unit definition table](units-unit-types.md). Mounting archives is a separate CD/startup responsibility. Selecting a resource here does not decide whether a weapon can target a submarine, how a shipyard builds, or which particle emitter runs. Those consumers interpret the loaded references.

## Language-sensitive path construction and version checking

BuildDataPath (`0x4290f0`) obtains the preferred-language string and, when non-null, first constructs a directory suffixed with a hyphen and that language, followed by a backslash and resource name. For a nonempty requested extension it removes the existing extension and appends the requested one. It attempts to open that candidate through HPI and closes the successful probe. Only a successful probe accepts the localized path.

Otherwise it constructs the ordinary directory/name path, applying the same extension replacement. It returns that fallback without probing it. Thus a returned path does not guarantee an existing resource. A null or empty extension leaves the name's current extension alone. The fixed caller buffers are filled with unbounded string operations; buffer capacity is not an API argument.

The source comment calls the suffix a side, but its producer is GetPreferredLanguage. This is language selection, not faction selection. HPI then applies loose-file and archive precedence within the chosen path. An available localized candidate can therefore override common content regardless of whether the common content also exists.

CheckGpfVersion (`0x429000`) resolves `gamedata/version.tdf`, loads it and selects the Version record. A found GPFVersion field must compare case-insensitively equal to `v3.0`; otherwise it opens a translated warning about Revision.GPF compatibility. A successfully loaded file with a missing record or field also warns. Failure to load the file bypasses this warning body entirely. The check does not reject startup, inspect archive signatures, compare every asset or establish multiplayer compatibility.

## Individual file adapters

| Entry | Selection and result |
| --- | --- |
| LoadBitmapByName, `0x429290` | Resolves `bitmaps/name.PCX`, invokes PCX loading with the supplied palette argument and reports the path through FatalError when loading fails. |
| LoadFontByName, `0x4292e0` | Resolves `fonts/name.FNT`, loads raw whole-file bytes and uses FatalError for a null result. Font interpretation occurs elsewhere. |
| LoadPaletteByName, `0x429330` | Tries `palettes/name.PAL` using file length as the existence test; otherwise decodes `palettes/name.PCX` into a new 1024-byte palette, attempts to write the PAL cache and removes common PALETTE ALP/LHT/SHD derivative files. |
| LoadRadarPic, `0x4295b0` | Opens the supplied path, reads a 64-byte header, and only when header byte `+0x2c` has bit 0 set seeks to the offset at `+0x28`, reads width/height and allocates/reads a RADARPIC frame. |
| LoadFileWithProgress, `0x429660` | Allocates the measured full length, reads ten chunks of integer size length/10 and then the remainder, setting the loading-progress byte to 9 through 90. |

Palette loading treats an empty PAL as absent. Existing PAL bytes are loaded without a separate 1024-byte size validation here. The fallback path allocates that size explicitly and regards failed PCX palette decoding as fatal. Cache writes and derivative removals discard errors, so a successfully returned palette does not guarantee updated on-disk caches. These filesystem changes are behavior of the game implementation, not operations performed while preparing this documentation.

RadarPic returns null on open failure or when its format bit is absent. If both output-coordinate pointers are non-null, it copies the dwords at header offsets 4 and 8; supplying only one pointer fills neither. Reads and dimensions are not comprehensively validated before allocation or multiplication. The preliminary length query does not itself bound the later reads. The returned frame owns newly allocated pixels independently of the closed file handle.

LoadFileWithProgress does not check for exact chunk counts. A negative read can reduce its accumulated offset, and the final remainder read's result is ignored. For files shorter than ten bytes it performs zero-length reads, which are unsafe for compressed HPI members under the boundary conditions explained in the archive reference. Its progress is a loading display, not transfer-integrity evidence. Its allocation is exactly the file length, without an added text terminator.

## Sound-name registration

LoadSoundByName (`0x429470`) maintains parallel game arrays: loaded sample pointers, 32-byte logical names and 32-byte source filenames, plus a count. With a non-null logical name, reuse requires an existing nonempty logical name matching case-insensitively within 32 bytes. With a null logical name, reuse compares filenames instead. Reuse returns the existing index without reloading.

A new registration calls LoadSoundFile, copies its filename, stores the logical name or an empty string, increments the count and returns the former count as index. It refuses registration when count plus one is at least 256, returning zero. Consequently the table never fills every physical slot, and zero is both a possible index and the full-table result. The bounded copies do not force a terminator for 32-byte inputs. No explicit rejection of a null loaded sample appears in this adapter; playback and sample lifetime are described in [game sound](sound-sound.md) and [DirectSound samples](sound-samples.md).

The two reuse modes are intentionally different. Reusing the same filename under a new logical name need not share the prior registration, whereas reusing a logical name with another filename returns its old registration.

## Animation-file cache and global resource references

LoadAnimGaf (`0x429700`) searches the game's animation-cache records by case-insensitive logical name. Each record contains a 64-byte name and a GAF allocation pointer. A cache hit returns that pointer. A miss resolves `anims/name.GAF`, loads it, takes FatalError on null, reallocates the record array by one 68-byte record, copies the name without a length guard and appends the pointer.

Cached names are logical names rather than resolved paths or file timestamps. Changing preferred language or source files does not automatically invalidate an existing cached entry. FindGafSequence (`0x429850`) forwards to FindGafEntry, but its local definition is void. As with other provisional forwarding declarations, callers must be traced rather than inferring a clean result contract solely from this wrapper's signature.

LoadGameResources (`0x429870`) obtains the shared files below and stores borrowed entry pointers into the game context:

| Animation file | Requested entries and role |
| --- | --- |
| fx | `smoke 1`, `smoke 2`, `fire1`, `alfboom1`, `radlogo`, `radlogohigh`, `nuclogo`, `h2oboom2`, `lavasplash`, `cannonshell`, `plasmasm`, `plasmamd`, `ultrashell`, `flamestream`, `explosion`, `explode2` through `explode5`, `nuke1`, `shadow` |
| igtitles | `igvictory`, `igdefeat`, `igpaused` |
| vismasks | `vismask` |
| fog | `Black1` through `Black4`, `Gray1` through `Gray4` |
| cursors | `cursorattack`, `cursorairstrike`, `cursortoofar`, `cursorcapture`, `cursordefend`, `cursorrepair`, `cursorpatrol`, `cursorpickup`, `cursorteleport`, `cursorrevive`, `cursorreclamate`, `cursorload`, `cursorunload`, `cursormove`, `cursorselect`, `cursorfindsite`, `cursorred`, `cursorgrn`, `cursornormal`, `cursorhourglass`, `pathicon` |
| Per-side intgaf value | `PANELTOP`, `PANELSIDE`, `PANELBOT` |

Two game fields intentionally request `plasmasm`. For alfboom1, the water/lava splashes, explosion, explode2 through explode5 and nuke1, loading also clears the byte at entry offset 2. This is a specific entry-state mutation, not a general “start every animation” call. Missing entry pointers are not checked before these stores.

The same routine reads language-resolved `gamedata/sidedata.TDF`. GENERAL/baseheight defaults to 480 when the record or field is absent. It then selects SIDE0, SIDE1 and so on until the first missing record. When intgaf exists, it loads that animation file and stores its panel file and three entries. The loop does not independently enforce the five-element panel-array capacity visible in this source layout. Its contiguous record rule means later sides after a gap are ignored.

FreeAnimFiles (`0x42a010`) frees every cached GAF allocation and the cache table, then resets count/pointer. It clears per-side panel file slots and six top-level animation-resource fields. It does not individually free their borrowed entry pointers, and many individual sequence fields remain stale values until resources are loaded again. Rendering must respect the surrounding resource lifetime rather than treating a nonzero old pointer as ownership.

## Texture archives and model binding

LoadTextureGafs (`0x42a440`) resolves `textures/*.GAF`, counts entries and assumes exactly one is `logos.GAF`. It stores blockCount as count minus one, allocates count pointer slots, enumerates all results and loads every file except logos. The pointer adjustment in the logos branch keeps non-logo files packed together. Progress is computed using count minus one; a directory containing only a non-logo file can reach division by zero, and missing/multiple logo assumptions can invalidate blockCount relative to filled slots. Count and enumeration must also remain consistent between passes.

The routine resets the global animation-play pointer count and pointer to zero. It does not first free an older pointer array, so ordinary lifecycle ordering is required. Logos are provided through separately managed game fields, not loaded by this loop.

LoadObject3d (`0x42a2c0`) resolves `objects3d/name.3DO`, loads it, reports missing data as fatal, mirrors/prepares its geometry and invokes BindModelTextures (`0x42a140`). Binding recurses into both model child links before processing each current object's elements. Every non-null texture-name field is resolved against loaded texture GAFs in order, then against the logos GAF if no ordinary texture matched.

Binding clears flag value 4 first. A logos match with exactly ten frames sets that flag, identifying the special logo family. An unresolved texture sets flag value 1 and color/type value `0xd1`. The search variable is left uninitialized before the ordinary-texture loop, so zero loaded texture blocks do not provide a reliable initialized miss value.

A matched entry with more than one frame initializes a GAF sequence directly over the element's former name storage and sets flag value 2. For ordinary animated textures, the address of this embedded sequence is appended to the animation-play pointer list; special logo sequences are excluded from that list. A one-frame entry instead stores its frame pointer and clears flag value 2. This is destructive binding: the name slot becomes frame or sequence state, so repeating binding as though names were still present is not a supported reload operation.

AddAnimplayPointer (`0x42a0e0`) is the general append helper for the same array. It reallocates by one pointer and stores the supplied address. The list borrows sequence addresses inside models; it does not own those models. The game lifecycle loads textures before unit/weapon model definitions and destroys those owners before freeing textures, as shown in [LoadBattleAssets and battle teardown](../../../src/game/game_state.cpp).

FreeTextureGafs (`0x42a570`) frees blockCount GAF allocations, clears their slots, frees the block-pointer table and frees/clears the animation-play pointer array when present. It does not reset blockCount. No animation update should traverse that count after teardown. [Model rendering](graphics-model-render.md) explains how prepared textures and animation entries are sampled, including construction display; this loader only prepares the references.

## Fonts and default palette lifetime

LoadGameFonts (`0x42a320`) loads COMIX and smlfont through the font adapter into two game fields. FreeGameFonts (`0x42a3b0`) frees and clears both. LoadDefaultPalette (`0x42a400`) loads PALETTE, copies 1024 bytes into the game's embedded palette storage, frees the temporary allocation and returns one. It relies on the palette loader's expected size; the copy itself is not bounded by an actual returned file length.

Fonts, general animation-cache resources and battle texture archives have separate cleanup functions. [Game-state lifecycle](../../../src/game/game_state.cpp) calls them at their respective application/battle boundaries. A common global game object does not make all these resources share one lifetime.

## Unit script and interface checksum aggregation

ComputeUnitScriptChecksum (`0x42a610`) returns immediately when a definition's scriptChecksum is nonzero. Otherwise it makes the unit-definition allocation writable, XORs contributions into that field, and restores read-only protection. Its inputs are:

1. The language-resolved `scripts/unitname.cob` bytes, when loadable.
2. Every discovered `guis/unitname*.gui` file's bytes, loaded individually through the same path builder.
3. The language-resolved `download/unitname.tdf` bytes when the file opens and has positive size.
4. The definition's weaponHashXor value.

Temporary loaded buffers and the download handle are released. GUI enumeration uses HPI's current visible-source view. Missing individual files are skipped rather than fatal here. The checksum primitive is documented in [math and checksum support](util-math.md). This aggregation does not independently validate exact read lengths beyond the underlying whole-file helpers.

XOR makes contributions order independent but permits cancellation, including repeated equal contributions. A resulting zero remains indistinguishable from the not-computed sentinel and can trigger recomputation later. Protection changes here permit writes to the definition table; they are not a mutex or a network authority gate. Network compatibility decisions using the checksum belong to their callers.

## Generated lifetime entry and completion limits

The final owned entry, `0x42a870`, is TdfFile's compiler-generated vector deleting destructor, emitted through an array construction in this physical file. It supplies the MSVC deleting-destructor contract: array mode destroys elements using the allocation's array count and releases the array allocation; scalar mode destroys the object and releases its storage when the deletion flag requests it. Parser destruction and unloading semantics are covered by [TDF](util-tdf.md). This entry adds no second asset-loader policy.

All 22 entries are covered above. The actual pixels, font metrics, sequence lengths, side panels and named-unit checksums depend on loaded assets and installed language/archive contents. No source listing, asset execution or generated archive is needed to explain the loading and ownership paths, and none was used for this writeup.
