# Unit definition discovery, FBI interpretation and asset binding

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_types`, `0x42a8d0` through the boundary before `0x42e140`. Sources: [initial definition enumeration](../../../src/units/unit_types_42a8d0.cpp), [full loader and build-menu support](../../../src/units/unit_types.cpp), and shared [definition layout](../../../src/units/unit_def.h). AddToCategories at 0x488e70 is physically in the latter file but belongs to the completed [unit commands module](units-unit-commands.md).

Definitions are shared type data, distinct from live unit records. This module discovers compatible types, interprets FBI fields, binds models/scripts/weapons/movement classes, constructs buildable-ID lists and incorporates downloadable menu entries. Live construction state, movement, targeting and resource settlement consume these definitions later.

## 1. Initial enumeration and compatibility

`LoadUnitInfo`, `0x42a8d0`, first loads weapon TDF parsers for compatibility checks, releases the old definition array after making it writable, and enumerates units/FBI files. It allocates zeroed 0x249-byte records for all candidates plus sentinel type zero. The sentinel display/internal placeholder is None and its availability bit is set.

For each successfully opened FBI it computes a checksum of the raw bytes. A same-name OVR bank identified as TA Unit Override can replace that checksum through the Compatability account using its unsigned decimal value as key. This compatibility override changes the recorded checksum, not the FBI text being parsed in this function.

It requires UNITINFO, reads localized display name, internal unitname, side, AI weight/limit text, model name and costs. Missing model name falls back to unitname. It stores norestrict/wacky fields, XORs selected parsed weapon-record checksums for three weapons and two explosion references, and evaluates the FBI Version against the current major/minor game version.

Availability bit 0x800000 is retained for an older major version or a matching major with supported minor. The minor component is derived by flooring the first decimal digit after the integer major. Archive/CD-origin checks and a normalized copyright comparison can also clear availability. Type limits begin at minus one.

After scanning, it deletes the temporary weapon parsers and removes incompatible entries by moving the final retained record into each hole. It adjusts IDs/count as it goes. Version-related removals can display an incompatibility notice, while the separate bad-origin/copyright flag suppresses that particular notice. It protects the definition array read-only and returns 1.

A missing UNITINFO returns zero after freeing the text buffer but before closing the current FBI and cleaning all temporary/protection state. A file that cannot be opened leaves a zeroed, unavailable candidate for later filtering. The body relies on fixed field/buffer sizes and valid copyright normalization inputs rather than independently validating arbitrary corrupted assets.

`RefreshUnitInfo`, `0x42bd10`, repeats enumeration when the count is zero or the enumeration-dirty flag is set, then clears that flag without propagating LoadUnitInfo's success. `FreeUnitInfo`, `0x42bcc0`, frees this summary table and resets count/pointer. It is not the full per-model/script teardown used after match asset binding.

## 2. Full load sequence and stable IDs

`LoadUnitTypes`, `0x42d2e0`, requires gamedata/moveinfo.TDF and processes CLASS0 through CLASS31 into the movement-class table. Present class records receive copied names and movement parameters; absent records are skipped.

It allocates the model-picture cache object and sizes its arena from screen area, approximately 1.3 times two bytes per screen pixel, multiplied by a display-context memory factor. That factor stays 1 up to the threshold, otherwise grows with the reported megabyte-like value divided by 16 and caps at 5. The allocation rounds upward to a 4096-byte boundary. This presentation cache budget does not change simulation unit capacity.

It compacts away unavailable definitions again, sorts nonzero definitions case-insensitively by internal unitname and assigns final sequential IDs. CompareUnitTypeNames, `0x42db60`, supplies the name comparison. UnitDef assignment, `0x42b370`, copies arrays, scalars, structures, pointers and individual flag fields so compaction/sorting preserve the full record. It is a shallow transfer of resource pointers, not a deep duplication of models/scripts/yard maps.

The definition-index bit width is calculated by shifting the definition count until zero. This width drives network state encoding. Final IDs follow the sorted loaded set; retaining an enumeration-stage ID across this process would be incorrect.

## 3. FBI identities, categories and numerical fields

`LoadUnitFbi`, `0x42bf40`, requires a loadable FBI and UNITINFO. Failure returns without applying a complete default replacement to the supplied record. It updates internal/display names and description, resolves defaultmissiontype, binds three bad-target-category masks and noChaseCategory, and selects objectname or unitname fallback.

The principal numerical contracts are:

| Family | Fields and interpretation |
| --- | --- |
| Cost/work | Integer energy/metal costs stored as floats; buildtime integer; workertime/healtime narrowed to shorts |
| Movement | maxvelocity, brakerate, acceleration, bankscale, pitchscale, damagemodifier and moverate1/2 read as fixed point |
| Movement defaults | bankscale and damagemodifier default to 65536; moverate1/2 default to twice loaded maxvelocity; other listed movement values default to zero |
| Economy | energymake/use, metalmake, extractsmetal, wind/tidal ratings and storage read as floats; makesmetal narrowed to byte |
| Health/sensors | maxdamage integer; sight, radar, sonar, both jammer distances and minimum cloak distance narrowed to shorts |
| Geometry/transport | waterline, transportsize and transportcapacity narrowed to bytes; build/turn angles, build range, cruise altitude and tactical distances narrowed to shorts |
| Cloaking | cloakcost integer-to-float; moving cost defaults to stationary cost; positive stationary cost derives can-cloak bit |

The tactical distances include maneuver leash, attack-run length and kamikaze distance; sortbias is also retained. These loaders do not independently apply a comprehensive physical range clamp to every field. Runtime consumers determine work cadence, range geometry, per-second conversion and permissions.

Selfdestructcountdown defaults to 5 and is stored in a three-bit field. The representable value is therefore narrower than an unrestricted integer number of seconds. Sound category resolves by case-insensitive category name, falls back to atoi for an unmatched supplied name, and defaults to zero when absent.

Corpse resolves a feature definition or remains 0xffff. Category population adds the type to named masks and ALL through the unit-command service. A cloak-capable definition with zero minimum cloak distance receives default 80 after parsing.

## 4. Exact flag sources

The flags' meaning comes from the loader keys, which correct several misleading private-view member names elsewhere:

| flags1 bits | Loaded source or derived role |
| --- | --- |
| 0..1, 2..3 | standingmoveorder and standingfireorder, each default 2 |
| 4, 5, 6, 7 | init_cloaked, downloadable, builder, zbuffer |
| 8, 9, 10, 11 | stealth, isairbase, istargetingupgrade, canfly |
| 12, 13, 14, 15 | canhover, teleporter, hidedamage, shootme |
| 16 | Derived from having at least one nondefault weapon |
| 17, 18, 19, 20 | armoredstate, activatewhenbuilt, floater, upright |
| 21 | amphibious |
| 23 | Availability/compatibility from enumeration |
| 24, 25, 26, 27 | isfeature, noshadow, immunetoparalyzer, hoverattack |
| 28, 29, 30 | kamikaze, antiweapons, digger |
| 31 | GUI-page-zero presence, assigned during asset binding |

| flags2 bits | Loaded source or derived role |
| --- | --- |
| 0, 1, 2, 3 | mobilestandorders, firestandorders, onoffable, canstop |
| 4, 5, 6, 7 | canattack, canguard, canpatrol, canmove |
| 8 | canload |
| 9 | Copied from canreclamate bit during loading, used by repair eligibility |
| 10, 11, 12 | canreclamate, canresurrect, cancapture |
| 13 | Derived from positive cloakcost |
| 14, 15 | candgun, norestrict |
| 16 | Initial enumeration's wacky field |
| 17, 18, 19 | showplayername, commander, cantbetransported |
| 20..22 | Three-bit selfdestructcountdown |

canhover and floater are different bits. A private member called floats may actually reference canhover, while one called over_water references upright. Likewise init_cloaked is not the same bit as canattack. Follow the loaded bit positions when interpreting height, damage exemption or initial cloak state. [Position/update rules](units-unit-position.md) and [interaction predicates](units-unit-targets.md) describe concrete downstream tests.

## 5. Movement-class inheritance and footprint geometry

The loader resolves movementclass by name. If unavailable, it reads equivalent movement information directly from the FBI into a temporary class. Either source supplies footprint X/Z, minimum/maximum water depth and land/water slope limits. maxslopevelocity is derived from maxvelocity divided by maxslope plus one through fixed-point arithmetic.

This means a named movement class can override the expected footprint/depth data source. Reading only a unit's visible model size or a familiar ship name is insufficient to infer passability.

Horizontal extents become plus/minus half the footprint scaled by the 16-unit cell size. Extentsize is their difference, and radius is the sum of horizontal extents divided by three, rather than an exact bounding-circle computation. Model loading later supplies vertical maximum/size. Placement, collision and rendering can use different geometric approximations derived from these values.

## 6. Weapon and explosion binding

weapon1/2/3, explodeas and selfdestructas each resolve through the weapon-name table, falling back to the default weapon record when missing. At least one nondefault weapon sets flags1 bit 16; all three default clears it. This is distinct from the flags2 canattack command permission.

Bad-target category masks, no-chase masks, command capability, weapon type, aim script and sensor eligibility remain separate gates. Binding a weapon pointer alone does not make every unit category shootable. The [naval reference](../../naval-systems.md) follows those gates into surface/submerged/air interactions.

## 7. Yard-map decoding

For bmcode zero, the loader allocates one yard byte per footprint cell and scans recognized YardMap characters. Other characters are skipped without consuming a footprint cell:

| Character | Yard byte |
| --- | --- |
| . | 0x00 |
| f | 0x6f |
| o | 0x2f |
| c | 0x2d |
| O | 0x2b |
| w | 0x37 |
| C | 0x35 |
| y | 0x29 |
| Y | 0x31 |
| G | 0x8f |

Case matters. These bytes combine support-height, occupancy-open/closed, feature, water and other placement tests rather than assigning one universal walkable boolean per character. [Terrain placement and yard transitions](map-terrain.md) explain how the individual mask bits are consumed. For bmcode nonzero the yard-map pointer is null.

When a recognized final character is followed by the string terminator, the cursor does not advance, allowing that final character to fill further cells. An empty or malformed string without a usable recognized character has no explicit safe end-of-input branch in this scanner. Asset correctness is part of the contract; this is not a general validated text grid parser.

## 8. Models, scripts and GUI page discovery

After final IDs/sorting, the full loader reads each available FBI, loads the named objects3d/3DO model, treats a missing model as fatal, mirrors geometry and binds model textures. It stores the model pointer and obtains model height for vertical extents.

It probes unitname0.GUI, then numbered GUI pages starting at 1 until the first missing file. Page discovery is consecutive; a later file after a gap does not extend the count. Presence of page zero sets the GUI flag. With numbered pages it records the suffix/count convention used by the menu system; with only zero it records one page, and with none zero pages.

It loads scripts/unitname.COB into the shared script-data pointer. Actual per-instance COB threads/object state are created later by [unit initialization](units-units.md). Model/script data here are shared definitions, not each unit's current animation pose.

The model-pointer table and definition table are protected read-only after their mutation phases. Protection calls express lifecycle boundaries in this implementation; the documentation work does not alter them or rebuild assets.

## 9. Base buildable lists and downloaded menus

The full loader requires gamedata/sidedata.TDF, then for builder definitions reads consecutive canbuild1, canbuild2 and later keys under CANBUILD/unitname. Unknown unit names are skipped. The temporary list and each builder's allocated ID table reserve 0x3c bytes, or 30 short IDs. The loop does not independently stop at 30 valid entries before writing. Consumers use the recorded count; copying the entire temporary buffer also copies unused trailing bytes.

`LoadDownloadMenus`, `0x42dcf0`, enumerates download/TDF files and allocates one 0xbd-byte record per file. Its declared record contains five entries with destination builder ID, page, button and unit name. It walks top-level TDF records, resolves UNITMENU against definition unitname, and stores MENU/BUTTON/UNITNAME. The parser does not locally cap records at the five-entry layout; malformed or oversized files can exceed that storage assumption.

It raises affected definitions' page counts to accommodate menu entries. Matching downloaded unit names also cause downloadable to be set when absent. `CheckDownloadableFlags`, `0x42bd40`, separately performs that consistency adjustment, temporarily making definitions writable.

`AddDownloadBuildOptions`, `0x42be30`, resolves each entry's unit name and appends its ID to the referenced builder list when the builder has a list. Its count guard is less-than-or-equal to 30, allowing the write at index 30 of a 30-element allocation. That off-by-one is a concrete limit in the shown body, not a supported promise of 31 safe entries.

`FreeDownloadMenus`, `0x42e120`, frees the downloaded-table allocation without independently resetting pointer/count in the body. It is not equivalent to freeing every builder's own ID list.

## 10. Reload and full teardown

`ReloadUnitType`, `0x42d1f0`, ignores zero/unavailable IDs, makes definitions writable, and, when the FBI has nonzero reported length, reparses it, frees the old shared COB data and loads a replacement script. It restores read-only protection on either path. It does not reload the model, sort/reassign IDs or rebuild every menu in this method. The FBI loader itself resets/allocates yard data without a complete old-yard release transaction.

`FreeUnitTypes`, `0x42db90`, makes tables writable, visits real definitions starting at 1 and releases models, yard maps, shared COB data and builder ID arrays. It destroys the picture-cache object and frees model-pointer/definition arrays, clearing their pointers. This assumes live units no longer depend on those shared assets. It differs from summary-only FreeUnitInfo and downloaded-table cleanup.

The full loader sets load progress to 100 and marks enumeration dirty at completion. This flag feeds later refresh policy; it is not a count of live units or a simulation-completion percentage.

## 11. Evidence and completeness

All annotated entry points in the module address range are covered: initial enumeration, assignment, summary cleanup/refresh, downloadable checks/appends, FBI parsing, reload/full load, comparison, complete teardown and downloaded menu load/free. The module does not define a save-bank format or transmit packets itself; it supplies IDs, compatibility metadata and shared assets used by those systems.

Exact named-unit statistics and animations require the actual FBI/COB/3DO/weapon/movement/GUI assets. The source establishes defaults, conversions, inheritance and fallbacks. For a shipyard issue, inspect bmcode, yard bytes, footprint source, builder IDs and GUI pages separately. For a naval movement issue, inspect canhover, floater, upright, amphibious, movement-class depth/slope and draft instead of treating them as one ship flag. For incompatibility, inspect enumeration/version/origin/checksum paths before tracing live construction.
