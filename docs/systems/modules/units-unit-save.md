# Unit save records and dependency reconstruction

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_save`, four annotated functions in [unit_save.cpp](../../../src/units/unit_save.cpp), `0x486fd0` through `0x487af0`. This module writes live-unit records and reconstructs units through ordinary creation plus component loaders. It also contains mission-unit name lookup against a parallel runtime-ID table.

## 1. Account layout and version gate

`SaveUnits`, `0x4876c0`, opens the Units account and scans the inclusive global unit-slot range. A slot is saved when runtime flag 0x10000000 is set. Each saved unit gets a sequential numbered box containing a 0xb8-byte packed record, while the record's unit ID retains its original slot identity.

Associated component data uses named boxes: COB state is Script followed by the sequential save index; order names use `u%04xm%04x`, combining hexadecimal unit ID and zero-based order number. Motion and resource-account serializers supply their own component state. A numbered save-box index is therefore not interchangeable with a runtime unit ID.

After at least one unit is saved, the account stores Number of Units and Version 0x11. With no live units, the function does not explicitly write zero count/version items. Whether an old account can retain prior values depends on the surrounding bank lifecycle; this body must not be treated as independently clearing old account contents.

`LoadUnits`, `0x486fd0`, requires the Units account and exactly version 0x11. It iterates the declared count, opens each numbered box and reads the record before asking LoadUnit to reconstruct its stored ID. Other versions are skipped without an alternative conversion path in this function.

## 2. Packed unit record

The 0xb8-byte record stores values and IDs, not runtime component pointers:

| Region | State |
| --- | --- |
| Definition and identity | 32-byte definition name, player byte, slot ID |
| Components | Total order count, motion-present integer |
| Physical/combat state | Position, angles, health, kill count and three compact weapon-slot records |
| Relationships | Live carrier ID/piece and live attacker ID, last attacker slot |
| World state | Extraction value, footprint-origin cell, sight cache and footprint dimensions |
| Scheduling/construction | Squad index, work time, remaining construction fraction |
| Damage/display/sync | Last damage kind, health percentages, sight frame, recent-damage timer, activation state, dirty flags |
| Packed flags | Low COB-state bits and selected unit runtime-flag ranges |

Some fields appear as integers in the save module's private view although other runtime views identify their bits as floats or pairs of shorts. The assignment preserves those field bits; it is not an integer conversion of buildLeft or extraction into a different quantity.

Weapon records retain compact target/state integers, selected timer/counter fields, selected slot flag bits and the referenced definition's index byte. Full aim callback objects and raw model/weapon pointers are rebuilt by initialization rather than copied from disk.

Runtime flag bit 12 and the highest runtime-flag range are not simply dumped verbatim. Definition/lifecycle initialization establishes some flags, and the loader merges the saved subset. The format therefore represents selected persistent state, not a memory snapshot of the complete 0x118-byte unit object.

## 3. Serialization order

For each live unit, SaveUnits writes COB state first, primary orders in list order, then secondary orders continuing the same order-number sequence. It counts both lists together. Motion is serialized only when present, followed by resource accounts.

It then fills the main record. A carrier is saved only when its live flag is set; otherwise carrier ID is zero and transport piece is 0xff. The attacker likewise becomes zero unless its record remains live. Dead/stale relationships are not intentionally persisted just because a pointer field is nonnull.

The record is written to its sequential numbered box and the saved count increments. Save-script calls are made through the script pointer without a local null check, so this path relies on saveable unit/script assumptions established by definition initialization and surrounding loading.

## 4. Recursive loading by original slot ID

`LoadUnit`, `0x487080`, immediately returns null for ID zero and immediately returns an already-live slot. Otherwise it scans the numbered unit boxes for a complete record with the requested ID. Failure to open a scanned box or read its complete record returns null immediately, rather than skipping ahead through every malformed entry.

On finding the record it resolves the definition by name and calls ordinary [CreateUnit](units-units.md) with completed state, saved low movement-mode bits and the original explicit slot ID. The ordinary availability, capacity, per-type-limit and player-range constraints still apply. If creation fails the unit is not reconstructed.

Creating before following relationships gives the slot its live flag. Recursive requests for that same ID can then return the existing object, allowing carrier/attacker dependencies to find it without recursing forever through a cycle. This is a partly reconstructed live object at that point, not proof that all its components have finished loading.

The loader restores angles, health, kills and exact saved Y. It recursively loads the carrier and attaches the current unit to its saved piece, recursively loads the attacker, then restores transport/attacker metadata, world/resource-related fields, squad/work/construction values, damage/sight/activation state and packed flags.

Creation initially used completed construction and registered the unit with map, counters and victory conditions. Saved buildLeft is restored afterward. Observers during reconstruction must account for this staged transition rather than assuming an unfinished saved unit was initially created through the normal zero-health unfinished branch.

## 5. Membership and occupancy reconstruction details

The squad field is assigned from the record before SetUnitSquad is called with that same value. Ordinary creation assigned squad 0. Because SetUnitSquad removes from the unit's currently recorded group before appending, this ordering can leave an original squad-0 vector entry when the saved group is different. The unit-field-based group dispatcher and vector-based consumers can then disagree. This follows the two bodies directly; the general membership contract is documented in [squads](units-squads.md).

The loader restores footprint and cached cell fields after creation already inserted the initial definition-derived occupancy. Near the end, low COB state bit 2 causes ForceNeighborFootprintReclaim. Correct final map/attachment reconstruction consequently depends on the complete restore sequence and surrounding game loading, not merely assigning the saved coordinate fields.

## 6. Orders, scripts, motion and weapons

Resource accounts are loaded for the unit. A nonzero saved motion-present field asks the existing motion component to load its state; it does not allocate an arbitrary missing motion object at that point. The selected definition and normal initialization must provide the compatible component.

The combined saved order count is reconstructed in ordinal order using the per-unit/per-order names. Each new order is directed to the secondary list when flag 0x40000 is set, otherwise the primary list. Pointer-to-tail variables preserve order within each category. If a primary head exists, its effects are reattached to the unit.

COB state loads from Script followed by the matched record's numbered-box index. This is why recursive scanning retains the matched index separately from the unit ID. The three weapon slots then receive their saved selected state fields and flags. The shown assignment writes the saved definition-index byte through the initialized weapon definition pointer; it does not independently replace that pointer with an arbitrary saved address.

Definitions, model state, script objects and aim callback objects therefore come from current loaded assets. The record overlays state onto those objects. Definition changes between save and restore can affect compatibility even though the original unit name still resolves.

## 7. Short records and trusted-input limits

The outer loader accepts either a full 0xb8-byte record or a record two bytes shorter. On the shorter branch it sets the record ID to zero. LoadUnit then returns null for that ID, so this branch does not reconstruct an equivalent unit through a general legacy-record migration. Other short lengths are skipped by the outer loop.

The inner lookup requires exact-size reads and does not independently validate all saved IDs, relationship indices, name termination, order counts or component compatibility. CreateUnit supplies some creation constraints, but this is not a general untrusted-save validator. Failure midway is not rolled back into the original pre-load state.

## 8. Mission-unit identifier lookup

`FindMissionUnit`, `0x487af0`, searches the current mission's initial-unit table alongside a caller-owned integer array. A match can use the mission record's identifier or unit name, case-insensitively, and must have a nonzero associated integer. It returns that integer, usually a caller-maintained runtime identity, rather than returning the mission-table index.

With a zero starting value it scans from entry zero. With a nonzero value it first finds that value in the parallel array, incrementing the index as it tests. The subsequent name search begins after the matching entry, which supports finding a later match. If no such later record exists it returns zero. The initial value-search loop reads its first entry before its end test, so valid nonempty table/array input is an assumed contract.

The mission table supplies names/configuration, while the parallel array supplies current runtime associations. This routine does not spawn absent units, resolve every same-type live unit, or serialize that parallel array itself.

## 9. Boundaries and completeness

All four module entry points are covered. Disk account transport and box lookup belong to HapiBank; order/COB/motion/economy serializers define their component payloads. Network unit replication uses different compact events and is not the same operation as loading these records.

For a missing saved unit, check version, record readability, type-name resolution, slot ownership, availability and limits. For a restored transport relationship, trace recursive creation before attachment and final field overlay. For lost orders or effects, inspect combined order numbering, list flags and reattachment. For state that appears briefly completed during restoration, inspect the completed CreateUnit call followed by saved buildLeft restoration.
