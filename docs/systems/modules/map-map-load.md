# Mission metadata, schemas, initial objects and checksums

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/map_load`, `0x435d30` through the boundary before `0x437840`. Most behavior is in [map_list.cpp](../../../src/map/map_list.cpp); [map_load.cpp](../../../src/map/map_load.cpp) contains checksum-cache container support. The shared object layout is [mission.h](../../../src/map/mission.h).

This module loads OTA mission definitions into metadata and initial-object tables. It resolves a compatible schema and computes a content checksum from selected TNT blocks. It does not itself render terrain, create live units from every initial-unit record, or maintain terrain occupancy. Those consumers run later in match loading.

## 1. Map-list refresh and selection

`RefreshMapList`, `0x435d30`, manages a mission-owned name list obtained from the [enumeration module](map-map-list.md). When its previous multiplayer-list flag was nonzero and the requested flag is zero, it frees the old list first. If no list exists, it passes the supplied flag as both enumeration controls and stores the returned count and buffer.

It then invokes `LoadMissionByName` on the list pointer, which addresses the first terminated name, and stores the flag. It neither checks the count nor propagates the load result. A refresh call therefore combines list acquisition and attempted initial selection; it is not a guarantee that the first mission successfully loaded. Callers that display a no-maps message perform their own checks.

## 2. Reset before loading

`LoadMission`, `0x435da0`, destroys the existing victory-conditions object and allocates a new one. It resets selected environmental metadata to sentinel or empty values, including surface metal, wind bounds, gravity, tidal strength, lava-world state, sea-level-trigger state, description and planet.

It frees and clears the initial-unit, rule and feature arrays and their counts, then frees and clears the briefing pointer. This happens before locating and validating the replacement definition. A failed load consequently does not preserve the previous mission as an atomic rollback snapshot. It can leave partially initialized metadata and a new condition object.

This reset does not initialize every field of the mission structure at once. Remaining fields are assigned as their header or schema is read. Investigations after failure must identify how far the load progressed rather than treating the entire object as either old or fully new.

## 3. Finding the OTA and terrain paths

For campaign type 1, the loader selects `MISSION<index>` from the campaign TDF. A missing record produces a message and zero. It reads the localized mission name and requires `missionfile`, resolves that name under Maps with an OTA extension, and loads it. Missing OTA data, a missing GlobalHeader, or the unsupported older format produces a message and zero. The campaign header's `maxunits` is read with default 200, and terrain path slot 1 is resolved from the same mission filename with a TNT extension.

For types 2 and 3, it clears the terrain-existence field, copies the supplied map name into the current name, and first tries its Maps/OTA path directly. If loading fails, it asks the translation service for a corresponding name and tries that OTA instead. A successful fallback replaces the current name and supplies the terrain path. Failure to translate or load returns zero.

Type 0 and unsupported types return zero. A null map argument is appropriate for campaign mode because the campaign record supplies the filename; the standalone-map branches require a valid string.

## 4. GlobalHeader contract

After locating an OTA, the loader requires GlobalHeader. It stores that parsed record's checksum field as `headerChecksum`, resolves presentation/restriction paths, and loads briefing text. The path slots and their localization behavior are explained in [campaign selection](game-campaign.md).

The remaining header state includes:

| Inputs | Result |
| --- | --- |
| `mapping`, `lineofsight` | Global initial mapping and sight settings, default 0 |
| Fixed assignments | Single-LOS type set to 1; single-commander-death state cleared |
| `memory`, `numplayers`, `Planet` | Metadata strings retained on the mission |
| `nomovie` | Global movie suppression option |
| `missiondescription` | Translated description, retaining original spelling when lookup changes nothing |
| `minwindspeed`, `maxwindspeed`, `gravity`, `tidalstrength` | Environmental generation and physics parameters |
| `lavaworld`, `nosealeveltrigger`, `waterdoesdamage`, `waterdamage` | World hazard/trigger parameters |
| `killmul`, `timemul` | Scoring multipliers |

Numeric header fields in this phase default to zero. Float fields are read as double-valued TDF results then stored as floats. Header values establish configuration; their downstream use in movement, economy, damage or scoring belongs to those systems.

The new victory-conditions object registers its conditions from the parser while GlobalHeader is selected. This precedes schema selection and initial-object extraction. A later schema failure can thus occur after header and condition state have already changed.

## 5. Selecting a schema

`SelectSchema`, `0x436860`, searches consecutive `Schema <n>` children under GlobalHeader. Missing the next numbered child ends that pass; gaps can hide later children. The schema's `type` string is compared case-insensitively.

For campaign mode, type preference follows difficulty:

| Difficulty | Preference |
| ---: | --- |
| 0 | Easy, Medium, Hard |
| 1 | Medium, Easy, Hard |
| 2 | Hard, Medium, Easy |

An unsupported difficulty returns zero. Within each preference it searches numbered schemas and returns the first match. When an output name is supplied it copies the selected schema name; the parser remains positioned on that schema.

Skirmish and multiplayer search Network 1 through Network 4. Their required player value begins with the global player count but can be raised/replaced by the highest occupied slot index plus one: skirmish examines active setup options; multiplayer examines nonzero, non-minus-one player IDs. It is therefore a slot-span requirement, not necessarily a fresh count of densely packed active players.

When the output-schema pointer is null, as in map eligibility enumeration, the first matching network schema returns success without counting start positions. A map can therefore be listed as eligible even though a later full selection cannot choose a useful start-position schema.

When an output name is requested, each candidate's `specials` children are scanned for `specialwhat` beginning with StartPos, case-insensitively. Candidates with zero such entries are ignored. A candidate replaces the current best if it exactly matches the player value, if the player value is zero, or if it has more starts than the current best while the best is not already exact.

This is not a general closest-fit calculation. With no exact match it prefers the largest observed nonzero count. Exact matches can replace earlier exact matches. When the player value is zero every nonempty candidate can replace the previous one, making search order relevant. At the end it restores the parser's current record to the selected schema and returns whether one was found.

## 6. Schema configuration and meteors

Failure to select a schema reports a map error and returns zero. On success, the loader reads HumanMetal/HumanEnergy into resource slots 0 and ComputerMetal/ComputerEnergy into slots 1, as floats converted from integer fields. It reads SurfaceMetal and resolves the AI profile under `ai` with a TXT extension. An absent profile name uses `default`; merely naming a missing fallback file is not equivalent to testing its successful load here.

MeteorWeapon controls whether meteors are enabled. A nonempty name reads radius, density, duration and interval; any zero parameter triggers loading meteor defaults. An empty name disables meteors and loads defaults. Both branches then install the parameter structure. The [meteor module](map-meteors.md) describes conversion to ticks, scheduling and projectile creation.

Finally the loader extracts initial objects from the chosen schema and returns 1. That return means the metadata/schema path reached its end. Individual unit names, feature names and downstream creation eligibility are not validated as live objects by this function.

## 7. Initial-unit records

`LoadMissionData`, `0x436c30`, reselects GlobalHeader and the named schema. Failure to select either returns without extracting objects. For a units subsection, it counts child records, sizes the three possible strings per record, and allocates one block containing fixed records followed by packed strings. String storage includes additional 0x400-byte allocation rounding/slack. Unit name, identifier and initial mission are nullable pointers into that block, so they must not be freed independently.

Each unit record captures:

- X/Y/Z from integer fields converted to 16.16 fixed point.
- Angle converted from degrees into a 65536-unit turn, then stored in the angle field.
- Player, with zero changed to 1.
- HealthPercentage with default 100, BuildPriority and CreationCountdown.
- MissionCriticalUnit, AiIgnore, AiPriorityTarget and Immunity as bit fields.
- InitialGroup as a four-bit field.

The extraction body does not clamp health to a conventional percentage interval, resolve the unit definition or execute the initial mission. It records the supplied configuration in the field widths shown. Consumers later determine how creation countdowns, ownership, immunity and grouping affect live units.

## 8. Specials, feature records and start lookup

Every specials child receives a rule record initialized with type 0. A `specialwhat` string beginning with StartPos gives it type 1 and stores XPos/ZPos in short fields. A digit immediately after the prefix causes the suffix to be parsed as an explicit ID. Otherwise a running counter supplies successive IDs. Positive IDs are reduced by one, while zero stays zero.

Explicit numeric IDs do not advance the unnamed-entry counter. Mixing explicit and unnamed forms can consequently produce duplicate stored IDs. `CountStartPositions`, `0x437300`, counts all type-1 records, including duplicates. `GetStartPosition`, `0x437320`, returns the first matching type-1 ID with X/Z converted to fixed point and Y set to zero. Failure returns zero without writing a replacement position. A count of N does not establish that every ID from zero to N minus one exists.

Feature records store a 128-byte feature name and integer X/Z. Missing names become empty. Coordinates default to minus one; a negative coordinate clears the feature name, leaving that record present in the counted table but ineffective as a named placement. Actual feature loading and cell insertion occur elsewhere.

`FreeMissionData`, `0x437280`, frees units, rules and features and clears their pointers and counts. It frees and clears the briefing pointer as well. This is the explicit array reset also expanded inside full mission loading.

## 9. Checksum construction and reuse

`ComputeMapChecksum`, `0x4373a0`, combines the stored OTA header checksum with a TNT checksum using XOR. A nonzero cached TNT checksum on the mission returns immediately. Otherwise it searches the process-wide map checksum vector case-insensitively by terrain path. A matching entry supplies the TNT checksum without reopening the file.

On a cache miss it opens the terrain file, reads a 0x40-byte header and requires magic 0x2000. It XORs the checksums of that header, width times height times four bytes at the header's plot-data offset, and, if present, feature-count times 0x84 bytes at its feature-data offset. Temporary buffers are released, the file is closed, and the path/checksum pair is appended to the vector. The returned result is the combined header/TNT value.

The selected TNT blocks do not cover every byte of every visual resource associated with the map. This is the implemented consistency check, not an assertion of complete asset identity. A process cache hit also does not recheck whether the file at that path changed after the earlier computation. Zero is used both as a possible checksum value and as the mission's not-yet-computed sentinel.

Failure to open returns zero. Invalid magic also returns zero, but the shown invalid-magic branch does not close the already opened file. Header and block read results are not checked, and sizes/offsets from the header are used without local overflow or file-bound validation. Those are observable failure-path limits of this implementation.

## 10. Checksum-cache container support

The remaining three entry points in map_load.cpp are vector insertion (`0x437580`), entry assignment (`0x437800`) and entry copy construction (`0x437820`). Entries hold a reference-counted string and an integer checksum. Copy/assignment preserve both, using string-reference lifetime operations rather than duplicating a raw unowned pointer.

Insertion either allocates expanded storage, constructs prefix/new/suffix elements and destroys the old range, or shifts/constructs within existing capacity. Its growth adds the larger of the old size and inserted count to the old size. The observed checksum caller appends one entry. Static initialization and destruction of the vector belong to [map enumeration](map-map-list.md).

## 11. Boundaries and completeness

This module defines no save-bank serializer or network packet layout. Its selected metadata, start positions, restrictions, conditions and checksum feed match loading, lobby consistency, environment systems and mission execution. Runtime unit records and terrain cells are separate from its initial-object arrays.

All 11 annotated entry points in this module range are accounted for. File-format values and field conversions are established from their readers. Actual campaign contents, named unit availability and how later consumers apply each mission flag remain dependent on those assets and downstream systems, rather than being supplied from assumptions about stock missions.
