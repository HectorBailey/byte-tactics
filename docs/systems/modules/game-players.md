# Player slots, updates, visibility and persistence

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `game/players`, from `0x464290` through the boundary before `0x4665d0`. Sources: [player lifecycle and update](../../../src/game/players_464290.cpp), [footprint visibility](../../../src/game/players_4658e0.cpp), [unit visibility](../../../src/game/players_465ac0.cpp), and [player save/load](../../../src/game/players_466050.cpp). Shared layouts are [Player](../../../src/network/player.h) and [PlayerInfo](../../../src/network/player_info.h).

The module connects controller setup, economy, squads/AI, information state, mission conditions and persistent player data. A player record's controller type, active flag, slot/team index, network identity, side, logo and watching flags are separate concepts. Their names are not consistently settled in all private source views; this writeup follows offsets and bodies where comments disagree.

## 1. Player and player-info state

The ordinary gameplay loops process ten 0x14b-byte player records. Each holds an active flag, identity/timing fields, PlayerInfo pointer, short/full names, inclusive unit range and IDs, controller type, AI/squad pointers, explored-grid storage, resources/statistics, account pointer, update deadlines, five eleven-entry relationship arrays, and slot/start/rank flags.

PlayerInfo contains lobby/map identity, side/color, sharing flags, watching/mapping/LOS settings, initial resources, maximum units, version and map checksum. This module reads selected values; the networking modules own lobby exchange and consistency checks.

The prevalent controller roles are 0 unused, 1 local human, 2 local computer and 3 remotely controlled. Individual branches still test activity and other flags in addition to type. For example, a nonzero type alone does not qualify a player for every economy or AI action.

## 2. Resetting and configuring slots

`ResetPlayerSlots`, `0x4644d0`, preserves each PlayerInfo pointer while zeroing its player record. It resets local/view player to zero, clears the identity array and rebuilds default names, self-relationship entries and info defaults. Controller and active fields become zero, color follows the index, side is zero, unit ranges/counts clear, slot byte becomes 0xff, team-style byte becomes 10 and alliance mode becomes 5. Chat read/write indices reset.

Both reset loops use indices 0 through 10 inclusive, although ordinary gameplay loops and shared declarations describe ten slots. `FreePlayers` likewise iterates through player-array-plus-ten inclusively. This extra-record access is present in the shown source and must not be silently rewritten in documentation as a ten-iteration loop. Establishing the complete backing allocation/sentinel intent requires the surrounding game layout; it is not evidence of an eleventh ordinary combat player.

`SetupPlayerSlot`, `0x464290`, clears all five relationship arrays, stores controller type and, except for type 3, updates PlayerInfo kind. It enables two self entries, clears the squad-manager pointer, assigns slot/start/rank defaults, marks active, resets lobby synchronization and rejection state, and clears an info flag. Active type-1/type-2 roles record a display-context-derived memory value.

Campaign/skirmish naming uses Player for type 1 and Arm/Core according to side for type 2, with the skirmish strings translated. Full name copies the resulting short name. Multiplayer naming is not assigned by these naming branches. A private field called index at +4 is assigned by setup while other views label that offset identity; later network identity handling must be traced rather than inferred from this provisional name alone.

## 3. Allocating per-player runtime services

`InitPlayerSlot`, `0x464700`, sets update, win/lose and display timestamps to the current game tick. It clears current balances, income/usage/capacity, lifetime produced/consumed/wasted totals, share thresholds and kill/loss statistics. It creates an extra UnitResources account if absent and resets it for the player's slot/team value. The starting-storage flag clears.

The explored grid uses half the terrain width and height. Its allocation size rounds their product up to a multiple of eight bytes, then zeroes the buffer. It is a byte grid, not one bit per explored cell despite the byte-count rounding. Old storage is released before allocation.

It constructs the ten [squads](units-squads.md). Unless the record is both active and type 3, it allocates SquadManager and creates player AI services. This condition is broader than only type 2. Callers and the later AI updater determine which roles actually execute strategic decisions.

`InitPlayers`, `0x464990`, invokes initialization for each nonzero-type slot, then loads default AI command text. `LoadDefaultAIScript`, `0x4648e0`, tries mission path slot 7 and falls back to ai/default.txt. Successful text is executed through the command system and freed. It then parses downloadable AI weight scripts and reparses limits/weights for active type-2 players. Missing both text files does not prevent the later per-AI passes in this body.

`RebuildAIFeatureCells`, `0x4649d0`, calls the AI feature-grid rebuild for slots with a squad-manager pointer. `FreePlayers`, `0x464a00`, clears/frees explored storage, frees squads, destroys per-player AI and manager timers/object, and releases the extra account allocation. Its inclusive extra iteration and legacy allocation/deallocation spellings are source facts; cleanup ordering with units must protect their player-owned dependencies.

## 4. Starting balances and economic observations

`InitPlayerResources`, `0x465e30`, skips starting-balance assignment while a pending save store exists. Otherwise campaign mode reads the mission's per-player start arrays and invokes starting-storage setup; skirmish reads slot options; multiplayer reads the host's info resource values multiplied by 100, falling back to the current slot when no host slot is found. It visits all ten slots rather than only active combatants.

The six accessors at `0x464ab0`, `0x464ac0`, `0x464ad0`, `0x464af0`, `0x464b00` and `0x464b10` return stored energy income, energy usage, their difference, metal income, metal usage and their difference. They do not settle pending unit requests or calculate fresh production on demand. [Economy accounting](../03-economy.md) explains the producers and settlement that populate those observations.

Two transfer functions have provisional names that are reversed relative to the resource fields they access:

| Entry point as currently named | Confirmed account channel |
| --- | --- |
| TransferMetal, 0x464b30 | Stored energy, SpendEnergy, destination energyMake |
| TransferEnergy, 0x464c60 | Stored metal, SpendMetal, destination metalMake |

Each rejects source/destination value 10, optionally clamps to the source's stored balance, ignores exact zero, and credits the destination extra account's production field. With the flag set it also spends from the source and sends the corresponding currently named share message with the float amount's raw bits. Without that flag it credits without local source spending, as appropriate to the separate received-event path. Exact packet interpretation belongs to sharing/network modules; these names alone are insufficient evidence for relabeling every channel globally.

An active type-2 destination scales the credit to 0.5 at difficulty 0 or 0.7 at difficulty 1, otherwise full credit. This body does not independently clamp negative amounts, bound every index beyond rejecting sentinel 10, test alliance, or clamp the credit to destination storage. Caller policy and later settlement remain necessary parts of a complete transfer.

## 5. Per-call work and slower player updates

`UpdatePlayers`, `0x464f80`, first runs pending pathfinder searches. It visits active type-1/type-2/type-3 records whose slot/team byte is not 10. Before the slower deadline gate, it ticks an existing squad manager, calls the AI updater, refreshes line of sight for live units in the player's inclusive range and draws radar units for the viewing player.

It then checks updateTime against gameTick. Not-yet-due players skip the rest of that pass. A due player advances its deadline by 30 ticks, preserving a scheduled sequence rather than resetting directly to current tick plus 30. Only one such increment happens for that player in one call; repeated calls can catch up after a delay.

The slower phase handles conditions/endgame, economy and viewing-player sensor/report work. Economy runs for active local controller roles when the player still has units or has never created any, the endgame bit is clear and no endgame countdown is active. A player whose last created force was eliminated therefore differs from a player still waiting for initial creation.

For the viewing player, the due phase updates sensor/radar/cloak state and radar mapping. In multiplayer it increments an economy-report counter and sends an economy snapshot every fourth eligible increment. This is separate from the per-call unit sight/radar work and from the simulation tick rate.

`RefreshOwnedUnitsLineOfSight`, `0x464da0`, separately refreshes every live unit in a supplied player range. It does not create or resize the player's grid.

## 6. Conditions and countdown semantics

For the local campaign player, victory is checked before defeat. A detected outcome initializes a negative shared countdown to 4; later qualifying observations decrement it until it falls below zero and set endgame/result flags. The shown blocks do not reset the countdown merely because a subsequent condition check is false.

Noncampaign local processing also checks defeat unless the relevant watching/exclusion info flag suppresses that path. Victory can enter the success countdown through the alternate path. Because these checks occur inside the player's due phase, countdown steps are observations on that schedule, not inherently rendered frames.

A separate multiplayer fallback after the player loop checks for zero combat players when commander-death mode differs from 2. It advances the same countdown outside the per-player deadline gate. That branch runs per UpdatePlayers call. A blanket explanation that all endgame countdowns mean a fixed number of seconds would therefore be incorrect.

Actual VictoryCondition and DefeatCondition algorithms remain in the victory module; this module schedules and interprets their results.

## 7. Commander replacement mode

When local defeat reaches its countdown threshold and commander-death mode equals 2, the updater attempts to create a replacement side commander. It resolves the commander definition from the local side and generates candidate positions inside a central rectangle with one-tenth-map margins.

It tries up to 9999 candidate draws. Each draw applies a three-by-three arrangement of CanPlaceUnitFootprint probes around the candidate using offsets derived from terrain width/height, requires no feature at the candidate, and, on lava worlds, requires sampled mean ground above sea level. The probe offsets and mode are implementation-specific; this is not a general nearest-valid-cell search.

After the attempt loop it calls completed CreateUnit at the last candidate, establishes starting storage from host settings, credits initial energy/metal through the new unit's account with applicable computer difficulty scaling, recalculates sight and focuses the commander. The shown code does not test the returned unit pointer before account access. Exhausting attempts or failing allocation/availability is not handled by an independent safe fallback here.

## 8. Watching after elimination

For other defeat paths, multiplayer can keep the local participant watching if no rejection reason is set and host configuration permits it or hosted AI remains. It marks the player info flag, clears two map flags, recalculates sight and broadcasts changed player info.

Without surviving hosted AI it can show a yes/no continue-watching dialog. With AI it explains that leaving would terminate hosted AI, subject to remaining combat-player checks. If watching is unavailable, it sets endgame and failure/result flags directly.

`ShowContinueWatchingDialog`, `0x464e70`, builds the same YESORNO.GUI-style choice layer, assigns translated Yes/No/title labels, installs its callback and renders it. `ContinueWatchingCallback`, `0x464de0`, ignores an unselected gadget, plays the button sound, and handles the named choices. Choice 1 broadcasts player info, clears the dialog-related flag and reports game event 4. Choice 2 sets the exit/endgame bit and clears the dialog-related flag; unrelated selections are cleared.

The dialog controls local continuation/view state. It does not revive destroyed units or reverse statistics.

## 9. Footprint visibility is a sampling test

`IsFootprintVisible`, `0x4658e0`, tests two positions: supplied cell origin and the diagonally displaced cell origin plus dx/dy. It converts cell coordinates to 16-unit world coordinates, supplies the height argument, and projects into the half-resolution information grid using X divided by 32 and (Z minus half Y) divided by 32.

With map flag 2 it uses the supplied player's explored-byte map. Otherwise it uses the shared visibility mask with the global viewing-player bit. Bounds failures return false. One passing sample returns true; the function does not require every cell in the footprint to pass, and two samples do not constitute exact polygon coverage.

## 10. Unit display visibility and underwater gating

`IsUnitVisibleToPlayer`, `0x465ac0`, first returns true for a unit owned by the supplied player object. That precedes its foreign-unit hiding tests. Foreign units with activation bit 4 return false. It builds a position from model minimum X/Z and maximum Y plus unit position. Without runtime bit 0x200, a model top below sea level returns false before map visibility samples.

It samples successive model-bound positions: initial top corner, opposite X corner, then the Z-extended lower corner, then the other X corner at that lower edge. The last point is not the original point, despite a provisional source comment suggesting otherwise.

Map flag 2 chooses explored-byte tests; otherwise the first three use IsPointVisible while the last expands the shared-mask test directly. These helpers project height into screen-map Z as above. At least one passing sample makes the unit visible. The function does not by itself decide radar/sonar contact, cloak detection, weapon selection or whether a projectile may hit it. The [naval reference](../../naval-systems.md) and later sensor/combat modules combine those separate permissions.

## 11. Player save/load

`SavePlayers`, `0x4662f0`, writes the Players account with Human Player and a 0x1c-byte GameTime box. For each nonzero-controller slot it writes a Player-number account with balances, lifetime production/use/waste, storage bonuses and enable flag, kills/losses, update/winlose/display timestamps, Controller, Logo, Side and an eleven-byte Alliances box.

`LoadPlayerControllers`, `0x465fb0`, is a separate preliminary pass: it reads Controller for each slot into both setup options and runtime type, setting zero when the player account is absent. `LoadPlayers`, `0x466050`, restores Human Player with default 10, copies it into viewing player, then requires exactly 0x1c bytes of clock state before continuing. A short clock read returns after the player-index assignments but before the per-slot resource restore.

Per-slot restoration reads the saved resource/statistical/timing values with zero defaults and restores side/color. Alliances are read only when the box size is exactly 11, and the self-alliance entry is forced to 1. Missing player accounts clear controller type. Controller values for present accounts are supplied by the separate controller pass rather than being reread here.

The listed save state does not include every player field or every relationship array. Commander-specific kill/loss counters, ranks, share thresholds, explored bytes, unit objects and AI internals are not all reconstructed by this serializer. Their ownership belongs to the surrounding save/load pipeline and other modules; persisting kills/losses here does not imply all scoreboard state is dumped wholesale.

## 12. Blink timing and completeness

`UpdateBlink`, `0x466580`, decrements a positive minimap blink counter; otherwise it resets the counter to 7 and toggles the blink flag. With this update called regularly, successive toggles are eight calls apart. Its frequency in wall time follows its caller, not an independent timer thread.

The module's setup, lifecycle, six resource accessors, two transfer channels, updater, visibility helpers, persistence and small interface helpers are covered. Remaining asset/configuration dependence includes AI text, initial side commander, condition definitions, host options and GUI resources. Their values determine a match's results without changing the branch contracts documented here.
