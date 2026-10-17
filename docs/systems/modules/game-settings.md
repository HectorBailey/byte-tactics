# Persistent game options, defaults and unit restrictions

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The game/settings group owns six entries in [settings.cpp](../../../src/game/settings.cpp), `0x42f910` to before `0x4318c0`. It reads/writes options, supplies game-key registry adapters, stores ten track bytes as values and applies a mission's use-only unit list. It is not a complete saved-match serializer or the frontend widget implementation.

## 1. Storage and application lifecycle

Most keys live below the current user's Software/Cavedog Entertainment/Total Annihilation registry subkey. Per-player skirmish fields use its Skirmish child. [Registry utilities](util-registry.md) handle opening/closing keys, byte transfer and accepted errors. In particular, a true read result can include ERROR_MORE_DATA and does not validate stored value type or exact length. This module normally treats that result as usable data; it does not add a universal type/length-repair pass.

LoadSettings (`0x42f9a0`) requires the game, Sound object, Options allocation and mission/map interface already initialized. [game_state.cpp](../../../src/game/game_state.cpp) invokes it during setup; [frontend.cpp](../../../src/frontend/frontend.cpp) can reload on transitions. It changes game fields and, for selected audio options, immediately invokes Sound methods. Loading does not itself switch resolution, reconstruct all render caches or synchronize multiplayer options.

SaveSettings (`0x430f00`) is called from menu, console and transition code. It writes individual values through void wrappers rather than a transaction. Failure is not reported or rolled back as one unit. Loading can also write missing defaults, so this loader is not purely read-only.

## 2. Main display, interface and rendering settings

The following missing values are assigned and written back during load, except where the behavior column identifies a different rule. Values stored in one-bit fields retain their low bit; assigning two is not equivalent to normalizing every nonzero value to true.

| Key | Missing default | Loaded interpretation |
| --- | ---: | --- |
| Interface Type | 0 | Values above one become one; negative values are retained |
| DisplaymodeWidth | 640 | Integer stored without supported-mode/range validation |
| DisplaymodeHeight | 480 | Integer stored without supported-mode/range validation |
| side | 0 | Integer side selection |
| Difficulty | 1 | Stored low 16 bits on successful read |
| scrollspeed | 32 | Truncated to an unsigned byte |
| screenchat | 1 | Integer field |
| damagebars | 0 | One-bit visual option |
| Anti-Alias | 1 | One-bit visual option |
| Shadows | 1 | One-bit master visual option |
| FeatureShadows | 1 | One-bit feature-shadow option |
| VehicleShadows | 1 | One-bit vehicle-shadow option |
| Shading | 1 | One-bit model-shading option |
| DitheredFog | 0 | One-bit fog-dithering option |
| Gamma | 12 | A stored value ten is changed to twelve; other values are retained |
| SwitchAlt | 0 | One-bit input option |

These flags are consumed by [model rendering](graphics-model-render.md), [terrain/fog rendering](map-line-of-sight.md) and interface controls. They do not alter weapon damage or construction payment. The gamma read changes only the option field here; palette/renderer code applies the actual presentation transform.

The loader also reads DisplaymodeDepth solely for an auxiliary option test. Only value 256 permits a Games value of one to set uiOptionFlags bit 1; otherwise that bit is cleared. It then unconditionally sets UI bits 2/3 and clears bit 4. This body does not use DisplaymodeDepth to select a new RGB rendering depth. AllMissions defaults to zero and otherwise assigns its low bit; it is not written back by this missing-value branch.

## 3. Audio settings and immediate side effects

| Key | Missing behavior | Successful-load behavior |
| --- | --- | --- |
| Sound Mode | Write current Is3DEnabled()+1, set stored three-bit mode to one | Enable 3D only for exact value two; otherwise disable it; assign low three bits |
| MixingBuffers | Set maximum eight, no default writeback | Pass integer directly to SetMaxBuffers |
| RestoreVolume | Clear one-bit option, no writeback | Assign low bit |
| WaveOutVolume | Leave current mixer value | If RestoreVolume is set, call SetWaveVolume |
| CDAudioVolume | Leave current mixer value | If RestoreVolume is set, call SetAuxVolume with nontemporary mode |
| unitchat | Byte value ten | Truncate supplied value to a byte |
| unitchattext | Byte value five | Truncate supplied value to a byte |
| musicmode | Bit value one | Assign low bit |
| cdmode | Byte value four | Truncate supplied value to a byte |
| ackfx | Bit value one | Assign low bit |
| buildfx | Bit value one | Assign low bit |
| speechfx | Bit value one | Assign low bit |
| fxvol | Integer 27 | Store integer |
| musicvol | Integer 32 | Store integer |

Other than the Sound Mode special branch, missing values in this table are not automatically written. Sound Mode's missing branch does not call Enable3D/Disable3D, so its stored mode and preexisting device option can differ until another update.

SetMaxBuffers does not clamp the requested count; invalid persisted values inherit the [sample-channel bounds/eviction weaknesses](sound-samples.md). Mixer setters can fail, but LoadSettings does not aggregate those return values into a load result. [Unit speech](sound-sound.md) uses the chat settings as strict priority thresholds and uses speechfx's bit as an audio gate. These are preference inputs rather than voice-file guarantees.

## 4. Names, password and capture/output fields

Password is read with an 11-byte capacity; failure sets its first byte to NUL. Nickname normally uses 17 bytes with the same empty fallback. When online configuration is loaded and its lobby name is nonempty, that name instead overrides the registry nickname by copying at most 16 characters after clearing the destination. A nonempty command-line host-game name similarly overrides Game Name; otherwise Game Name is read with 17-byte capacity and empty fallback.

The raw registry read paths do not add a final terminator after a full-size or accepted-more-data result. The override paths' cleared destination plus strncat establish termination for their bounded inputs. Password is written as an ordinary string setting; this group implements no encryption/obfuscation or authentication protocol.

Image Output Directory is read with 256-byte capacity. On failure it asks for the Windows user name, falls back to user_images if that fails, and formats display-context directory field 0x628 plus a separator plus that name into the game's output-directory buffer. It does not create the directory here. This sprintf has no explicit capacity check against the combined prefix/name.

Movie Output Rate defaults to ten without immediate writeback. SaveSettings writes the output directory and output rate only when their individual dirty fields are nonzero, then clears each dirty flag regardless of whether the void registry writer actually succeeded. A failed write can therefore lose that pending-dirty indication.

## 5. Other scalar preferences

textlines, textscroll and mousespeed each default to integer ten without writeback. gamespeed defaults to ten or truncates the loaded value to an unsigned short; effectiveGameSpeed is then copied from speedCtrl. This function does not negotiate a new multiplayer speed or perform a simulation tick. clock defaults to a cleared one-bit UI flag.

NumSkirmishPlayers defaults to four. The successful-read branch tests whether the value is greater than one and at most ten, but both outcomes assign the original value. It therefore does not clamp invalid values. Later loops use numSkirmishPlayers against an Options layout containing ten player records; oversized positive input can drive accesses beyond that array, while negative input can skip the ordinary loops. The redundant condition must not be described as effective validation.

PlayMovie defaults to one or retains the loaded integer. Frontend movie state later interprets it. AllMissions, NumSkirmishPlayers and track settings have separate explicit save helpers rather than relying on every field being written by SaveSettings.

## 6. Mission and skirmish rule defaults

SingleCommanderDeath, SingleMapping, SingleLineOfSight and SingleLOSType default to one and are written back when absent. The corresponding MultiCommanderDeath, MultiMapping, MultiLineOfSight and MultiLOSType keys use the same default/writeback policy in separate game fields. SkirmishCommanderDeath, SkirmishMapping, SkirmishLineOfSight and SkirmishLOSType similarly initialize Options fields to one and write missing defaults.

SkirmishDifficulty defaults to one with writeback; a successful value keeps only its low 16 bits. SkirmishLocation defaults to one with writeback and supplies Options.fixedLocations. The values are not fully interpreted as victory, mapping or LOS algorithms here; those consumers remain [mission loading](map-map-load.md), [sight](map-line-of-sight.md) and victory/setup systems.

SkirmishMap is read as up to 256 bytes. If missing, the loader changes mission type to two, refreshes the map list, copies the current mission name, changes mission type to zero and writes that name. It does not save/restore an arbitrary prior type. The bounded copy does not force a terminator for an exactly full-length source, and availability of a mission name depends on the map interface. This fallback is a map-list side effect, not a constant built-in map choice.

## 7. Per-player skirmish configuration

For each index below numSkirmishPlayers, the loader reads the following values under the Skirmish child key. Missing player values are assigned but not written back immediately:

| Key pattern | Field | Missing value for player i |
| --- | --- | --- |
| Player%dController | controller | 0 |
| Player%dSide | side | i modulo two |
| Player%dColor | color | i |
| Player%dAllyGroup | allyGroup | 5 |
| Player%dMetal | starting metal | 1000 |
| Player%dEnergy | starting energy | 1000 |

They are stored as integers without per-field range validation here. The loader does not allocate a player, create a commander, change alliances live or credit an active economy account. Later skirmish setup consumes the Options records. The unvalidated player count affects both loading and saving these arrays.

## 8. SaveSettings coverage and asymmetries

SaveSettings writes the main display/interface/side/scroll/chat/visual fields, all four single and multiplayer rule fields, difficulty/gamma/input switch, password/nickname/game name, sound mode/options/volumes, text/mouse/game-speed/clock settings, skirmish rule/difficulty/location/map fields, each configured player's six fields and PlayMovie. When RestoreVolume is enabled it queries the actual current wave/auxiliary levels for persistence rather than simply writing the startup snapshots. Query failure values such as -1 are not locally filtered.

It writes Options.fixedLocations under both FixedLocations and SkirmishLocation, while LoadSettings uses SkirmishLocation for that field and does not read FixedLocations. Output directory/rate are conditional dirty writes, as described above. The musicmode write explicitly keeps bit zero. It does not save the auxiliary DisplaymodeDepth/Games tests, UI forced bits, effectiveGameSpeed, AllMissions or NumSkirmishPlayers in this body. The latter two are handled by [settings_campaign.cpp](../../../src/game/settings_campaign.cpp).

The writers discard per-key failures. There is no atomic snapshot or compare-and-swap with other processes. Loading many options does not mean all were successfully persisted, and saving while arrays contain invalid counts inherits those accesses rather than validating a serialized schema.

## 9. Track bytes and generic game-key adapters

SaveTrackSettings (`0x42f910`) writes exactly ten supplied bytes as DWORD values track0 through track9. It neither reads tracks nor uses the CD's discovered track count to choose a length. The caller determines the starting pointer and correspondence to the CD table. No bounds argument, range check or combined return result is supplied.

WriteGameRegistryValue (`0x42f960`) forwards key, buffer and byte count to WriteRegistryBinary under Total Annihilation. ReadGameRegistryValue (`0x42f980`) supplies the same subkey to the generic read wrapper with the caller's in/out size. Their generic values are raw registry bytes, not HapiBank accounts or a full match-save object.

## 10. Mission use-only unit restrictions

ApplyUseOnlyUnits (`0x431740`) obtains the mission's name slot six. A null path or failed TDF load returns before touching definitions. On success it makes the shared unit-definition allocation writable and clears availability bit 0x800000 on entries one through unitDefCount-1. Index zero is excluded from that clearing loop.

It then enumerates the TDF's top-level record names. For each, it case-insensitively scans unit definitions from index zero, sets availability on the first matching unitname and stops that inner scan. Unknown names do nothing. It ignores record contents, duplicates have no additional effect and an empty successfully loaded list can clear all nonzero definitions' availability. Finally it restores read-only protection. This updates the same [unit availability flag](units-unit-types.md) used by definition lookup/build consumers; it does not destroy existing units or rewrite factory queues here.

The operation does not intersect the selected names with a separately retained previous availability mask: it clears, then sets matches. Protection/allocation calls have no local error recovery and the scans assume valid counts/strings. Definition loading and mission sequencing decide when it is appropriate to apply this restriction.

## 11. Coverage and persistence limits

All six address-owned functions, their inline load policies, complete key families, write asymmetries, immediate Sound side effects and restriction loop were inspected. Underlying registry behavior is documented separately, while actual menu controls, game-speed negotiation and saved-match serialization remain their own modules.

No settings were changed in Windows during this documentation task. The reference uses existing files only, with no builds, installations, game execution or copied source listings. Numeric defaults reflect this source baseline and do not assert universal values for modified assets or a preexisting user's registry.
