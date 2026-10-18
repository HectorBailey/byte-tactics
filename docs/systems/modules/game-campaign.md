# Campaign sequences, mission selection and briefing paths

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `game/campaign`, addresses `0x435100` through the boundary before `0x435d30`. The implementation resides in [map_list.cpp](../../../src/map/map_list.cpp) and the [campaign-file probe](../../../src/game/campaign.cpp), using the shared [Mission class](../../../src/map/mission.h). Module ownership follows the address inventory rather than the physical filename.

This module turns campaign records and map-selection names into requests to the mission loader. It supplies briefing text and resource paths to presentation code. It does not decide whether a mission has been won, spawn its initial units, or write campaign progress to a save bank. Those systems consume or alter the selection state through this interface.

## 1. Selection state

The mission object distinguishes its numeric game type, campaign filename, parsed campaign TDF, current mission index, current mission name, translated/display name, file paths, briefing buffer, and cached terrain length. These values answer different questions. A nonempty mission name is not proof that every asset loaded successfully, and a cached nonzero terrain-file length is not a completed terrain parse.

`GetGameType` (`0x435100`) returns the numeric type. `GetCampaignName` (`0x4352b0`) returns the internal campaign-name buffer only when nonempty, otherwise null. `GetMissionName` (`0x435c30`) and `GetTranslatedName` (`0x435c20`) return their buffers directly. `HasMissionName` (`0x435c40`) tests only the first byte of the mission name. `GetMissionIndex` (`0x435c50`) returns the current index.

Returned strings belong to the mission object. Reloading names, changing modes or deleting the object invalidates assumptions about their contents or lifetime. The getters do not allocate snapshots for their callers.

## 2. Loading or clearing a campaign

`LoadCampaign`, `0x435110`, unloads the embedded campaign TDF, copies the supplied campaign filename and clears all nine path slots. An empty filename then returns. This empty-name path provides initialization/reset behavior; it does not proceed to load mission zero.

A nonempty name is resolved into path slot 0 using the `camps` directory and TDF extension. The method loads that file into the embedded TDF parser. On failure it opens a message box identifying the requested campaign file, calls itself with the empty name to clear the campaign configuration, and returns.

On success it clears the TNT checksum, sets the mission index to zero, and invokes the mission loader with a null map argument. The null argument is meaningful for campaign mode: the later loader obtains the OTA filename from the selected `MISSION<n>` record instead of accepting a standalone map name.

The method returns no success value and does not propagate the mission loader's result. A successfully parsed campaign file therefore does not guarantee that its first mission's OTA, header or schema is valid. Callers must inspect the resulting state or use a selection operation with an explicit result when that distinction matters.

The failure-message buffer is only 0x80 bytes, while a resolved path can be longer. The source identifies the unbounded formatting as a suspected original buffer overrun. A second nonempty-name test is redundant in this body; the path-building call does not alter the caller's input pointer.

`CampaignFileExists`, `0x4352d0`, separately builds the ordinary camps/name/TDF path and attempts to open it through the file-service function currently named HAPI_OpenFileAppend. A successful open is immediately closed and returns 1; failure returns zero. This probe does not parse the TDF, validate numbered missions or use the localized-directory preference implemented by BuildCampaignFilePath. Its result is an open-success observation, not campaign-content validation.

## 3. Numbered mission records and lists

`CountMissions`, `0x4356f0`, requires a nonempty campaign name, resets the campaign parser's record cursor and searches consecutively from `MISSION0`. The first missing numbered record ends the count. A later `MISSION7` does not bridge a missing `MISSION6`.

`MissionExists`, `0x435980`, repeats this consecutive-record count and returns whether the total is greater than the supplied index. It has no independent nonnegative-index check. Its name should therefore not be interpreted as full validation of arbitrary signed input: for example, a negative index can satisfy that final comparison even when it is not a usable numbered mission.

`BuildMissionList`, `0x435760`, counts the same sequence. For a nonzero count it allocates count times 256 bytes, initializes the first byte, and writes variable-length terminated display names in sequence. Each record's name comes from localized `missionname`; a missing name receives the built-in unnamed-mission error text. The list is walked with the terminated-string helper, rather than indexed as fixed 256-byte records despite that allocation bound.

A blank campaign or zero mission count returns zero without assigning a new output allocation. Callers must use the returned count before reading the output pointer. If record selection unexpectedly fails during the construction pass, the function returns zero after the allocation has already been assigned to the output; the body does not free that allocation in its failure branch.

## 4. Loading by display name or by index

`LoadMissionByName`, `0x435a20`, clears the TNT checksum before dispatching by game type.

For type 1, it builds the campaign display-name list and searches it case-insensitively. The first matching name wins. It releases the temporary list, stores the matching index, and returns the result of loading that mission. A name absent from the list produces zero. Selection is by localized mission name, not directly by the underlying OTA filename; duplicate names therefore make later duplicates unreachable through this search.

For types 2 and 3, it passes the requested map name to the mission loader. After a successful load, when the preferred language exists and differs from English, it lowercases the input into a temporary buffer and translates it into the display-name field. If the translated result compares equal to the input, it restores the input spelling. Other cases copy the input into the display-name field directly. It returns the mission loader's result.

Unsupported types return zero. As elsewhere in these legacy interfaces, input strings are expected to fit fixed internal buffers. The 0xff-byte bounded display-name copy on the translation path does not explicitly append a final terminator in this body when the result fills that bound.

`SelectMission`, `0x435c00`, clears the checksum, assigns the requested index and returns `LoadMission(null)`. It performs no preliminary bounds check and does not restore the old index if loading fails. Missing records are diagnosed by the later loader.

`AdvanceMission`, `0x435c60`, counts consecutive records and checks whether there is a mission after the current one. If so it clears the checksum, increments the index and invokes the loader. Its return value is 1 because another numbered entry existed; it does not forward the loader's success value. It returns zero when no next entry exists. Victory/progression callers must not read a return value of 1 as proof that the next mission's assets loaded.

## 5. Resolving the nine resource-path slots

`SetNameSlot`, `0x4353b0`, copies text into one of nine 256-byte slots. Slot 1 additionally caches the named file's length through HAPI, or zero for an empty path. There is no local slot-index range check or bounded-copy guard. These are internal APIs whose callers must supply valid indices and fitting strings.

`BuildCampaignFilePath`, `0x435430`, stores an empty slot immediately for an empty name. For a nonempty name and an available language suffix, it first tries a directory suffixed with that language. It strips any extension from the assembled name, adds the requested extension, and attempts to open the localized path. If that succeeds it closes the file and stores that path. Otherwise it constructs and stores the ordinary directory path. The fallback path is stored without an equivalent open-success requirement.

The campaign and following mission-loader calls establish these slot roles:

| Slot | Role | Typical directory and extension |
| ---: | --- | --- |
| 0 | Campaign definition | `camps`, TDF |
| 1 | Terrain file | `Maps`, TNT |
| 2 | Briefing text | `camps\briefs`, TXT |
| 3 | Narration | `camps\briefs`, WAV |
| 4 | Mission hint | `camps\hints`, TXT |
| 5 | Glamour picture | Supplied path with PCX extension |
| 6 | Allowed-unit definition | `camps\useonly`, TDF |
| 7 | AI profile | `ai`, TXT |
| 8 | Glamour sound | `camps\briefs`, WAV |

`GetNameSlot`, `0x4356c0`, returns null for an empty slot and otherwise a pointer to its internal buffer. A nonnull fallback path is not equivalent to a successful file-open test. Slot 1's cached byte length supplies an additional existence/size observation for the terrain file.

## 6. Briefing-buffer lifecycle and an exceptional path

`LoadBriefing`, `0x435320`, frees the previous briefing buffer first. If slot 2 is empty, it sets the pointer to null and returns. For a nonzero reported file length, it allocates length plus one, reads that many bytes and writes a terminator. `GetBriefing`, `0x4353a0`, returns the stored pointer without copying or validation.

There is a significant zero-length path: when slot 2 is nonempty but HAPI reports length zero, the function does not explicitly clear the pointer after freeing an old buffer. If an old buffer existed, the member can retain a freed address. This observation follows the body directly; ordinary mission loading clears the briefing pointer before this call, but a separate reload with existing data can reach the problematic sequence. The method also does not inspect a read-result value before terminating the allocated buffer.

## 7. Metadata access and terrain-size tiers

`GetTerrainLength`, `0x4358f0`, returns the cached length/existence field established by slot 1. `GetDescription` (`0x435900`) and `GetPlanet` (`0x435910`) return internal strings populated by the mission loader.

`GetTerrainSizeTier`, `0x435920`, maps the cached terrain byte length to tier values:

| Byte-length interval | Tier |
| --- | ---: |
| Below 0x3e6666 | 0x10 |
| 0x3e6666 through below 0x600000 | 0x18 |
| 0x600000 through below 0x800000 | 0x20 |
| 0x800000 through below 0xa00000 | 0x30 |
| 0xa00000 through below 0xc00000 | 0x40 |
| 0xc00000 and above | 0x80 |

These tiers are derived from file length, not directly from map width or height. An absent/zero-length file takes the first tier. The getter does not open or validate the terrain file again.

## 8. Dependencies and completeness

The [map-enumeration module](map-map-list.md) owns construction and replacement of the mission object. HAPI supplies file lookup, size and reads; TDF supplies campaign records; localization supplies display text and preferred-language paths. The later map-loading module fills map metadata and mission arrays, and condition/endgame systems decide when selection should advance. The current module defines no packet payloads or persistent bank fields of its own.

All 23 annotated entry points in this address range are covered above, including the separate file probe and simple getters. The implementation makes the difference between selection, path resolution, metadata existence and successful mission loading explicit. Asset contents still determine the actual campaign order, localized strings and available resources; those values are not inferred from conventional game behavior.
