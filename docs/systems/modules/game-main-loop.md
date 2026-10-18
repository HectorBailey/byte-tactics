# Battle-frame dispatch, setup transitions and starting units

## Scope and execution boundary

The `game/main_loop` group owns ten entries from `0x496790` to `0x497180`, all in [main_loop.cpp](../../../src/game/main_loop.cpp). It runs one battle-frame pass, changes callbacks while entering menus/loading, applies mission options, and supplies starting-resource and commander helpers. Its full scope extends beyond the first battle-loop function.

An application frame, a battle pass and a simulation tick are different operations. The application dispatcher invokes the current game callback. MainLoopTick can run zero or several simulation ticks and then draw one battle frame. The actual pacing calculation and per-tick subsystem loop are in [game_dialogs.cpp](../../../src/ingame/game_dialogs.cpp), described in [time and simulation](../02-time-simulation.md). This reference covers how the owning frame calls and gates them, without counting that unfinished neighboring module as completed.

## Profiling snapshot and simulation branches

MainLoopTick (`0x496790`) first sums nine profiling accumulators, copies each into its display value and resets it. It clamps the summed total to at least one and obtains a new millisecond timestamp. The snapshot represents work charged before this pass; it is not a count of simulation steps or a measurement solely of the current rendering operation.

The flag at game offset `+0x2a44`, bit 0, selects the network-form branch used here. When set, the frame always calls UpdateFramePacing. If simulation steps are pending it calls RunGameSteps with argument one, charges elapsed time to profiling bucket 8 and calls three continuation hooks. If loading flag bit 2 at `+0x38d75` is set, it calls AssignStartPositions and clears that bit only when assignment reports success, then charges bucket 0. Three further hooks follow that pending-step path.

If no steps are pending and pause is set, the network-form branch optionally flushes queued packet-manager traffic, handles incoming packets and sends a probe when current timer ticks strictly exceed the next-probe deadline. It advances that deadline by 60 timer ticks and charges bucket 0. If no steps are pending and pause is clear, this particular fallback path does not service packets; ordinary networking can still be serviced by other application/subsystem paths.

When the network-form flag is clear, simulation runs only if interface flag bit 0 at `+0x37ebe` is clear and pause is clear. It then updates pacing and calls RunGameSteps with argument zero if steps are pending, charging bucket 8. Thus the local paused path skips pacing entirely in this function, while the network paused path still calls the pacer and can receive traffic.

Each profile charge measures current milliseconds minus the previous profile timestamp, adds that span to its chosen accumulator and updates the timestamp. Bucket labels and simulation clock units must not be inferred from the mere integer values 0 and 8. Subsystem timing also occurs inside called routines.

## Input, selection refresh, drawing and capture

After simulation selection, the frame calls HandleGameKey and UpdateEdgeScroll only when interface flag bit 0 is clear. It always collects visible unit IDs. It then examines the interface word and a separate three-bit field at `+0x2bee` to choose how a pending selection refresh is handled.

If the word contains `0x800`, any bits in `0x65`, or the separate field is nonzero, a set bit 4 causes unitIndex to be cleared. Otherwise a set bit 4 is cleared and RefreshSelectionOrders is called. This makes selection refresh conditional on the surrounding interface modes, not just whether the simulation advanced.

DrawBattleFrame is called with both arguments one regardless of which simulation branch ran. The frame charges bucket 8 afterwards and calls another hook. Rendering therefore continues across a no-step or paused pass, subject to the surrounding application dispatch. Drawing uses the maintained visible-unit state; it does not itself advance every unit.

Movie capture runs when its capture index is positive and nextMovieFrameTick is no greater than gameTick. It calls SaveScreenshot with the configured text buffer and prefix `FRAM`, advances the capture deadline by integer division of 30 by movieOutputRate, and resets lastSimBudgetTick to current timer ticks. There is no catch-up loop here: a late pass captures at most one image. A zero output rate would divide by zero, while a rate above 30 gives a zero deadline increment; this function relies on configuration preconditions rather than clamping them.

Resetting the simulation-budget timestamp prevents the capture interval from simply contributing as ordinary elapsed budget on the next measurement. The capture deadline itself uses simulation ticks, while profiling uses milliseconds and budget reset uses GetTicks. [Screenshot implementation](../../../src/ingame/screenshots.cpp) owns file encoding and application-level capture keys.

## Entering and returning to menus

InitFrame (`0x496a60`) selects cursor mode `0x13` if needed and updates its animation, shows the software cursor, clears three application bits including the network-form bit, sets mission type zero, clears two further flags at `+0x3923b`, clears queued keys and installs state 2 with MenuFrame as callback. It also installs LeaveNetGameCallback as the close handler with argument zero.

ReturnToMainMenuFrame (`0x496b10`) first calls EnterMainMenuState, then performs the same relevant application/mission/flag/key resets. It enables GUI key commands and installs the same state, callback and close handler. These functions establish callback state; they are not the complete resource teardown sequence. [Game-state lifecycle](../../../src/game/game_state.cpp) owns that surrounding cleanup.

MenuFrame (`0x496bb0`) refreshes unit information and runs the frontend state machine before testing these branches in order:

| Condition | Installed state/callback |
| --- | --- |
| Application bit 2 set and mission game type 1 | State 4, CampaignSetupFrame |
| Application bit 2 set and game type 2 | State 5, LoadingScreenFrame |
| Frontend menu state `0x11` and application bit 2 set | State 3, PreBattleFrame |
| Frontend menu state `0x10`, bit 4 at `+0x2b4c` set, substate `0x12` or `0x13` | State 3, PreBattleFrame |

Every transition hides the software cursor and installs the leave-network close handler. After testing transitions, MenuFrame still hides the cursor, blits menu layers and shows it again. Changing the callback does not recursively invoke the new callback in the same body. Broad frontend numeric states retain their observed values here rather than being given unsupported menu names.

## Prebattle and campaign setup

PreBattleFrame (`0x496ce0`) uses the local player's info. If flags byte `+0x97` has bit 0 set, it broadcasts the one-byte value 8 using that player's ID. Otherwise, when bit 4 at `+0x2b4c` is set and the heartbeat deadline is strictly earlier than GetTicks, it moves the deadline 60 ticks ahead and sends a network heartbeat. Packet transport and reception are separate modules.

Every path then installs state 5, LoadingScreenFrame, and the same close handler. Sending a packet or waiting for the heartbeat deadline is not a gate that leaves this callback installed for another frame. There is no acknowledgement wait in this function.

CampaignSetupFrame (`0x496db0`) sets the player count to two, sets up slot zero with kind one and slot one with kind two, writes one to the referenced subobject's byte at offset `0x96`, and installs the loading callback/state. The local source's overlapping views are provisional; the direct state writes are established without inventing an additional campaign-player allocation.

## Mission-option and resource helpers

ApplyMissionOptionFlags (`0x496e10`) copies the settings block's first integer into battleCommanderDeath. It assigns the three remaining integers into single-bit fields at `+0x14281`: input `+0xc` to bit 2, `+0x4` to bit 0, and `+0x8` to bit 1. Assigning integers into these bitfields retains the low bit, not a general nonzero-to-true conversion. Callers in [game_load.cpp](../../../src/game/game_load.cpp) select single-player or shared options before applying this mapping.

SetStartingStorageBonus (`0x496e90`) marks the player's start flag and stores two arguments as floats, each with a minimum of 200. The width argument goes to player `+0xdc`; height goes to `+0xe0`. These source-local names are also shown as size1/size2 in other views. This function changes starting storage/resource state, not camera or model dimensions. Its other callers include [player initialization](../../../src/game/players_464290.cpp).

InitStartingResourcesFromSkirmish (`0x497080`) walks ten slots. A qualifying player is active, has controller type 1, 2 or 3, and has index other than 10. It maintains separate running maxima of the qualifying options' metal and energy values. Inside that same loop, immediately after updating maxima, it marks the current player and stores the clamped running energy maximum at `+0xdc` and metal maximum at `+0xe0`.

This is a prefix maximum, not a two-pass global maximum assignment. If an earlier participating player requests 300 and a later one requests 1000, the earlier player is not revisited after the maximum grows. Only later qualifying slots see that larger maximum. Inactive/nonparticipating slots neither contribute nor receive updates. The order dependence follows the loop directly; the code does not establish that it was intended as a fairness rule.

## Commander creation and camera positioning

SpawnCommanderAtStartPos (`0x496ee0`) takes a team index and a start-position index. It copies the team's selected side and color from option bytes into PlayerInfo, marks the player started and initializes its two starting-size floats from the option integers with a minimum of 200. The energy/size2 option maps to `+0xdc`, and metal/size1 to `+0xe0` in this view.

It asks the mission for the specified start position as three 16.16 fixed-point coordinates. On success it looks up the starting unit type by the selected side's commander-name entry and calls CreateUnit with team, type, position and trailing arguments 1, 1, 0. The local name-table view begins at `+0x37f5f`, which corresponds to the commander field within the packed side definitions, not the side's display name. [Side definitions](game-side-data.md) establish that layout and [unit lifecycle](units-units.md) explains creation.

A missing start position formats a fatal error including its number. No bounds check for arbitrary team/side indices and no separate invalid-unit-ID check precedes creation in this helper. If FatalError unexpectedly returned, the following local-camera calculation could use an uninitialized position; ordinary execution relies on the fatal path ending the operation.

For the local team, the camera is positioned after creation using the signed whole halves of X and Z, minus half the current view dimensions, with final argument zero. This drops fractional coordinate parts and centers the view approximately on the start. Remote teams do not reposition the local camera. Unit terrain placement, collisions, script initialization and replication are responsibilities of CreateUnit and its callers, not repeated here.

## Completion and interaction limits

All ten owned entries are covered, including setup helpers outside the main tick. The observed frame order explains why the interface can remain responsive while simulation is paused, why start assignment is retried until success, why capture can affect pacing and why menu transitions take effect through the callback dispatcher.

The exact simulation phase order and adaptive rate calculation remain in the separate scheduling module; frontend widgets, network packet interpretation, battle drawing and match loading have their own source ownership. No timer measurement, game execution or screenshot generation was performed for this reference.
