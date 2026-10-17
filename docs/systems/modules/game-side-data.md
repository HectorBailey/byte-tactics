# Side definitions, interface rectangles, fonts and logos

## Scope and data model

The `game/side_data` group owns five entries from `0x4318c0` to `0x432520`, all in [side_data.cpp](../../../src/game/side_data.cpp). It loads shared team-logo graphics, reads faction identifiers and interface geometry, and owns per-side font cleanup. It does not define unit movement classes, build permissions, weapon categories or faction AI behavior.

The game context contains a side count and five packed side records, each `0x232` bytes. Each record has a 30-byte name, four-byte name prefix, 32-byte commander unit name, thirty rectangles, two integer palette colors, a side-number integer and a font pointer. Rectangles store four integer coordinates. Their actual screen interpretation belongs to interface consumers, including the [info panel](ingame-info-panel.md) and [control panel](ingame-control-panel.md).

Input is `gamedata/sidedata.TDF`, resolved through the [language-sensitive path builder](game-data-files.md) and [HPI source priority](util-hpi.md), then parsed through [TDF](util-tdf.md). The same sidedata file is independently read by general resource loading for baseheight and intgaf panel assets. Those reads share the filename but populate different state.

## Side discovery and scalar fields

LoadSideData (`0x431a60`) constructs a local parser, loads sidedata without checking the return, points at the first side slot and starts index zero. Each iteration stores the side number into the current slot before resetting the parser and selecting SIDE followed by the index. The first absent record terminates the loop. The final count is the number of consecutively present records, not the highest named record plus one.

For each selected side, the loader requests name, nameprefix and commander with capacities matching their destination fields. It copies each string only if its TDF getter reports a found field. Missing fields therefore leave prior/initial destination storage rather than explicitly assigning an empty default. The getter's ordinary found-string termination rules keep these specific bounded strings within their fields.

A present font field resolves `fonts/value.FNT` and loads whole-file bytes. Null loading invokes FatalError with the path. An absent field leaves the existing font pointer unchanged. Energycolor and metalcolor are integer fields with zero defaults. The values are stored without clamping, color conversion or palette validation here.

The implementation has no side-count limit in the discovery loop, despite the five-element storage. Moreover, because the sideNumber store precedes the missing-record test, an input containing five valid sides causes an additional sideNumber write through the sixth slot before SIDE5 can terminate discovery. This is a concrete storage-boundary concern, not evidence that sideNumber itself has the wrong meaning. The source's speculative comment questioning the field's purpose is not needed to explain the observed index store.

Repeated loading is not an in-place reload protocol: previous fonts are not freed before assigning newly loaded pointers, absent values can retain old state, and the loop relies on ordinary initialization and cleanup ordering.

## Complete rectangle schema

Each rectangle is a child record of the selected SIDE record with x1, y1, x2 and y2 integer fields. Missing coordinate fields default individually to zero. A missing rectangle record is fatal, with a diagnostic naming the rectangle and side. No width/height calculation, endpoint sorting, clipping or resolution scaling occurs in this loader.

| Child record | Destination role |
| --- | --- |
| LOGO | Primary logo rectangle |
| ENERGYBAR | Energy bar |
| ENERGYNUM | Energy numeric display |
| METALBAR | Metal bar |
| METALNUM | Metal numeric display |
| TOTALUNITS | Total unit count |
| TOTALTIME | Elapsed time |
| ENERGY0 | Energy zero marker |
| METAL0 | Metal zero marker |
| ENERGYMAX | Maximum energy label/value |
| METALMAX | Maximum metal label/value |
| ENERGYPRODUCED | Energy production display |
| ENERGYCONSUMED | Energy consumption display |
| METALPRODUCED | Metal production display |
| METALCONSUMED | Metal consumption display |
| LOGO2 | Secondary logo rectangle |
| UNITNAME | Primary unit name |
| DAMAGEBAR | Primary unit health/damage bar |
| UNITMETALMAKE | Unit metal production |
| UNITMETALUSE | Unit metal use |
| UNITENERGYMAKE | Unit energy production |
| UNITENERGYUSE | Unit energy use |
| MISSIONTEXT | Mission text |
| UNITNAME2 | Secondary unit name |
| DAMAGEBAR2 | Secondary health/damage bar |
| NAME | Name block |
| DESCRIPTION | Unit description |
| RELOAD1 | First weapon reload display |
| RELOAD2 | Second weapon reload display |
| RELOAD3 | Third weapon reload display |

The first eleven are loaded through an inlined helper; the following sixteen and three reload records use ReadSideRect (`0x431950`). Both save the current parser record, select the child, read four coordinates and restore the prior record. Restoring the cursor is necessary so each next lookup remains relative to the SIDE record rather than the previous rectangle.

The standalone helper explicitly restores and returns after reporting a missing record. The inlined helper reaches restoration after either branch if FatalError returns. Neither invents a fallback rectangle for a missing child. The distinction between a missing record and missing coordinates matters: the former is fatal, while the latter yields zero coordinate values.

After all side records, the parser unloads and the count is stored. Rectangle presence does not enable the corresponding gameplay ability. For example, a RELOAD3 location does not give every unit three weapons, and a resource-output location does not set that unit's production. The panel reads current gameplay state into these configured display regions.

## Logos, borrowing and fonts

LoadLogos (`0x4318c0`) resolves `textures/logos.GAF`, calls LoadGaf and stores the resulting allocation. It then immediately obtains the `32xlogos` entry and stores a borrowed pointer. Unlike the fatal wrappers in data_files, this body has no explicit null check between loading and entry lookup. The underlying GAF implementation determines how a missing allocation is handled.

The entire logos allocation is also used by [model texture binding](game-data-files.md). A named model texture can fall back to a matching logos entry; ten-frame entries receive the special logo flag and are excluded from ordinary texture-animation pointer registration. That behavior is implemented in data_files, not in LoadLogos.

FreeLogos (`0x431920`) frees and clears the top-level logos pointer. It does not clear the separately stored 32xlogos entry pointer, which consequently becomes stale. Callers must respect the shared allocation's lifetime. It is not a separately owned frame allocation to free independently.

FreeSideFonts (`0x431a20`) visits all five side slots, regardless of current sideCount. Each non-null font is freed and cleared. It does not clear identifiers, rectangles, colors, side numbers or the side count. The fonts are independent allocations even if two sides request the same filename; this loader does not use a font cache.

[Application lifecycle](../../../src/game/game_state.cpp) loads side data followed by logos during game initialization and frees logos plus all side fonts during application resource shutdown. Battle texture/model loading uses the already prepared logos. Loading a map does not itself redefine these side records.

## Completion and asset-dependent results

All five owned entries are covered: logo load/free, rectangle read, all-font cleanup and complete side loading. Actual faction names, commander definitions, panel coordinates, font bytes, logo frames and colors come from installed assets. Neither the packed field sizes nor the SIDE-number convention establishes the number of factions present in a particular installation. Source traversal establishes the contiguous discovery rule, five-slot storage and lifetime requirements without executing the game or copying source into this reference.
