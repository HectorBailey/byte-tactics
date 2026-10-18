# Application and battle lifetime, wind, speed and interface state

## Scope and resource layers

The `game/game_state` group owns 22 entries in `0x490ac0..0x491ec0`, all in [game_state.cpp](../../../src/game/game_state.cpp). It coordinates application resources, battle resources, state callbacks, surfaces, wind updates, speed controls, saved CD categories, cursor hints, HUD-layer restoration and save-preview cleanup. Its global game pointer is shared with other systems, but its local struct is a partial view of a much larger object.

Three resource layers matter. Process/window/archive services are established by application main and display code. InitGame/ShutdownGame manage resources shared by menus and battles. LoadBattleAssets/ShutdownIngameSystems manage a particular battle. The [match-loading worker](game-game-load.md) calls battle setup before creating initial units or restoring live save records. No individual function here represents every stage at once.

## Mode callbacks

SetGameMode (`0x490b30`) stores the requested frontendState and selects its callback:

| Mode | Handler |
| --- | --- |
| 0 | InitFrame |
| 1 | ReturnToMainMenuFrame |
| 2 | MenuFrame |
| 3 | PreBattleFrame |
| 4 | CampaignSetupFrame |
| 5 | LoadingScreenFrame |
| 6 | BattleFrame |
| 7 | EndGameFrame |
| Other | Null handler |

Mode 6 installs HandleBattleQuitPrompt as the close handler; all other values install LeaveNetGameCallback, with argument zero. The setter does not call the new handler, allocate battle state or validate that the mode's prerequisites exist. Several transition bodies directly install the same fields rather than calling this setter. [Main-loop setup](game-main-loop.md) describes those transitions, while the application frame dispatcher decides when a retained handler runs.

## Offscreen surface ownership

CreateOffscreenSurface (`0x490ac0`) allocates an OFFSCREEN software surface at the current game screen dimensions and registers it as the restore surface. It does not release an older allocation first or check allocation success. FreeOffscreenSurface (`0x490b00`) frees and clears the stored pointer, removes the restore binding and calls RestoreScreen.

Force640x480Surfaces (`0x491a70`) always sets the game screen-dimension fields to 640 by 480. If the actual display already has that size, it performs no allocation or rebinding. Otherwise it frees the old offscreen allocation, clears its restore binding, restores the screen, changes the window size/resolution, allocates a new OFFSCREEN and binds it both for restoration and offscreen rendering. It does not restore the earlier resolution afterwards.

The [display](graphics-display.md) and [surface](graphics-surface.md) references describe those underlying operations. These wrappers establish ownership and sequencing; a surface pointer is not a COM display object to release through the same interface.

## Complete application initialization sequence

InitGame (`0x491200`) samples Windows memory status before and after initialization without branching on those results. Its substantive stages run in this order:

1. Read current screen dimensions, allocate/register OFFSCREEN, clear two endgame bits and restart state, set mission type zero, reset speed/carry under the rules below, copy the existing INI unit limit into effective/lobby limit fields, set chat filtering and clear selected UI bits. Disable page flipping.
2. Load general GAF resources, initialize sound, reset frontend state, initialize packet tables and register all order types. Load game fonts, the default palette and all sound definitions/samples.
3. Load alpha, shade and light tables and create gray/blue palette maps. Allocate the `0x22c` SkirmishInfo option block and load settings.
4. Load CDLISTS registry storage, enable CD audio from the low bit of musicMode, apply playback order, install ReopenCdAudio as callback, reopen the disc and select category zero. Apply brightness/volume settings.
5. Load side definitions and logos. Set brightness using 0.5 plus gamma multiplied by approximately 1/24. Bind the current GUI context, configure guis/anims/fonts search paths and the default COMIX font, initialize cursor frames, load commongui and hattfont12/hattfont11 GAF slots, and install the first GUI font slot as its language glyph source.
6. Set text-key color 254 and current COMIX font. Clear debug/capture/output-dirty fields, pending-save and movie pointers, install initial state zero/InitFrame and the leave-network close callback. Clear picture-cache state and enable quick-key clearing.
7. Read the UnitLimit INI preference, default 250, clamp it to 20 through 500, and store unitLimitIni.

The last UnitLimit read occurs after the earlier copies into maxUnits and maxUnitsLobby. Those earlier assignments use the value present at that moment; the function does not recopy the newly clamped preference into those fields at its end. Match setup later explicitly chooses effective limits for relevant mission types.

Application loading is not a staged transaction with automatic unwinding. Many delegated calls use fatal paths or unchecked allocations. Repeatedly invoking InitGame over live resources would overwrite pointers and registrations; it expects ordinary fresh lifecycle state. Resource loaders' own contracts, including required asset names, are detailed in [data files](game-data-files.md), [side data](game-side-data.md), [sound](sound-sound.md) and the graphics references.

ShutdownGame (`0x4916a0`) first copies per-track categories to the CD cache and writes the complete CDLISTS block. It then frees picture cache, GUI GAF slots 1 and 0, common GUI GAF, logos, side fonts, samples, game fonts, sound services and animation files. It frees/clears OFFSCREEN, removes the restore binding and restores the screen, clears the order-type table, frees/clears SkirmishInfo, frees unit-info storage, releases packet data and frees the OTA enumeration cache/mission object.

It does not call ShutdownIngameSystems itself. The surrounding exit path must first dispose of live battle objects when applicable. Some borrowed resource pointers are not cleared by their owning loaders' frees, so surviving nonzero fields do not prove the allocation remains usable.

## Battle initialization order

LoadBattleAssets (`0x4917d0`) first resets chat indices/eleven recipient bytes, selects order mode one, clears the build-type/hover IDs and selected input/interface/restart bits. It then performs these resource operations in order:

| Order | Stage |
| --- | --- |
| 1 | Load light-bar graphics, reset speech and load texture GAFs |
| 2 | Discover feature files, initialize unit categories and run the feature-enumeration hook |
| 3 | Create particle lists, allocate weapon storage and load weapon definitions |
| 4 | Load TNT map data and reset camera state |
| 5 | Load unit definitions and download menus, then allocate unit-instance memory |
| 6 | Resolve feature links, release the temporary feature-file list and build all passability maps |
| 7 | Initialize wind denominator/counter and call UpdateWind |
| 8 | Allocate transform/projected/assembly scratch buffers of `0x960`, `0x640` and `0xa0` bytes |
| 9 | Reset budget timestamp, game tick and carry, resetting requested/effective speed to ten for type 3 |
| 10 | Initialize meteors, radar and players; create pathfinder; initialize explosions and commands |

Finally it clears all ten players' unitsCreated counters, selected endgame/lobby state bits, the CD activity sample ring and write index, and score-panel slide position; it sets the net/CD sentinel byte to 255.

Ordering connects real dependencies. Textures exist before models are bound. Weapon definitions exist before unit definitions resolve weapon names. Unit memory and definitions exist before placement/restoration creates instances. Map/feature state exists before passability maps and pathfinding. Scratch buffers are process pointers with battle lifetime, not serialized positions.

The initial wind call occurs with counter zero. Its behavior depends on the game tick then present; the explicit gameTick reset comes later in this body. The ordinary loading worker already resets game tick, but this helper should not be described as independently guaranteeing a fresh wind sample on every call.

## Battle teardown and quitting

ShutdownIngameSystems (`0x491b60`) clears network-game flag value 4, stops CD audio and selects category four. It collects endgame statistics before destroying live state, runs a precleanup hook, and frees unit memory, particles, explosions, pathfinder, players, radar and map resources in that order.

It then frees/clears the three scratch buffers, frees download menus, unit definitions and weapon definitions, frees texture GAFs and their animation-pointer list, frees weapon-instance storage, movement classes and unit categories. It closes the network session only for mission type 3. Destroying model-owning definitions before textures is important because bound model entries borrow texture frames/sequences.

ShutdownIngameAndQuit (`0x491c60`) blanks the screen, removes local players, invokes this battle teardown and calls QuitApp with null. The separate QuitApp/application path owns further application/platform exit. This wrapper provides no reload, rollback or save prompt of its own.

## Wind sampling and consumers

UpdateWind (`0x490c40`) compares windCounter with gameTick. Only when the counter is strictly less does it generate a sample. It advances the counter by a CRT-rand-derived interval of 5 through 14, multiplied by 30 ticks, giving 150 through 420 ticks. It adds to the old counter rather than assigning current tick plus the interval; after a large jump it can sample on successive calls while catching up, but there is no inner catch-up loop.

Speed is windSpeedMin plus RandomInt of windSpeedMax minus windSpeedMin. The [game RNG helper](util-math.md) defines the range behavior; the upper speed bound is normally excluded. A nonzero speed selects a new 16-bit direction. Zero speed retains the existing direction. The actual body does not perform a separate direction jitter before that selection, despite its overview comment.

It calculates X and Z components as minus twice the fixed-point sine/cosine helpers at that direction and speed. It stores windFraction as speed divided by windSpeedDenominator, clamping only values above one. There is no denominator-zero guard or lower clamp here. It marks windEnabled one on a sampling call. On calls before the next deadline, it only sets windEnabled zero; it does not clear stored speed, fraction or vector components.

Consequently windEnabled is a new-sample notification gate, not an assertion that stored wind becomes physically zero between samples. [Wind-generator script callbacks](units-unit-motion.md) use it to decide whether to send direction/speed changes. Economic production and particles can consume the retained wind state under their own update rules. The generator's existence or economy rate is not created by this environment routine.

## Requested and effective game speed

ResetGameSpeed (`0x490da0`) sets requested/effective speed to ten only for mission type 3 and clears the pacing carry for every type. SetGameSpeed (`0x490df0`) clamps its input to 1 through 20, emits a translated message if requested speed changed, and assigns both requested and effective values. Ten is displayed as normal; other values are displayed as an offset from ten.

With broadcast enabled it sends three bytes: packet type `0x19`, subtype 1 and the clamped speed, using the local DirectPlay ID. It can broadcast even when the requested value is unchanged, since only the message is conditional on change. It does not test host authority itself. Packet reception and permission policy belong to networking/callers.

IncreaseGameSpeed (`0x490ee0`) and DecreaseGameSpeed (`0x490f10`) step within those bounds and request broadcast. The [frame pacer](../02-time-simulation.md) may subsequently reduce effective speed under lag/overload without changing the requested control speed. Setting requested speed explicitly resets that effective value immediately; it is not a promise of a fixed wall-clock tick rate.

## Persistent CD category cache and reopen behavior

LoadCdLists (`0x490f40`) reads 2720 bytes from registry value CDLISTS and zeroes the block only when the registry helper reports failure. It does not separately reject a successful short value. InitGame contains this same loading behavior inline. SaveCdLists (`0x490f80`) copies categories for tracks numbered one through GetTrackCount into the front record's category bytes and writes the entire block. ShutdownGame contains that behavior inline as well.

The cache has twenty records of `0x88` bytes. The disc serial lies at record offset `0x20`; track-category bytes begin at `0x24`. ReopenCdAudio (`0x490fe0`) remembers the current category, sends MCI stop/close/open commands, reapplies enabled state and playback order and restores the category. It ignores individual MCI result codes in this wrapper.

It obtains the current disc serial and scans the twenty serial fields. A match is copied to a temporary record, earlier records shift one position and the matched record becomes first. Its cached track table is then copied into the sound object. This is a move-to-front cache, not an append-only collection.

On a miss, the records shift down and the new serial is written first. A local sixteen-byte default table has seven leading ones followed by nine zeroes. Only when the disc has exactly sixteen tracks and the first is data does the sound object receive that table immediately. Independently of that special test, the first sixteen cached category bytes are set from the local defaults for the new record. Other bytes in the moved record are not wholly reinitialized here. The source comment suggesting insertion itself requires a sixteen-track disc is therefore inaccurate.

After reopening, network-game flag value 4 combined with frontend state 6 selects PlayNextTrack; other states stop CD audio. This policy is separate from whether CD audio is enabled. Device serial/category meaning, callbacks and MCI behavior are documented in [CD audio](sound-cd-audio.md).

The save loops do not clamp GetTrackCount to the per-record category area, and registry writes discard their result. The cache is local preferences, not save-bank match state or a network synchronization hash.

## Cursor and HUD-layer control

SetCursorMode (`0x491c80`) compares the current signed-byte mode with its integer input. If different, it stores the narrowed value and passes the corresponding cursorModeSeq table element to SetCursorAnimation. It has no table-range validation; valid mode IDs are a caller responsibility. Repeating the same mode preserves its current animation state rather than restarting it.

UpdateBattleHoverMode (`0x491cc0`) has three paths. When input flag value 2 is set and orderMode is `0x0e`, it updates placement-ghost validity and returns. When neither low input flag is set, it selects cursor `0x13` if needed and returns. Otherwise it picks the unit under the cursor, stores its ID, resolves a cursor from the current order mode/selection and changes animation only if the resulting mode differs.

The two early-return paths do not independently clear hoverUnitId here. [Placement](ingame-build-placement.md), [unit cursor resolution](units-unit-scripts.md) and [command dispatch](orders-order-dispatch.md) determine validity and actions. A displayed cursor is a hint derived from current interaction state, not execution of the order or proof the later target remains eligible.

PopUntilNamedLayout (`0x491d70`) defers normal HUD restoration when interface masks `0x800`/`0x65` or lobby dirty mask `0xe0` are set and force is zero. It sets interface bit `0x10` and returns zero. Otherwise it clears unitIndex and pops GUI layers until the mainHudLayoutName is on top, returning one, or until no layer remains, returning zero. It preserves the named layer rather than closing it.

CloseTopScreenIfNotNamed (`0x491e10`) closes just one top layer and clears unitIndex when it is not the named HUD. It is not equivalent to the full pop loop and applies no corresponding defer masks. The main-loop selection-refresh path consumes related pending flags, linking modal UI handling with later order-panel refresh.

FreePreviewCaches (`0x491e50`) independently frees savegame filename, description, side-list and radar-preview allocations when non-null, and clears all four globals. It does not close a bank or delete saved games. The save-browser module owns creation and interpretation of these cache arrays.

## Completion and limits

All 22 entries are covered, including the CD cache, environment sampling and interface utilities outside the main init/shutdown pair. Exact assets, registry contents, display capabilities, disc layout and runtime state affect results but were not executed or queried for this reference. Source order is the basis for the lifetime and failure observations above; broader startup and time chapters remain drafts until their remaining owning modules are documented.
