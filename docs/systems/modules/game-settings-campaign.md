# Save-bank orchestration, campaign bytes and small settings helpers

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The game/settings_campaign group contains nine functions in [settings_campaign.cpp](../../../src/game/settings_campaign.cpp), `0x432520` to before `0x432ba0`. Its scope includes the top-level saved-game state writer/loader and bank adapters, not just campaign registry keys. It coordinates already existing subsystem serializers. The bank's file encoding, save-browser widgets, victory rules and individual unit records remain separate modules.

## 1. Bank interface and ownership

[HapiBank](../../../src/util/hapi_bank.h) is a four-byte object pointing to a bank of named accounts, typed named items and binary safe-deposit boxes. The implementation is in [hapi_bank.cpp](../../../src/util/hapi_bank.cpp). InitBank sets its pointer null and returns the object; CloseBank releases the bank and associated allocations. Opening/selecting an account changes the current account used by subsequent item/box operations.

This module uses bank name Total Annihilation 3.0 and summary account Summary. These are application identifiers supplied to OpenBank/SaveBank, not the Windows registry root and not the executable version's complete compatibility policy. Registry settings below use the separate Total Annihilation key.

OpenSummaryBank (`0x432520`) allocates/initializes HapiBank, opens the supplied file with that bank identifier and Summary filter, then opens/selects Summary and returns the object. An OpenBank false result closes/deletes the object and returns null. The subsequent OpenAccount result is not checked. The bank API can create a missing account on selection, so selection alone is not a schema-validation guarantee.

OpenHapiBank (`0x4325b0`) has the same allocation/open/failure cleanup but passes a null account filter and returns the opened bank without an additional Summary selection. It permits the broader subsystem-account view used for state loading. Both wrappers assign a null object when allocation fails but then invoke obj->OpenBank without a guard, so their apparent allocation test does not implement safe out-of-memory recovery.

FreeSummaryBank (`0x432590`) accepts null, otherwise closes the bank and calls operator delete on its object allocation. Returned bank pointers are caller-owned. Data pointers borrowed from accounts remain tied to that bank's lifetime; these wrappers do not create independent immutable snapshots of every item.

## 2. Complete saved-state load order

LoadSavedGameState (`0x432610`) selects Summary. If maxunits exists it reads that integer with default zero into the unsigned-short unitLimitIni field; absence preserves the existing limit. Out-of-range values are truncated by that assignment rather than validated here.

It then restores subsystems in this exact order:

1. Players.
2. Camera position.
3. Terrain features.
4. Metal plot map.
5. Player-feature plot map.
6. Mapping/exploration data.
7. Units.
8. Meteors.
9. Victory/mission conditions.

Finally it sets the game's loaded bit and returns one. These calls are void at this boundary and their failures are not aggregated. A true result means the orchestration reached its end, not that it validated every account, object reference or asset. There is no local rollback to the prior match, null-bank check or cancellation after a partially restored subsystem.

The order establishes important dependencies: player and terrain-information state precedes [unit reconstruction](units-unit-save.md), and victory conditions are read after the live objects they can refer to. [Feature](map-features.md), [plot-map](map-plot-map.md), [mapping](map-line-of-sight.md), [player](game-players.md) and [meteor](map-meteors.md) references explain their individual storage contracts. This function does not itself relink every unit pointer or reparse the source map; those are delegated/load-lifecycle responsibilities.

## 3. Creating a new save image

SaveGameFile (`0x4326b0`) creates a local HapiBank, explicitly initializes it, starts a new bank and opens Summary. It writes build date and build time unusually: each formatted BUILD DATE/BUILD TIME string becomes an integer item's name with value zero. They are not two ordinary string-valued Date/Time fields.

It then writes the following summary schema through the bank interface:

| Item | Source and interpretation |
| --- | --- |
| maxunits | Current configured unsigned-short unit limit |
| Campaign | Mission object's current campaign name |
| Mission | Mission object's selected mission name, after possible advance |
| Map | The same selected mission name |
| Difficulty | Current game difficulty |
| Side | Current local player's PlayerInfo side |
| Players | Current player count |
| Gametype | Mission object's numeric game type |
| Thumbs | Game's campaign status byte string |
| Description | Optional caller string, omitted when null |
| Game ID | Caller-supplied integer |
| Game Time | Current game tick |

The code assumes the game, mission object, local player info and strings are valid. It does not independently validate every index or guarantee a fallback when GetCampaignName returns null. Individual item-set return values are ignored.

For game type two it additionally writes CommanderDeath, Location, Mapping, LineOfSight and LineOfSightType from the skirmish Options object. Other game types do not take that summary branch. These values describe setup rules; writing them does not apply a new rule to the active match.

## 4. Live match versus between-mission save

Frontend state six selects the live-match payload. Other states call Mission::AdvanceMission before writing Mission/Map. They then write BetweenMissions=1 and restore selection using the stored g_game->mission index. This is not a save/restore of an arbitrary previous local mission index: it specifically uses that game field.

The between-mission image therefore names the advanced mission while retaining a restored in-memory selection afterward. It contains summary/progression metadata rather than a complete snapshot of current unit/terrain state. [Campaign selection](game-campaign.md) explains mission advancement and path updates. Saving can temporarily mutate selection even though its main purpose is file output.

State six instead opens the Radar Image box, saves the supplied finalSurface through SaveSurface, then invokes these serializers in order:

1. Camera position.
2. Players.
3. Units.
4. Mapping/exploration.
5. Terrain features.
6. Player-feature plot map.
7. Metal plot map.
8. Meteors.
9. Victory/mission conditions.

Subsystem serializers manage their own account/box selection. This wrapper does not force them all into the initial Radar Image box or inline their records into Summary. Nor does it freeze the simulation, suspend networking or lock every subsystem while serializing; an appropriate stable save point must be established by callers.

## 5. Final file operation and failure boundary

After constructing the in-memory bank, SaveGameFile calls SaveBank with the requested path, Total Annihilation 3.0 identifier, compression argument one and audit argument zero. It stores that result, closes the local bank and returns the stored value. It does not emit an audit dump through this call, retry another path, or retain the temporary bank for later inspection.

The wrapper does not test NewBank, OpenAccount, OpenNamedBox or each item/serializer result before the final save. The underlying bank/file implementation determines detailed compression, disk-write and compatibility outcomes; this top-level return is not a per-subsystem completeness report. It also does not implement an atomic rename or backup rotation at this level. Those behaviors must be established by the actual lower file routine or higher save caller, rather than assumed from the SaveGameFile name.

No game code, binary bank or registry was executed/modified to write this reference. This describes the serialization calls, not the result of creating/loading a runtime save.

## 6. Arm/Core campaign registry byte strings

LoadCampaignRegistryName (`0x432a80`) chooses CoreCamp for any nonzero first argument and ArmCamp for zero, then requests 26 bytes from that registry value into the supplied output. The provisional name does not make the result necessarily a campaign filename. Its fallback is 25 bytes of value 0x55, ASCII U, followed by NUL, matching a fixed-length campaign-status representation supplied to campaign consumers.

On a read failure it installs that complete fallback. On a successful read it does not validate the per-byte status alphabet, fill missing positions or explicitly force byte 25 to NUL. It inherits accepted incomplete-data/type behavior from [registry utilities](util-registry.md). The caller must supply at least 26 writable bytes.

SaveCampaignRegistryName (`0x432b00`) selects the same key, writes NUL at caller buffer byte 25 and saves it as a registry string. It mutates caller storage before writing; it is not a const input/snapshot API. Embedded earlier NULs shorten the saved string because the writer uses strlen. No checksum, campaign-file existence check or per-mission completion computation happens here.

These registry bytes are distinct from Summary's Campaign filename and Thumbs status item. Their related purpose does not make their storage formats interchangeable, and neither routine independently awards mission completion or unlocks a campaign mission.

## 7. Small explicit setting saves

SaveAllMissionsSetting (`0x432b60`) writes only bit zero of allMissionsFlags under AllMissions. Other bits are discarded. SaveNumSkirmishPlayers (`0x432b80`) writes the stored integer under NumSkirmishPlayers without validation. Both use the void registry write interface despite an integer-shaped declaration in this file and expose no reliable success return.

These separate functions complement [SaveSettings](game-settings.md), which does not persist those two fields in its main batch. The player-count writer does not repair LoadSettings' ineffective count validation or clamp a count to the physical ten-player Options array.

## 8. Coverage and persistence interpretation

All nine owned entries were inspected, including both bank openers, cleanup, complete load/save call sequences, live/between-mission branches, all summary item families and four registry helpers. Unit/component records, HapiBank encoding, save browser and victory algorithms are separate unfinished or already documented module responsibilities, not silently treated as covered by this orchestration.

The bank and temporary save image are locally owned; game objects, mission strings, surface and caller registry buffers are borrowed. No audio voice cursor or movie decoder position is added by this wrapper. There is no network save-state exchange in these bodies. Documentation is based on static source tracing without builds, installations, game execution or copied source listings.
