# Map enumeration and mission-object ownership

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/map_list`, from `0x434a30` up to the next module boundary at `0x435100`. Its implementation is the first portion of [map_list.cpp](../../../src/map/map_list.cpp). That source file also contains later campaign and map-loading modules. The filename therefore has a broader scope than the module documented here.

This module owns the OTA enumeration cache, replaces the active mission object when the game mode changes, and establishes the mission object's allocation and destruction contracts. It discovers eligible map names. It does not load terrain cells, instantiate mission units or draw the map selector.

## 1. State and ownership

There are three distinct collections involved:

| State | Representation | Owner and purpose |
| --- | --- | --- |
| Global OTA enumeration | Allocated sequence of terminated names, byte size, count, completeness flag | Shared cache used by map-selection callers |
| Active mission | `g_game->mapInfo`, a `Mission` instance | Mode-specific metadata, current map/campaign and mission data |
| TNT checksum cache | Static vector of string handles and checksums | Reuses map checksum results in the later map-loading module |

These collections have different lifetimes. Replacing the active mission does not by itself invalidate the global OTA enumeration. Freeing the global enumeration and mission is an explicit operation. The checksum vector is a file-static object with process-lifetime initialization and destruction.

The shared [Mission declaration](../../../src/map/mission.h) contains the mode, campaign name, nine path slots, parsed campaign TDF, current mission names, briefing buffer, map list, environmental metadata and initial mission-object arrays. Its total layout is 0xec4 bytes in this revision. Many of these fields are populated by the following modules rather than by the constructor here.

## 2. Replacing the active mission

`SetMissionType`, `0x434ab0`, first checks the existing mission. If its type already equals the requested value, it returns without resetting the object. This preserves selected metadata and buffers when callers repeat the current mode.

If the type differs, it deletes the current object and constructs a replacement. The new result is stored in the singleton pointer, including a null result if allocation supplies one. The shown body does not provide a secondary fallback object. Callers that immediately dereference the mission depend on successful allocation.

The argument denotes a game-mode value rather than a player or spatial owner. Frontend paths select type 1 for campaigns, type 2 for skirmish and type 3 for multiplayer. Application reset paths use type 0. These meanings are established by [frontend state handling](../../../src/frontend/frontend.cpp), [campaign selection](../../../src/frontend/campaign_menu.cpp), and [game-state lifecycle](../../../src/game/game_state.cpp).

Deleting a mission also destroys the current global victory-conditions object, as described below. A mode change can consequently invalidate condition state as well as the selected map metadata.

## 3. Enumerating eligible OTA maps

`LoadMapList`, `0x434bf0`, accepts an optional output-buffer pointer and two integer controls. Their observable meanings are more precise than assigning them guessed original names:

- The second argument requests an early exit after the first eligible map when nonzero.
- The third argument allows an incomplete cache buffer to be transferred to the caller rather than copied.
- A null output pointer requests the count without obtaining a separate buffer. On a cold cache it still performs enumeration and creates the cache.

When no cached buffer exists, the function changes the cursor to mode `0x14`, allocates an initially empty one-byte list, and asks file services for `Maps\*.ota`. Each candidate is converted to a Maps/OTA path and parsed as a TDF file. It is accepted only if loading succeeds and the active mission's `SelectSchema(3, parser, null)` succeeds. Finding a filename alone is therefore insufficient. Schema eligibility is delegated to the map-loading module.

For an accepted candidate, it strips the extension, makes a lowercase copy for translation lookup, and chooses the translation unless that result compares case-insensitively equal to the lowercase lookup key. In that fallback case it retains the candidate's original filename spelling without its extension.

Each accepted name is appended with its terminator. An additional trailing zero remains after the last name. The stored byte size begins at one and grows by each name length plus one. Consumers must walk terminated entries using the returned count; this is not an array of fixed-size filename records.

Enumeration preserves directory-service order. Sorting belongs to consumers: the [skirmish selector](../../../src/frontend/skirmish_menu.cpp), for example, obtains a copied list and sorts it before assigning it to the list box. Sorting that caller-owned copy does not reorder the shared cache.

## 4. Full scans, short scans and network servicing

A nonzero second argument stops at the first eligible candidate, rather than at the first file examined. If every candidate fails parsing or schema selection, the scan can still reach the end and return zero. The completeness flag remains false for this short-scan mode, even if no early exit occurred.

A zero second argument marks the cache complete after examining the directory list. When this is a full scan and the active mission has type 3, the loop services incoming network packets and, when the packet manager is enabled, flushes queued packets. This work occurs after each examined candidate, whether or not that file produced a map entry. Other modes and short scans do not execute that loop branch.

The function restores cursor mode `0x13`, records completeness, and re-enters itself to use the normal cached-result path. It does not independently load map geometry or start a match while servicing the enumeration loop.

## 5. Cache copies and transfers

On a cache hit, the function returns the cached count immediately. It does not automatically rescan an incomplete buffer merely because the new caller requests a full scan. The distinction between an incomplete buffer and no buffer is therefore significant.

If an output is requested, the normal path allocates the recorded byte size and copies the list. The caller then owns a separate allocation. When the third argument is nonzero and completeness is false, the function instead returns the existing buffer and clears the global buffer pointer. The count and byte-size fields remain available, but the next call sees no buffer and performs a new scan.

The later `Mission::RefreshMapList`, `0x435d30`, passes its multiplayer argument as both controls and stores the resulting allocation in the mission. This explains how a fast initial selection can consume a short list without permanently installing that short list as the global full enumeration. The later method also frees its mission-owned list when switching from its nonzero mode back to zero.

An empty list is still an allocated buffer. Callers should test the count before treating its contents as a usable map name. The skirmish selector explicitly reports that there are no maps when the count is zero. The enumeration body contains no independent allocation-failure recovery around its writes, reallocation or copies, so it relies on the allocation layer's contract.

## 6. Mission construction and destruction

The constructor, `0x434f70`, zeros the unit/rule/feature counts and pointers, file-existence state, map-list state, multiplayer-list mode and briefing pointer. It empties the current mission-name strings, stores the supplied type, then invokes `LoadCampaign` with the empty string. That later method unloads the campaign TDF and clears the campaign/path slots. Construction does not imply that every environmental metadata field already describes a valid loaded map.

The destructor, `0x434ff0`, first frees and deletes the global victory-conditions object when present, then clears its global pointer. It releases the mission's units, rules and features and resets their pointers and counts. It frees the briefing and the mission-owned map list. Destruction of the embedded TDF object follows normal member lifetime rules.

The body contains a second round of feature/rule/unit pointer checks after those pointers were cleared. Those checks are redundant in the shown path and do not cause a second free. This is an implementation detail worth preserving when tracing cleanup, rather than evidence for two separately owned copies of each array.

`FreeOtaEnumCacheAndMission`, `0x434b90`, frees the global enumeration buffer if present, clears the buffer pointer, completeness flag, byte size and count, deletes the active mission, and clears its singleton pointer. This is the broad teardown operation; `SetMissionType` is the narrower replacement operation.

## 7. Static checksum-cache support

The annotated static initializer `0x434a30` and registered destructor `0x434a60` establish the file-static vector of checksum-cache entries. Each element contains a reference-counted string handle and an integer checksum. Destruction releases the string handles and vector storage through the container lifecycle.

The later checksum function and vector insertion helpers use this state, but their algorithms are part of `map/map_load`. An OTA-list reset does not clear this static vector in the bodies documented here. It must not be confused with the name-list buffer just because both cache map-related data.

## 8. Boundaries and investigation consequences

This module defines no save-file records or network packet payloads. Save restoration can select a mode through the shared mission interface, and multiplayer discovery keeps the connection serviced during a long full scan, but neither makes this a serializer or a network map-transfer implementation.

For a missing map, trace directory discovery, TDF loading, schema selection and translation separately. For a stale list, inspect whether an incomplete buffer remained cached, whether a caller took ownership, and whether explicit teardown ran. For metadata unexpectedly surviving a repeated mode selection, the same-type early return is the controlling behavior. For lost victory conditions, replacing or deleting the mission is a relevant lifetime event.

All seven annotated entry points in this module range are accounted for: two static lifetime functions, mission replacement, global teardown, enumeration, constructor and destructor. Original names for the enumeration controls remain provisional; their branch behavior and ownership effects are established above.
