# Match loading, worker handoff, synchronization and cursor coordinates

## Scope and phases

The `game/game_load` group owns seven entries from `0x497180` to `0x498f70`. Six are in [game_load.cpp](../../../src/game/game_load.cpp); the exception boundary is in [game_load_497c70.cpp](../../../src/game/game_load_497c70.cpp). Responsibilities include match construction, the loading-screen callback, synchronization display and three screen/world coordinate helpers. Loading battle assets themselves is delegated to [game-state lifecycle](../../../src/game/game_state.cpp), while persistent record interpretation is delegated to save/player/unit modules.

Loading has two cooperating execution paths. The main application thread displays progress, services network traffic and attempts start assignment. A worker invokes LoadMatch, builds battle state and finally sets a loaded bit. Progress percentages and completion flags are separate state; six bars reaching 100 are not the condition that transfers the application into battle.

The volatile progress bytes begin at game `+0x38d6f`; six stage bytes are followed immediately by a loading-flags word at `+0x38d75`. Its relevant flags are bit 0 started, bit 1 loaded, bit 2 start-assignment requested and bit 3 synchronized. The source uses volatile views for cross-thread observations, but supplies no general mutex or thread join in this module. [StartThread](util-registry.md) wraps CRT thread creation and returns a success boolean rather than a retained join handle.

## Main-thread initialization

LoadingScreenFrame (`0x497f40`) runs first-time setup when started is clear. It closes existing screens, disables GUI key commands, selects cursor mode `0x14` with the hourglass animation if needed, sets COMIX as current font and applies all 256 game-palette colors. It saves settings except for mission game type 2, closes remaining screens and blanks the screen.

The loading presentation uses 640 by 480. If the actual display differs, the function frees the previous offscreen allocation, clears its restore binding, restores the screen, resizes the window, changes resolution and allocates/binds a replacement offscreen surface. It loads language-resolved `palettes/guipal.PAL`, remaps GUI colors to the game palette and frees the temporary palette. Null palette and allocation results are not separately guarded at each call site.

It resets the simulation-budget timestamp, pending steps, game tick and speed hysteresis, and sets endGameCountdown to -1. It then prepares the future battle dimensions from configured display width/height. The battle view begins at X 128, Y 32, ends at width minus one and height minus 33, and derives inclusive view dimensions. Thus the physical loading display can remain 640 by 480 while game fields already describe the intended battle view.

It loads the `loadgame2bg` cached picture and clears the loading slot block. Each active controller-type-1/2 slot is recorded as in-game, and an additional per-slot flag records whether PlayerInfo has mask `0x40`. These arrays support the subsequent synchronization protocol; recording them is distinct from declaring everyone synchronized.

The callback starts LoadThreadMain with zero stack-size/parameter arguments. Failure invokes FatalError with the loading-thread message. It clears kill/loss flash arrays, loading-bar alpha and previous-percentage arrays, sets started and invokes OnlineUnload. Because thread creation precedes the final main-thread setup writes, this is not a suspended-thread initialization protocol; ordinary source ordering is the available evidence.

## Worker entry and mission settings

LoadThreadMain (`0x497c70`) calls LoadMatch inside a structured exception boundary and passes the label `Load Thread` to ExceptionFilter. It has no rollback, resume or substitute-loaded-state path in its handler. Detailed exception reporting belongs to debug support, not this module.

LoadMatch (`0x497180`) seeds the game RNG with the sum of the performance counter's low/high halves, seeds CRT rand from calendar time and resets game tick. These are distinct RNG states. Mission game type selects settings as follows:

| Type | Initial policy |
| --- | --- |
| 1 | Sets noncampaign state zero, applies singleSettings and applies use-only-unit restrictions |
| 2 | Copies the INI unit limit to effective maxUnits, sets noncampaign state one and applies the option block's mission flags |
| 3 | Copies the INI limit, sets noncampaign state one, clears pause bit 0 and obtains mission/settings from the host-player path |

The flag mapping used by types 1/2 is the [mission-option helper](game-main-loop.md): commander-death integer plus three single-bit view/map assignments. Effective multiplayer maxUnits can subsequently come from host data; the INI preference is not the final authority for all match types.

For type 3 the worker locates the host slot. If the local keepalive flags contain mask 2, it repeatedly flushes queued packets when enabled, receives packets, refreshes the host slot and sleeps 50 milliseconds. It leaves the loop only when the host slot is not sentinel 10 and local PlayerInfo has a color other than 255 plus a nonzero byte at `+0x8f`, then sleeps another 50 milliseconds. There is no timeout/cancellation check in this loop. Without that keepalive condition, this local wait is skipped; later host accesses still rely on valid caller/session state.

The mission is loaded using host PlayerInfo. If a subsequent host lookup returns sentinel 10, the switch branch stops. Otherwise host fields supply noncampaign/cheating state, commander-death mode, LOS/mapping flags and maxUnits. This does not make every host lookup safe automatically: some accesses occur before or outside the sentinel check.

## Pending saves and battle foundations

If pendingSaveStore exists, the worker selects its summary account. It tests presence of `BetweenMissions`, not a boolean value stored in that item. Without that item, it restores player controllers. In type 2 it also scans ten option slots for kinds 1/2, computes one past the highest qualifying slot, raises numSkirmishPlayers if necessary and reapplies slots to game players. This is a highest-slot extent calculation, not just a count of occupied slots.

Next it calls LoadBattleAssets. That separate lifecycle function loads textures, map, definitions and battle services in its established order. Calling it is not equivalent to restoring live unit records: restoration and initial unit placement occur later.

For type 3 the worker sets start-assignment-requested and waits in 50-millisecond sleeps until synchronized is set. It does not service packets inside this second wait. The main loading callback is responsible for processing traffic and calling AssignStartPositions while the worker is waiting. No deadline is imposed here, so loss of progress in the main-thread/network side can leave the worker waiting indefinitely.

Once synchronized, the worker refreshes host mapping/LOS/commander options and walks ten players. It considers active controller types 1/2, seeds a fallback position randomly inside an 80-world-unit border, skips players whose info has mask `0x40`, and asks the mission for the assigned start position. The return from that lookup is ignored here, so the random fallback can survive a lookup failure, unlike the separate fatal-checking commander helper.

For each such player it resolves the side's commander name and creates a unit using the player's kind/index field as the team argument, with trailing creation arguments 1, 1, 0. Host energy and metal settings multiplied by 100 establish starting storage floats with a minimum of 200. The local controller's position is retained for camera centering. If the local info has bit 6 set, mapping and LOS bits are cleared and the camera uses half-view coordinates instead; otherwise it centers on the retained start. If no qualifying local start initialized that retained value, this branch has no independent fallback initialization. It finally reports game event 6.

## Skirmish starts, mission units and restored state

A fresh type-2 match with no pending save chooses one of two placement policies. Fixed locations walk active controller types 1/2/3 whose kind is not sentinel 10 and spawn each commander at its own slot number. Nonfixed locations collect those qualifying slot numbers into an array, optionally shuffle that populated prefix, and assign its entries back to qualifying players in ascending slot order.

The shuffle runs unconditionally for more than two players. For zero, one or two it first uses a CRT-rand half-range decision; calling shuffle still does not guarantee the order changes. This path shuffles qualifying slot numbers, which can be sparse. It should not be conflated with the networking module's separate start-assignment candidate generation. It initializes player resources after fresh skirmish spawning.

After these branches the worker calls RecalculateLineOfSight with argument one. Then it selects the live-state path:

| Pending store / mission | Behavior |
| --- | --- |
| Pending store, summary lacks BetweenMissions | LoadSavedGameState, then continue to common tail |
| Pending store, summary contains BetweenMissions | CreateMissionUnits and center on the mission start |
| No pending store, type 1 | CreateMissionUnits and center on the mission start |
| No pending store, other type | Continue to common tail without mission-unit creation here |

The distinction explains why a between-mission save starts new mission objects while an ordinary save restores serialized state. Earlier type-3 setup is not globally skipped solely because a pending store exists; the source's actual ordering above should be preserved when investigating unusual combinations. Full record reconstruction is covered by [save orchestration](game-settings-campaign.md), [player persistence](game-players.md) and [unit persistence](units-unit-save.md).

## Common worker tail and completion signal

The tail clears the cached picture, chooses a side name-prefix using playerIndex and formats the main HUD layout as that prefix followed by `MAIN2.GUI`. The packed side-name view begins at the nameprefix field, while commander lookup adds four bytes to reach the following commander field. These offsets match [side definitions](game-side-data.md), rather than two contradictory tables of side display names.

It loads the GUI layer with flag `0x20`, installs HandleMain2LayoutEvent and the game owner pointer, sets the local PlayerInfo started mask `0x10`, broadcasts player info, updates network game info and players, and initializes player resources. The fresh-skirmish path has already called that resource initializer once; this tail call is still present.

If a pending bank exists, the tail closes it, deletes its object and clears pendingSaveStore. [HapiBank](util-hapibank.md) explains why this explicit close/delete sequencing matters. The worker rebuilds AI feature cells and finally sets loaded, flag value 2. There is no loaded signal earlier merely because a stage bar completed.

## Main-thread synchronization, drawing and handoff

While loaded remains clear, each loading-screen pass probes active type-1/2 players, optionally flushes queued traffic, and receives packets. If start-assignment-requested is set, it calls AssignStartPositions. Success clears that request, sets synchronized and disables guaranteed-packet mode through HAPINET_guaranteepackets with argument zero. It flushes again when packet management is enabled.

The callback then binds the offscreen surface and tries to lock the screen. Successful locking draws the cached background and, for non-type-1 missions, a centered translated map label. Mission names are bounded to 99 characters plus NUL and lowercased before translation for non-English preferred languages. The final combined label uses a fixed 128-byte buffer without a bounded formatter.

It obtains frame zero of LIGHTBAR and clears its frame hotspot offsets. Six bars use these stages:

| Byte index | Label | Y coordinate |
| --- | --- | ---: |
| 0 | Textures | 135 |
| 1 | Terrain | 177 |
| 2 | Units | 218 |
| 3 | Animation | 262 |
| 4 | 3D Data | 304 |
| 5 | Explosions | 347 |

Each uses label X 90, fill-left X 205 and fill-right equal to 205 plus integer percent times 7 divided by 2. The fill is 20 pixels tall. Colors switch at completion using GUI palette entries 12 and 10. A newly observed value 100 sets that bar's flash alpha to 30; alphas are decremented by two on qualifying timer changes. Values are bytes without local clamping, so an invalid percentage can exceed the expected width. LIGHTBAR itself is drawn as a frame; the 3D-data section also contains an extra explicit frame draw in this body.

For type 3 the callback draws synchronization status and sends load progress. [Network game code](../../../src/network/net_game.cpp) averages the six stage bytes into a two-byte type-`0x2a` packet and records the average for qualifying players. That average is presentation/protocol progress, not the loaded flag. Screen unlock/flip occur after drawing. Every still-loading pass sleeps 200 milliseconds, including a lock failure; the worker does not share that drawing sleep.

DrawSyncStatus (`0x497ce0`) first renders/marks/blits its GUI layer. When synchronized it draws the translated completion text. Otherwise it counts active controller types 1/2/3 excluding kind 10, and counts ready players that additionally have percent exactly 100 and a nonzero loaded-array entry. It divides 620 pixels among qualifying players, draws bars at Y 420 through 435, labels them with player names, and displays the ready count with singular/plural text at Y 400.

No zero-player guard precedes the width division, and individual percentage values are not clamped. It uses a temporary local text buffer inside the unsynchronized branch and retains its pointer for the final draw outside that block; the traced stack storage remains the implementation evidence, not a safe general-purpose lifetime pattern to reproduce.

When loaded is observed, LoadingScreenFrame takes the handoff path before further loading drawing: stops sounds, blanks the display, frees the loading picture, restores configured battle resolution/offscreen storage if necessary, draws light bars, calls MainLoopTick once and shows the cursor. It then installs state 6 with BattleFrame and the battle quit-prompt close handler, clears another GUI state field and zeroes eight bytes covering progress plus loading flags. Finally it selects CD music category zero and starts the next track if none is playing. The first direct MainLoopTick therefore happens before the normal battle callback is installed. No explicit worker join occurs here.

## Screen and world coordinate helpers

OffsetWorldPosFromView (`0x498cd0`) adds the view origin minus the left/top battle borders, 128 and 32, to a supplied point and forwards to ClampWorldPosToTerrain. Its local integer declaration for the third argument differs from the actual output-pointer declaration elsewhere. The called [LOS-owned terrain projection](../../../src/map/line_of_sight.cpp) supplies the real coordinate conversion; this wrapper is not an independent height sampler.

MinimapCursorToWorldPos (`0x498d00`) takes the stored screen position, subtracts minimap gadget X/Y, scales by map pixel width/height divided by gadget width/height, clears the output position and stores the integer results into its X/Z whole halves. It obtains Y from GetGroundHeight. Fractions remain zero. There is no gadget-size-zero guard or independent clamping in this helper.

UpdateCursorWorldPos (`0x498da0`) accepts a snapshot of the screen point. If inside the minimap hit rectangle and input bit 3 is clear, it scales offsets directly by world width/height over minimap dimensions, sets input bit 0 and clears bit 1. Otherwise it clamps the point to the visible battle rectangle, adds camera scroll offsets, clears bit 0 and sets bit 1 according to whether the original point lay inside that rectangle. Bit 2 becomes bit 0 OR bit 1, not their conjunction.

It passes the resulting coordinates to ClampWorldPosToTerrain, stores the projected world position, derives cell X/Z using unsigned right shifts by 20 into short fields, then obtains that cell and stores its feature ID. The called converter writes 16.16 world coordinates. Shifting by 20 therefore converts them into 16-world-unit cells; a nearby source comment calling them 20.12 fixed point is misleading. The projection also accounts for terrain height and sea level, as described in [line of sight and world picking](map-line-of-sight.md).

This cursor state feeds placement, interaction and command selection. It records where the current screen gesture points and whether it came from a valid display region. It does not itself authorize a construction placement, select an attackable target or issue an order. Those decisions belong to [battle placement/input](ingame-build-placement.md) and [command dispatch](orders-order-dispatch.md).

## Completion and investigation limits

All seven owned entries are covered, including worker exception handling, setup/save branches, thread/UI synchronization, presentation and coordinate conversion. Missing start records, invalid dimensions, absent GUI/GAF assets and stalled peers have different failure paths; a loading bar alone cannot distinguish them. This reference follows static source and makes no claim of runtime timing, successful network handshakes or save compatibility tests.
