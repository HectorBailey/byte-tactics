# GUI record serialization, field readers and shared GAF assets

## Scope and file boundary

The `gui/gui_file` group owns 33 entries in `0x4acd00..0x4aefa0`, all in [gui_file.cpp](../../../src/gui/gui_file.cpp). It reads/writes gadget fields as TDF text, serializes complete GUI record arrays and loads/frees common GUI/font GAF allocations. Runtime layout stacking, focus, event callbacks, list scrolling and widget drawing belong to the generic GUI module and its consumers.

A gadget record is `0x15b` bytes. Its leading common fields identify type, association, name, geometry, attributes, colors, texture/font indices, active state and common attributes. A body beginning at `+0xb6` is interpreted according to type. Record zero additionally carries panel metadata, version and default gadget names. The shared runtime [Gadget declaration](../../../src/gui/gadget.h) uses unions for those overlapping roles; local structs here expose the fields used by serialization.

[TDF](util-tdf.md) owns text syntax, numeric conversion and record selection. [HPI](util-hpi.md) chooses the loaded source and supplies loose-file writing. A GUI field describes presentation/input state; it does not itself give a unit a build capability or create an order.

## Text writer primitives and limits

WriteSectionHeader (`0x4acd00`) emits a bracketed name line with depth minus one tabs, then an opening-brace line with depth tabs. WriteSectionEnd (`0x4acda0`) emits depth tabs and a closing-brace line. WriteKeyValue (`0x4acde0`) emits a key/value assignment terminated by a semicolon/newline after depth tabs. The separately owned WriteTabs helper and inlined equivalents implement indentation.

These methods write raw text bytes without escaping delimiters, embedded newlines or arbitrary user text. They use 100-byte local formatted-line buffers even when source strings can be longer. HAPI write counts are discarded. Negative indentation yields no iterations rather than a validation error. They are legacy serializer building blocks, not a length-safe generic TDF writer.

## Common field schema

WriteCommonFields (`0x4ace50`) and ReadCommonSection (`0x4ad350`) cover the following keys inside COMMON:

| Key(s) | Read conversion/default | Writer behavior |
| --- | --- | --- |
| id, assoc | Narrow integer to unsigned byte, default zero | Decimal byte values |
| name | String capacity 16, empty default | Copies at most 16 bytes into a terminated temporary before writing |
| xpos, ypos, width, height | Narrow integer to signed short, default zero | Decimal signed geometry |
| attribs | Integer, default zero | Decimal integer |
| colorf, colorb | Read as integer, narrow to unsigned short then store in integer field | Decimal stored value |
| texturenumber, fontnumber, active, commonattribs | Narrow integer to byte, default zero | Interprets stored bytes as signed when formatting |
| help | String capacity 128 followed by the clearing/translation behavior below | Writes current help text |
| gaffile | Replace low flag bit from integer, preserve other bits | Writes only low bit |

ReadCommonSection acts only when selecting COMMON succeeds. It does not clear the entire gadget on a missing section. Numeric narrowing is not clamping: out-of-range integers can wrap in byte/short fields. Nonzero even gaffile input contributes low bit zero; it is not converted by a generic truth test.

The help path reads the field and then zeroes all 129 bytes of its help storage before translating/copying that now-empty string. Thus the loaded help text is discarded in this body. This is an observed read/write asymmetry, not a translation of the original help value. The final bounded translation copy also does not force a terminator independently.

Writing common fields can mutate the source geometry. For type zero, nonzero global `DAT_0051fba8` and nonnegative ypos, the writer subtracts 480 directly from the object's ypos before formatting it. It is not a temporary exported coordinate. Repeated saves can subtract again while the stored position remains nonnegative. No inverse adjustment is performed by ReadCommonSection itself.

## Panel metadata and defaults

WritePanelFields (`0x4ad4f0`) emits totalgadgets, a nested VERSION record with major/minor/revision, and panel, crdefault, escdefault and defaultfocus names. The version bytes are formatted with signed-char interpretation. Panel gets a terminated sixteen-byte temporary; the other default-name writers trust their in-memory termination.

ReadPanelFields (`0x4ad890`) narrows totalgadgets to short, reads each name with capacity sixteen and empty default, and loads version bytes with zero defaults only if VERSION exists. If it selects VERSION it leaves the parser there; the whole-file reader resets to the next top-level record before continuing. Missing VERSION does not explicitly initialize the three destination bytes.

crdefault and escdefault identify GUI controls used by generic confirm/cancel behavior, and defaultfocus identifies a focus target. This serializer does not resolve those names to valid gadget indices. It also does not compare version bytes against a supported-version policy or reject a layout based on them.

## Type-specific field pairs

| Record type / role | Writer/read entry addresses | Serialized body and read setup |
| --- | --- | --- |
| Status-only helper | `0x4ad980` / `0x4ada10` | status, narrowed to short on read |
| 1, button | `0x4ada40` / `0x4adc70` | status, text, quickkey, grayedout low bit, stages |
| 2, listbox | `0x4add60` / `0x4addf0` | itemheight; reader also clears callback and user fields |
| 3, text input | `0x4ade20` / `0x4adf10` | maxchars and translated text |
| 4, slider | `0x4adf90` / `0x4ae170` | range, thick, knobpos, knobsize; reader additionally loads translated text and clears slider callback |
| 5, text/link | `0x4ae230` / `0x4ae300` | translated text and link; reader clears hotkey and initializes text/link storage |
| 6, hotspot flag | `0x4ae380` / `0x4ae410` | hotornot low bit |
| 7, font-filename field | `0x4ae440` / `0x4ae4b0` | filename, read with capacity 32 |
| 8, surface-filename field | `0x4ae4e0` / `0x4ae550` | filename, read with capacity 32 |
| 10, provisional empty/line field | `0x4ae580` / `0x4ae610` | nuttin integer |

The standalone status pair exists even though the whole-file type switch does not dispatch a separate status-only type. Its existence does not fill every unhandled gadget ID with an implicit reader.

Button reading sets status, clears/reads its 128-byte text and copies the translated text with a 128-byte bound. quickkey is read as text: an alphabetic first byte is stored directly; otherwise atoi supplies the byte value. The writer instead formats quickkey as a signed decimal byte. It replaces only grayedout's low bit and stores stages as a byte; it does not validate that status/frame counts fit stages or artwork.

Text-input maxchars is narrowed to a short, then clamped only above 128. Negative values or integers that wrap before comparison are not given a safe minimum. The reader obtains text with capacity 128 and then performs an unbounded copy of its translation, so a longer translation can exceed the nominal field. Loading text does not truncate it to maxchars after translation.

Slider reading narrows range/knob positions/sizes to short. thick is also narrowed to short before being stored in its wider field. There is no enforcement that knob size is positive, knob position lies within range or range matches gadget geometry. The writer emits the four numeric fields but omits the reader's text field, so a save/reload need not preserve slider text.

Text/link reading clears its hotkey and text storage, translates using a 127-byte copy and reads link with capacity sixteen. The writer's local link view is larger than the runtime tail, but it writes a C string beginning at that same location. No link-target resolution occurs here. Font/surface filename readers similarly only retain names; later rendering/resource setup determines what those gadget types actually load.

Hotornot uses a one-bit assignment, retaining the integer's low bit. nuttin is an unvalidated integer with no stronger semantic meaning established by the provisional field name. Missing numeric body fields normally default zero, but fields outside a selected type's reader can retain their prior bytes.

## Whole-file writing, backup and partial failures

WriteGuiFile (`0x4ae630`) changes the supplied name's extension to GUI. If HAPI_FileLengthByName reports a nonzero existing length, it forms a BGU backup name, removes that backup and renames the GUI file to it. It ignores remove/rename results, then creates the new loose GUI file.

Length testing conflates empty/missing content and can also discover an archive-backed GUI even though remove/rename operate on loose filesystem paths. A new loose layout can therefore override archive content without renaming that archived member. There is no atomic temporary-file replacement or restore-on-write-failure transaction.

It iterates first-record totalgadgets plus one records, emitting GADGET0, GADGET1 and so on, each containing COMMON plus the type-specific body. Type zero uses panel fields; supported types follow the table above except the unused standalone status pair. Other types receive only common fields. Several calls are inlined duplicates of the standalone field writers, with the same schema.

The source record array, total count and output handle are trusted. File creation failure is not checked before writes; individual write/close results are discarded. A partially written file can remain while the prior file has already been renamed. Writing mutates eligible panel ypos values as described above. This is an editor/export path with observable filesystem/state effects, not a pure serialization-to-memory operation.

## Whole-file reading and record order

ReadGuiFile (`0x4aeac0`) changes the requested extension to GUI and loads a TdfFile. Failure returns zero. A successful load sets return one, then enumerates all top-level records by index until selection fails. It does not require their names to be GADGET plus a consecutive number.

For each record it saves the parser's current-record handle, applies ReadCommonSection, restores the top-level handle and selects the type-specific reader. Several readers are inlined with their same defaults, narrowing and translation behavior. Restoring before the type read prevents COMMON from becoming the source for body fields. The parser is reset at the beginning of every next-record iteration.

There is no output-array size argument, per-record allocation, type range validation or complete destination clearing. An absent COMMON can leave the prior type/fields controlling body interpretation. After enumeration it overwrites record zero's total count with actual top-level record count minus one, narrowed to short, rather than trusting serialized totalgadgets. A successfully loaded file with no records can therefore store -1 and still return one. Parser structural errors retain TDF's fatal behavior; this success value is not comprehensive GUI validation.

The function unloads its parser, leaving copied strings/scalars in caller records. File-origin flags and TDF object pointers are not stored as the widget's persistent file representation by this reader. Generic GUI loading must allocate enough records and prepare runtime resource fields elsewhere.

## Common GUI and glyph-GAF loading

LoadGafIntoSlot (`0x4aedd0`) combines the object's configured animation directory with a name, forces GAF extension and tests file length. A nonzero length loads a GAF into the indexed slot. It takes the first sequence pointer at loaded-file offset `0xc`, obtains frame index `0x49` and uses that frame's unsigned height as a vertical adjustment. It subtracts that height from every valid frame's signed Y hotspot in that first sequence and sets activeFontGaf to the slot allocation.

This is glyph baseline preparation, not a translation of every GAF sequence by its current hotspot. If frame `0x49` is absent, the adjustment comes from an uninitialized local. If loading returns null after a nonzero length test, the next metadata access is not independently guarded. The slot index is not checked, and replacing an existing pointer does not first free its allocation.

For zero file length it clears the indexed slot and returns, but does not clear activeFontGaf. An older active pointer can therefore remain even though that requested slot was replaced with null. Directory copying and name concatenation use fixed 256-byte buffers without a complete termination/capacity guarantee.

FreeGafSlot (`0x4aeda0`) checks the chosen one of three buffer slots, frees/clears it when present and sets activeFontGaf zero regardless of which slot had been active. If the chosen slot was already null, it does not clear activeFontGaf. It owns the GAF allocation rather than each borrowed glyph frame separately.

LoadGafFile (`0x4aeee0`) forms the same directory/name/extension path for the common GUI GAF and loads only when file length is nonzero. A missing file leaves the old common pointer unchanged; replacement does not free the old pointer. FreeCommonGuiGaf (`0x4aef80`) frees and clears a present common pointer. [Application initialization](game-game-state.md) normally loads commongui plus hattfont12/hattfont11 slots and frees them at shutdown.

## Completion and integration limits

All 33 owned entries are accounted for: three syntax writers, common/panel pairs, ten body-field pairs, whole-file read/write and four shared-GAF ownership helpers. GUI runtime callbacks, text/slider/list interaction, resource binding for individual gadgets and actual visual layout remain separate systems. Exact assets/translations can expose the capacity and missing-glyph paths above. No GUI file was exported, backup renamed, asset rendered or game executed for this source-based reference.
