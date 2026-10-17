# Indexed palettes, color transforms and filename helpers

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/palette`, all 32 entries from `0x4ba200` through `0x4bb150` in [palette.cpp](../../../src/graphics/palette.cpp). This is the color foundation for indexed rendering: a pixel stores one of 256 palette indices, and lookup tables map indices to other indices for blending, brightness, shading and tinting. It does not determine model geometry or when a construction wireframe is drawn.

## 1. Display-owned state and table dimensions

The module's DisplayContext view contains a 256-entry RGB0 palette, brightness storage, GDI/DirectDraw palette handles and five transform buffers. RGB0 entries contain red, green and blue bytes followed by a zero/reserved byte. Buffer availability is controlled by flags independently of the pointer values.

| Buffer | Availability bit in display flags | Bytes | Indexed transformation |
| --- | ---: | ---: | --- |
| Alpha | 5 | 65536 | Two input indices, 256 by 256 |
| Shade | 6 | 8192 | 32 rows by 256 source indices |
| Light | 7 | 8192 | 32 rows by 256 source indices |
| Gray | 8 | 256 | One output index per source index |
| Blue | 9 | 256 | One output index per source index |

AllocAlphaTable (`0x4ba5c0`), AllocShadeTable (`0x4ba610`), AllocLightTable (`0x4ba660`), AllocGrayTable (`0x4ba6b0`) and AllocBlueTable (`0x4ba700`) allocate the indicated byte counts and store the pointers. They return 1 without a local null check and do not themselves enable the availability bits. Their paired Free methods (`0x4ba5f0`, `0x4ba640`, `0x4ba690`, `0x4ba6e0`, `0x4ba730`) release the buffers without nulling pointers or clearing flags. Display initialization/destruction must keep those fields consistent.

SetAlphaTable (`0x4baad0`), SetShadeTable (`0x4bab00`), SetLightTable (`0x4bab30`), SetGrayTable (`0x4bab60`) and SetBlueTable (`0x4baba0`) copy exactly the table's full byte count into display-owned storage when the corresponding availability bit is enabled. Otherwise they silently do nothing. They neither retain the caller's pointer nor validate its size. Pointers typed as unsigned-int tables in these interfaces still represent byte lookup data.

## 2. Installing and reading the hardware/window palette

SetPaletteColors (`0x4ba200`) acquires the graphics MAIN lock and copies count four-byte input entries into the unscaled display palette beginning at start. It then constructs a temporary scaled palette: each requested channel is multiplied by the float stored at display offset +0x614, truncated and capped above at 255; the fourth byte becomes zero. There is no lower-bound brightness clamp and no range validation for start/count.

Two indexing details constrain partial updates. The stored-palette copy advances from source entry zero, while the brightness loop reads source entry i for i starting at start. A compact count-entry input and a full 256-entry input therefore do not have interchangeable partial-update behavior. Furthermore, only the requested temporary entries are initialized. The DIB branch nevertheless builds and installs all 256 entries, so a partial call can read uninitialized temporary colors outside that range. Full updates starting at zero avoid those particular partial-update limits.

With a DIB present, the function deletes any existing logical palette, allocates a 256-color LOGPALETTE, builds version 0x300 entries, creates a new GDI palette and writes an RGBQUAD color table to the display DC. The RGBQUAD path reverses red/blue byte order as Windows expects. Return values from CreatePalette and SetDIBColorTable are not used to reject the operation. With no DIB and display flag bit 2 set, it invokes DirectDraw palette SetEntries for the requested range; a nonzero HRESULT returns failure after unlocking. Other paths return success even when neither installation branch ran. The local flag name gdi is therefore not sufficient to interpret every branch as a GDI operation.

The inline lock uses InterlockedExchange with the constant tag MAIN. If free, it records that tag; if already owned by the same tag it treats acquisition as nested. Other owners wait on the graphics event. Unlock releases/signals only when the saved previous value was zero. This is tag-based nesting, not a per-thread recursive mutex implementation in this file.

GetPaletteColors (`0x4ba4c0`) reads all 256 entries through GDI GetPaletteEntries when flag bit 2 and a DIB are present, otherwise through DirectDraw GetEntries when bit 2 is set. Failure returns zero. It writes requested entries into destination indices first through first+count-1, rather than packing them starting at destination zero, and clears their reserved bytes. When bit 2 is clear, the local palette is not filled before the copy but the routine still returns 1. A successful return alone therefore does not establish initialized colors in that branch. Unlike SetPaletteColors, this routine does not take the MAIN lock locally.

SetBrightness (`0x4ba590`) stores its argument's 32-bit value at +0x614 and reapplies the full stored palette. The storage is declared int locally but read as a float by SetPaletteColors, so this is a bit-pattern interface; interpreting an arbitrary integer percentage as the intended brightness multiplier would be incorrect. The stored RGB0 entries remain the unscaled basis for reapplication.

## 3. Color matching and brightness ordering

NearestColor (`0x4ba880`) exhaustively checks all 256 entries using unweighted squared RGB distance. A strict improvement replaces the result, so a tie retains the earliest index. No gamma correction, perceptual weighting or reserved-index exclusion is applied here.

SortByBrightness (`0x4ba920`) computes each entry's red+green+blue sum, creates the original-index array 0..255 and sorts ascending sums while carrying those indices. It does not reorder the palette itself. The sum is an equal-channel brightness proxy, not a luminance calculation.

NearestColorInBand (`0x4ba9d0`) uses that sorted order to restrict distance checks to brightness sums within 40 of the requested RGB sum. It selects the smallest squared RGB distance among those candidates. If no candidate was examined, it uses the scan-end index rather than invoking exhaustive NearestColor. A break above the band can select that first out-of-band entry; reaching 256 assigns into an unsigned byte and wraps the index to zero before the order lookup. Consequently the accelerated lookup does not promise a globally nearest color or a specially handled empty band.

All generated transforms below use the band-limited matcher. Even mathematically exact source colors can map to another palette entry when duplicate RGB values or the sorted-order tie behavior permit it, except for alpha diagonal entries explicitly forced to their own index.

## 4. Alpha blending

BuildAlphaTable (`0x4ba750`) returns null when alpha availability is off. Otherwise it creates the brightness sort and fills the display's 65536-byte buffer. The row and column are the two source palette indices. A diagonal entry returns that same index directly. Other entries average the two red channels, two green channels and two blue channels using integer division by two, then match the resulting RGB color back into the palette.

This alpha table is a fixed approximately 50/50 blend, not a continuous alpha channel or a 256-level opacity parameter. Once generated, a consumer can combine indexed pixels using one lookup rather than three-channel arithmetic and a nearest-color search. Exact visible output depends on the palette or a supplied cached table. Construction/fog/particle drawing must select the appropriate consumer operation; merely having an alpha table does not make every unit translucent.

## 5. Shade, light, gray and blue generation

BuildLightTable (`0x4babd0`) fills 32 rows. Row r multiplies each RGB component by 1+r/30, truncates and caps above 255, then matches the resulting color. Row 0 uses unchanged channel values; row 30 doubles them; row 31 is slightly above double. The table contains resulting palette indices, not RGB triples or scalar brightness factors.

BuildShadeTable (`0x4badf0`) starts with scale zero and subtracts g_shadeStep after each row. The [global record](../../../data/globals.csv) supplies g_shadeStep=-0.06875. Thus row r scales by r*0.06875: row zero matches black, while the last row reaches 2.13125 times each component before clamping. The name shade does not imply that every row darkens the source. Each scaled component is converted through an unsigned short and capped at 255 before matching. Consumers decide which row corresponds to their lighting state.

BuildGrayTable (`0x4bad30`) replaces all three channels with the integer arithmetic mean of source RGB, then matches to the nearest allowed palette candidate. A colored palette need not contain an exact neutral gray for every average, so the resulting displayed pixels are palette approximations rather than newly allocated gray colors.

BuildBlueTable (`0x4baf30`) halves red and green, halves blue and normally adds 50, then matches the tint. Its source includes a blue-half-plus-60 overflow test, with a 255 result when that test passes; an eight-bit blue channel halved is at most 127, so that saturation branch is unreachable for ordinary palette input. Retaining the literal arithmetic matters more than inferring a conventional blue-filter formula.

Each generator checks its own availability flag, returns a pointer to the display-owned table on success and null otherwise. It overwrites the existing buffer in place, does not allocate it, and does not install a changed hardware palette. Calling a generator requires readable 256-entry palette data and valid allocated storage. Generating these lookup tables and changing display brightness are separate operations.

## 6. Filename helpers in the same address group

The final five methods are byte-string utilities used more widely than rendering. Their ownership in this module follows the address map, even though their names are generic.

| Entry | Behavior and boundary rule |
| --- | --- |
| ChangeExtension, `0x4baff0` | Copies input to output, scans backward for a period until a backslash, removes that suffix, then appends a period and supplied extension |
| HasExtension, `0x4bb0a0` | Returns true for any period anywhere in the string, including a directory component |
| StripExtension, `0x4bb0f0` | Scans the entire string backward and truncates at the last period, with no backslash stop |
| StripFileName, `0x4bb120` | Truncates just after the last backslash, preserving a trailing backslash; leaves a string with no backslash unchanged |
| StripPath, `0x4bb150` | Copies the substring after the last backslash to the beginning, including the terminator; leaves a bare filename unchanged |

These routines recognize backslashes specifically, not forward slashes, URL syntax or Unicode path conventions. They do not receive capacities. ChangeExtension uses unbounded copy/append and begins from length minus one, including the empty-input boundary. StripExtension and HasExtension can therefore disagree with a directory-aware understanding of an extension. The functions return caller-owned storage or a boolean; none opens a file. NopRetC (`0x4bab90`) is the remaining three-argument empty callback.

## 7. Integration and coverage

[Palette-table loading](graphics-palette-tables.md) supplies precomputed ALP/SHD/LHT buffers or calls these generators. [Game-state initialization](../../../src/game/game_state.cpp) invokes that setup and gray/blue generation. The [display implementation](../../../src/graphics/display.cpp) owns buffer allocation flags and palette handles; [drawing](../../../src/graphics/draw.cpp), blits and model rendering consume the results. A visible underwater tint, shaded hull or blended effect also depends on the consumer's chosen lookup row, input index and draw mode.

All 32 address-assigned entries were checked across the source tree: palette set/get/brightness, five allocation/free pairs, five table-copy setters, five generators, three matching/sort helpers, one empty callback and five filename helpers. There is no unit targeting, resource accounting, network packet or save record here. Table ownership and display state are local rendering infrastructure; exact indexed output depends on runtime palette/table assets. The reference is based on static source inspection without builds or gameplay execution.
