# Mission command text, ownership transfer and unit categories

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_commands`, `0x487bf0` through the boundary before `0x489280`, implemented in [unit_commands.cpp](../../../src/units/unit_commands.cpp) and the category-population method in [unit_types.cpp](../../../src/units/unit_types.cpp). It interprets initial mission text, instantiates configured mission units, transfers units between owners, looks up definitions and manages category masks. Supporting sort/container routines also lie in this range.

## 1. Initial mission text processing

`RunInitialMission`, `0x487bf0`, reads comma-separated clauses, skipping leading whitespace and dispatching case-insensitively on the first letter. Positions parse as floating-point world X/Z converted to 16.16; Y is set to zero. GetOrderType chooses a unit-specific mission kind where used, and AddOrder receives the generated request. The parser does not directly execute movement, construction or weapon simulation.

| Clause | Interpretation established by the body |
| --- | --- |
| O followed by two integers | Sets two two-bit standing move/fire fields; missing conversions retain initialized current values |
| M followed by X Z | Resolves command mode 2 at a position and adds the resulting order |
| U followed by X Z | Resolves command mode 5 at a position |
| G followed by name | Finds a mission unit and resolves mode 7 against it |
| P followed by X Z and optional seconds | Resolves mode 9 and passes optional duration times 30 as an order parameter |
| A followed by X Z | Resolves position-based attack mode 3 |
| A followed by type name | Adds ATTACKUTYPE with the resolved definition ID |
| BW followed by optional count | Adds BUILDWEAPON; count defaults to 1 |
| B followed by type, count, X Z | Adds MOBILEBUILD when motion exists, otherwise BUILDINGBUILD without a position |
| WA followed by optional name | Adds WAITFORATTACK for that mission unit, falling back to the current unit |
| W followed by optional seconds and integer | Adds WAIT with duration times 30 and the second parameter |
| D | Adds SELFDESTRUCTFG |
| S | Adds MAKESELECTABLE |
| I followed by name | Immediately attaches to the named mission unit rather than adding an ordinary command order |

Names in most clauses accept letters, numbers, underscores and periods; WA's name scanner omits underscores. Only A explicitly uses two successful numeric conversions to choose between coordinate and type-name forms. Other branches do not consistently validate conversion counts before using parsed coordinates. Text and scanned names use fixed buffers without a complete local length check. This is trusted asset command text, not a robust general-purpose command language.

The parser tracks whether an order-producing clause was processed and whether one of selected clause forms handled selectability. When processed, it clears runtime bit 0x20 and, unless that second marker was set, adds MAKESELECTABLE at the end. Position A, P, D and S set that marker. O and I do not set processed, and the BW branch adds its order without setting processed. Therefore BW alone does not trigger the same automatic tail operation as most other order clauses.

All AddOrder calls here pass removal/queue control value 1, with command-specific arguments. The order-list implementation determines how that value affects existing commands. A string containing several clauses should be explained through those queue semantics, not assumed to execute all clauses synchronously in the parser.

## 2. Initial mission-unit creation

`CreateMissionUnits`, `0x488310`, allocates a null-initialized pointer vector parallel to the mission's initial-unit records. It first finds each definition by name. Missing definitions leave that entry null and continue. For found definitions it converts the configured one-based player number to zero-based, requires a valid active controller role 1/2/3 and valid slot, and reports fatal error for invalid ownership.

It snaps the configured position through the definition's footprint helper, modifying that mission record's position, then calls completed CreateUnit in mode 1. Successful units receive the record's immunity-related bit, percentage-derived health and heading, and enter the parallel vector. The health calculation uses definition maximum times configured percentage divided by 100, then narrows to the health field; this method does not independently normalize arbitrary percentages.

A second pass interprets initialMission only for records that have both text and a created unit. Creating the whole initial set first enables name references to later-listed units. FindMissionUnit uses this parallel table, as described in [unit persistence and mission lookup](units-unit-save.md).

This body does not implement creationCountdown scheduling or every other mission-record flag. It creates the records through the shown immediate path. If the mission unit count is zero, it deactivates mission conditions; missing definitions for all records do not cause that same count-based test to pass when the configured count remains positive.

## 3. Ownership transfer preconditions

`GiveUnitToPlayer`, `0x488570`, ignores same-owner units, nonlive units and units marked 0x4000. It notifies victory conditions of capture before later receiver activity/capacity checks. A failed later transfer can therefore still have issued that notification; it is not a final-success callback.

The destination must be active. Ownership is not changed by simply assigning the old unit's player pointer. Ordinary unit storage is partitioned into player-owned ID ranges, so locally controlled receipt creates a new unit in the recipient's range. See [core unit lifecycle](units-units.md).

For an active current owner of type 1/2 and a destination of type 3, the function takes the network handoff path. It clears one unit flag, sets a 150-tick post-transfer holdoff and broadcasts a 0x18-byte type-20 record with old unit ID, destination network ID, construction value, health, angles and three stockpile bytes. It then applies damage amount 30000 with kind 4 to the old local unit and returns.

The packet's construction field is assigned by casting buildLeft to integer, and the receipt path converts that integer back to float. It does not copy the fraction's raw bits or apply a percent scale in this body. A fractional remaining-build value can consequently lose its fractional part on this path. The packet's three stockpile fields are all gated by slot 0's flag, another difference from the per-slot checks used on receipt/local copying.

The [network dispatcher](../../../src/network/net_game_453d40.cpp) resolves the old live unit and destination identity before calling this function with the packet. Connection/left-player handling is a separate network concern; no raw player pointer travels in this record.

## 4. Local receipt and state preservation

The local-copy branch accepts only destination type 1/2. It calls completed CreateUnit with the same definition, position and low movement-mode bits. Availability, per-type limits and free recipient slots can reject creation. In that case it returns without applying the ordinary local-copy destruction call to the original, although capture notification already occurred.

On success it clears standing move/fire fields in the replacement's runtime flags. With a received packet, it restores health, construction from the integer field, angles and stockpiles for each enabled weapon slot. Without a packet it copies health, construction bits and angles directly from the old record, copies each enabled slot's stockpile and applies kind-4 damage to the old record.

The received-packet branch does not repeat that last damage call in this body. It then applies the old unit's activation mask to the new unit with set and clear calls. It does not clone the entire old record: old orders, exact unit ID, kill experience, script execution stack and every transient motion/resource field are not copied by this function. Normal creation supplies fresh components before selected state is overlaid.

Thus gifting/capturing can require recipient capacity and definition availability, change ID, reconstruct scripts/model state and reset selected command state. The detailed capture work/order timing belongs to order handlers; this is the ownership conversion service they call.

## 5. Definition lookup

`FindUnitType`, `0x488a50`, and `FindUnitTypeId`, `0x488b10`, binary-search the case-insensitively sorted definition table, excluding sentinel entry zero. The former returns a definition pointer or null; the latter returns its stored ID or zero. They depend on the loader maintaining sorted order and valid count. These are definition lookups, not searches for a currently live instance with that name.

`GetEnergyUse`, `0x488f30`, returns explicit nonzero energyuse first. Otherwise a positive wind rating returns negative windFraction times rating, then a positive tidal rating returns negative map tidal strength times rating, otherwise zero. This priority means wind wins when both generator ratings are positive and explicit usage is zero. Negative output expresses generation in this observation, not failed resource spending. Actual economic settlement remains separate.

## 6. Category-mask registry

The global category vector is sorted case-insensitively by reference-counted name. Static initializer/destructor entries `0x4889d0` and `0x488a00` own its container lifetime. Each category points to a zero-initialized 0x40-byte UnitTypeSet, or 512 bits.

`InitUnitCategories`, `0x488be0`, sets the map-loading/category flag to 1; it does not independently populate every named category. `GetCategoryMask`, `0x488c50`, binary-searches for a name and returns its existing mask or allocates/inserts an empty one. Querying an unknown category therefore creates an empty registry entry rather than reporting a spelling error.

`UnitTypeSet::AddTypeOrCategory`, `0x488d30`, first attempts a concrete definition lookup. A nonzero ID sets its bit and returns output marker 1. Otherwise it gets the named category, ORs all sixteen integer words into the destination and sets the output marker to zero. It accumulates into the destination rather than clearing it. This operation copies current category bits; it does not create a live dependency that automatically incorporates future edits to that category.

The bit index assumes an ID within the fixed mask capacity; no separate range check is shown. [Definition loading](../../../src/units/unit_types.cpp) populates categories and binds weapon/no-chase masks. [Selection](../../../src/ingame/selection.cpp) uses masks such as CTRL_F, while [AI command handling](../../../src/ai/ai_player.cpp) accepts individual types or categories. A category is consequently shared classification data, not an automatic weapon-permission or selection action by itself.

`UnitDef::AddToCategories`, `0x488e70`, implements definition-side population. It scans whitespace-separated names, gets or creates each category mask and sets the current definition ID's bit. It then always sets that bit in ALL, even when the supplied list was empty. Names are separated by whitespace here, unlike the comma-separated initial mission clauses. The method accumulates membership, with no local bit-index bounds check or width limit on the legacy token scan.

`FreeUnitCategories`, `0x488bf0`, deletes every mask, clears the vector and resets the flag. Vector element destruction releases names. Cached mask pointers in consumers must not outlive this teardown.

## 7. Container and player-sort support

The category insertion entry `0x488fb0`, assignment `0x489240` and copy constructor `0x489260` preserve reference-counted names and mask pointers while vector storage grows/shifts. Mask objects themselves remain separately allocated; copying an entry does not deep-copy its mask. Explicit category cleanup owns their deletion.

The other support entries are QuickSort (`0x488810`), InsertShift (`0x488920`) and Partition (`0x488960`) for pointer ranges with a supplied predicate. QuickSort chooses a median-of-three pivot, partitions and recurses on the smaller range while iterating the larger, stopping at ranges of sixteen pointers or fewer. The outer sorting routine performs insertion completion. These helpers participate in player ordering used by unit-range allocation; they do not interpret mission strings or independently assign alliances.

## 8. Completeness and dependencies

All 19 annotated functions in this module range are covered, including the category-population method physically retained in unit_types.cpp. The initial mission text and category contents depend on OTA/FBI/AI assets. Ordinary unit creation, reference invalidation, script execution, capture timing, network transport and order execution remain their own modules. The module emits an ownership packet but defines no independent HapiBank account format.

For a missing initial order, inspect clause parsing, name resolution, processed/selectability markers and queue handling. For failed gifting, inspect receiver role/capacity before assuming an owner-pointer issue. For weapon-category mismatch, distinguish an unknown empty mask from a populated category and inspect when masks were copied or modified. These traces connect configuration to runtime behavior without treating a permissive lookup as proof that an action succeeded.
