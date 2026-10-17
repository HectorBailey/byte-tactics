# In-game dialogs, score display, music activity and simulation scheduling

## Scope and state

The `ingame/dialogs` group owns 23 entries in `0x493340..0x495860`, all in [dialogs_493340.cpp](../../../src/ingame/dialogs_493340.cpp). It includes sharing/chat/unit-information dialogs, tab-menu handling, score-panel rendering, music-activity sampling, frame pacing, the simulation-step loop and debug event dispatch. Completing only its pacing functions would leave most of its responsibilities undocumented.

The routines share game state but use several independent flags: orders-panel bits track modal interface state, lobby dirty bits select tab/chat presentation, PlayerInfo controls player role/options, and pause/lag/faster bits describe pacing. GUI layer callbacks receive a hot-gadget index of -1 for teardown. Their teardown branches free owned display/list resources or clear flags; a button event is a different path.

The neighboring [application frame](ingame-screenshots.md) calls activity/audio service and timers before dispatching a state callback. The [battle pass](game-main-loop.md) calls the pacing and simulation services described below, while [game-state lifecycle](game-game-state.md) prepares their resources and resets battle arrays.

## Sharing sliders and unit gifts

UpdateMetalReadout (`0x493340`) and UpdateEnergyReadout (`0x493390`) locate the corresponding slider, obtain its integer value and set translated text in METAL# or ENERGY#. A missing slider leaves its readout alone. These are display updates, not economy requests or transfers.

OpenShareDialog (`0x4936f0`) refuses a local PlayerInfo with bit 6 set. It loads SHARE.GUI with flag `0x800`, sets orders-panel bit `0x40`, and installs HandleShareDialogEvent plus the game owner. METAL and ENERGY sliders use the local player's current stored balances truncated to integer maxima, start at zero and install their readout callbacks. The knob length comes from gadget height; travel comes from gadget width minus that length. There is no live balance observer installed here to refresh those maxima while the dialog stays open.

The routine allocates a concatenated-name buffer sized numPlayers times 30, resets a ten-entry network-ID mapping to -1, and walks all ten players. Eligible recipients must be active, have controller type 1/2/3, index other than 10, either still own units or have created none, not be controller type 1 and not have PlayerInfo bit 6 set. Names are copied without checking that each fits its thirty-byte share of the allocation. The list records network IDs rather than assuming its visible row equals a player slot.

With no recipients it closes the screen before configuring the list. The local names allocation has not yet been handed to ConfigureListBoxByName and is not explicitly freed on that early path. Otherwise it attaches the names, initializes both text readouts, marks the dialog changed, enables keyboard input and renders the layer.

HandleShareDialogEvent (`0x4934b0`) frees the player-list records and clears panel bit `0x40` on teardown. MAPINFO and SHARUNIT events play the option sound, mark changed and clear the current gadget selection. OK obtains the selected row, returns for a negative selection, maps its saved network ID back to a slot, and rechecks participation, live/not-yet-used status and the recipient's bit 6 before acting. Positive row bounds and a failed-slot sentinel are not separately validated at every access here.

The resource mapping must be followed into [player transfer bodies](../../../src/game/players_464290.cpp):

| Slider | Called symbol | Fields actually transferred |
| --- | --- | --- |
| METAL | TransferEnergy at `0x464c60` | Source metal/SpendMetal, destination metalMake |
| ENERGY | TransferMetal at `0x464b30` | Source energy/SpendEnergy, destination energyMake |

The names are reversed relative to their field behavior. The dialog therefore passes matching resources despite apparently mismatched symbols. Flag one requests source spending and the corresponding share message. The callee clamps to the source's current balance, so a slider's old maximum is not the final transfer amount. Difficulty-dependent recipient adjustments and received-event behavior are covered in [players](game-players.md).

SHARUNIT additionally invokes GiveSelectedUnitsToPlayer (`0x4933e0`). That helper collects the local selection, obtains the Commander category mask and gives units only when their low two state bits are not 2, both carrier/cargo links are zero and their type is outside that category. It does not grant ownership of transported cargo indirectly or let the gift change a commander's owner through this path. [Ownership transfer](units-unit-commands.md) owns the actual handoff, cleanup and replication.

MAPINFO calls both local ShareMapInfo and SendShareMapInfo. Neither unit gifting nor map sharing requires the recipient to pass a separate alliance test in this handler. CANCEL plays the previous sound. OK/CANCEL return without an explicit CloseTopScreen in this callback; generic GUI gadget/layer behavior supplies any configured dismissal. A callback return should not be documented as an unconditional close call.

## Chat target controls and draft lifetime

OpenTalkDialog (`0x494050`) rejects local PlayerInfo bit 6 and an active unit-info modal flag `0x800`. It initializes the 129-byte draft once. Expanded chat is selected when lobby flag `0x100` is set and game type is 3: TALK2.GUI uses layer flag `0x800`; ordinary TALK.GUI uses `0x880`. It installs the handler/owner, sets panel bit 4, restores draft text, sets SENDTO and focuses TALK.

Non-type-3 games disable SENDTO. Expanded multiplayer chat also sets SENDTYPE, refreshes ally presentation and calls ResetPlayerGadgets (`0x493ae0`). That reset disables each PLAYER-number gadget and sets each eligible nonlocal LIVEPLYR-number stage according to chatMode: zero all, one allied, two not allied, three the custom recipient byte. It filters controller types zero/four rather than applying the share dialog's exact participation test.

HandleTalkDialogEvent (`0x493bf0`) clears panel bit 4 on teardown. Clicking a gadget with the LIVEPLYR prefix selects custom mode three and copies its button stage into the recipient byte selected by the numeric suffix. That suffix is not range checked; a malformed gadget name can write outside the eleven-byte mask. Clicking SENDTYPE obtains a mode, folds values at least four back to zero, refreshes player gadgets and refocuses TALK.

Clicking SENDTO stores the low button-stage bit into the expanded-chat flag, copies the current draft, closes the layer and reopens the selected presentation. This preserves text across the layout change. It does not itself send the draft.

Submitting TALK copies up to 255 text characters, skips leading spaces and processes commands/temporary recipients as follows:

1. A leading plus invokes [ExecuteCommandLine](util-commands.md) on the following text. The initial permission mask is one, replaced by seven when UI option bit 1 is set, with bit 2 also added for noncampaign state. If the command result contains bit 2, the send mode becomes all. Command execution does not automatically consume the text from the later chat path. This caller declares an integer return, but the forwarding definition in console_commands.cpp is void and delegates ExecuteCommand; that provisional declaration mismatch limits the cleanliness of the result contract.
2. Nonempty text can begin with a single digit or A/E followed by one of comma, colon or semicolon. A digit selects player 0..9 when its network ID is nonzero, temporarily clears all eleven recipient bytes and sets that player's byte. A selects allied mode; E selects enemy mode. The prefix is skipped for the outgoing text. Other first characters retain ordinary mode/text.
3. It calls SendChatMessage for the local player with argument four and an optional player/Allies/Enemies label, then restores the prior chatMode and saved recipient state.
4. It clears the draft and expanded-layout bit, then focuses TALK again. An invalid numbered target reaches the clear path without sending.

The saved recipient structure contains ten bytes, whereas the temporary clear spans eleven. Normal player targeting uses the first ten slots, but the eleventh byte is not restored by that save/copy. Per-message targeting otherwise does not permanently replace the selected mode. Chat transport, display prefixes, network recipients and command definitions belong to their corresponding modules.

## Unit information dialog and image ownership

OpenUnitInfoDialog (`0x4942e0`) returns if panel bit `0x800` is already set. With a hovered GUI gadget it uses the first sixteen name bytes to resolve a unit type. Otherwise it uses hoverUnitId and tests visibility to the current playerIndex observer before obtaining that unit's type. If the GUI-name route fails it does not additionally fall back to the world-hover route; type zero simply returns.

For a valid type it loads UNITINFOx.GUI with flag `0x1000`, installs handler/owner, assigns the HOTR image callback and loads the unitpics PCX from the definition's unitname. It builds a property-list allocation through MakePropList and adds translated labels for Cost, Energy, Metal, Build Time, Statistics, Max Velocity, Acceleration and Turn Rate. Labels begin at Y 32 with fifteen-pixel steps; values from the NUL-separated property list begin at X 240 and the same spacing. Added gadget attributes are set to `0x411`. It frees the temporary property list and sets the NAME text from the definition's display name.

DrawUnitInfoImage (`0x494290`) draws the retained image only when nonzero, using the gadget's rectangle origin and the layer surface. HandleUnitInfoDialogEvent (`0x494220`) frees that image and clears the modal bit on teardown. DONE plays a small-button sound; unrelated events clear the selected gadget. A missing picture does not independently abort dialog creation here. Gadget lookup, type/array bounds and dynamic text additions rely on the expected GUI/definition layout.

The displayed values are definition information. The dialog does not simulate a build time under current shortages or derive a guaranteed weapon engagement outcome. [Unit types](units-unit-types.md), [info panel](ingame-info-panel.md), [economy](game-economy.md) and combat references explain those separate quantities.

## Tab menu and HUD callbacks

ToggleTabMenu (`0x495010`) plays SmallButton. If any lobby bits `0xe0` are set, it clears all three and closes the top screen only when that screen is TABMENU.GUI, then returns. Otherwise it sets the open bit `0x20`, hides the cursor, loads TABMENU.GUI with flag `0x800` and installs its handler/owner.

It computes a candidate count by skipping active controller-type-1 players, then skipping any player whose info has bit 6 set. It increments for every remaining slot, including inactive slots. This is not a count of live nonallied opponents, despite the overview comment, and it assumes info pointers exist on the examined records.

For type 3 with local bit 6 clear, ALLIES/SHARE are enabled when that count is positive. CONTROL is enabled when lockFlags bit 0 is clear and IsHostLocal succeeds; its test does not include the count. Other mission/observer cases disable all three. The function enables key commands, renders the layer and shows the cursor.

HandleTabMenuEvent (`0x494740`) opens options, share, control or allies dialogs according to the clicked name, playing BigButton and clearing the selected gadget. OPTIONS also sets orders-panel flag one. CANCEL explicitly closes the top layer. Teardown clears lobby bits `0xe0` and disables key commands. HandleTabMenuCancel (`0x494840`) has the same teardown cleanup and otherwise merely clears selected gadgets except CANCEL; it does not duplicate the full submenu logic.

OpenMain2Layout (`0x495200`) loads the retained HUD name with flag `0x20`, installs HandleMain2LayoutEvent and the game owner. That handler (`0x494890`) only clears a nonnegative current gadget selection. It has no special resource teardown. Screen stacking, named HUD restoration and selection refresh are coordinated with [game-state HUD helpers](game-game-state.md).

## Sliding score panel and event flashes

FlashScorePanelKillLoss (`0x4948b0`) sets the indexed kill/loss flash bytes to 30 when the corresponding arguments are nonnegative. It does not reject indices above nine. DrawScorePanel (`0x4948e0`) periodically subtracts two from positive flash bytes when GetTicks passes its decay deadline, then advances that deadline by one. Ordinary 30-to-zero values remain even; arbitrary odd byte values can underflow because subtraction is not saturated.

The panel opens when visual flag `0x80` is set, or the space-key state is down without an active type-3 text-entry focus. Otherwise it closes. Slide position ranges nominally zero through 125. Each pass moves by one quarter of the remaining distance, with a minimum step of one, playing Panel/Options sounds at the transition boundaries. The movement is per draw invocation, not explicitly elapsed-time scaled.

The panel begins at screen width minus slide position and Y 32, is 125 pixels wide, and sizes its faded background using numPlayers times forty plus fourteen. It draws translated Kills/Losses headings, then searches ranks in order. Qualifying rows are active types 1/2/3, index not ten, not eliminated after having created units, and bit 6 clear. The local player receives extra fades; the colored 32xlogos frame is mapped across each row, followed by name and counters.

When battleCommanderDeath equals two it displays commander kills/losses; otherwise it uses ordinary counters. Event flash values are passed to text drawing. If no eligible player has the expected rank, drawing itself decrements higher eligible ranks to close the gap. The renderer therefore mutates ranking state while repairing display order; it is not a purely observational sort over a separate immutable list.

## Combat activity and CD category selection

AddCdActivitySample (`0x494ff0`) adds its integer argument to the current entry of a thirty-entry ring. [Damage](../../../src/units/unit_targets.cpp) adds one when an identified attacker or victim belongs to the local player. [Unit death handling](../../../src/units/units_485010.cpp) adds five for a kill attributed to the local player. The value is gameplay activity, not frame duration, checksum disagreement or network desynchronization.

UpdateCdCategoryByActivity (`0x494e70`) samples only with network-game flag value 4 set, loading either not started or already loaded, and current timer ticks strictly beyond the last sample plus thirty. It increments a stable-sample counter; after more than ten samples, it sums all thirty ring entries starting just before the current write index and also sums the first five encountered entries as recent activity.

If the current sound category is zero, total activity above fifty or recent activity above thirty, combined with local unit count above thirty, selects mode one. If current category is one, total below ten, recent zero and stable counter above sixty select mode zero. Other categories do not trigger those two transitions. A change from the remembered activity mode updates the sound category and resets stability.

It then advances/wraps the ring write index, clears the new bucket, calls ReportIntervalTimer and records the current sample time. Large time gaps produce one sample per call, not a loop filling every missed bucket. This is hysteresis for background-music activity; [CD playback](sound-cd-audio.md) still decides actual track availability, order and device behavior.

## Frame pacing and effective speed

UpdateFramePacing (`0x495230`) records elapsed GetTicks since the previous budget timestamp, updates that timestamp, and starts with effectiveGameSpeed multiplied by 0.1. It marks faster when effective speed is below requested speed. It scans active type-3 players with units for the smallest syncTick below local gameTick, retaining that player and computing lag from local tick.

Lag at least 900 sets lagging and multiplies the rate by `(3600 - min(lag, 3600)) / 2700`, with a minimum factor of 0.01. Below 900 it clears lagging. It then multiplies elapsed timer ticks by rate, adds fractional carry, takes the floor as the pending whole-step count and stores the fractional remainder as a float. Negative pending counts become zero.

Pause sets pending steps to zero and returns after those measurements/carry updates. Otherwise more than five steps is capped to five and increments speed hysteresis. Above ten capped observations, hysteresis resets and effective speed decreases if greater than one. A noncapped budget decrements hysteresis; below -100 it resets and effective speed increases toward requested speed. â€œNoncappedâ€ includes positive budgets of one through five, not only idle frames.

The discarded whole steps above five are not restored to fractional carry, which was computed before capping. This is bounded adaptive pacing, not unlimited catch-up. Requested speed, timer ticks, simulation ticks and profiling milliseconds remain separate quantities. [Speed controls](game-game-state.md) set requested/effective state and optionally broadcast it; this routine adapts locally to its conditions.

## Simulation-step order, profiling and after-loop work

RunGameSteps (`0x495490`) copies simStepsPending into a local loop count. It does not itself clear the original pending field. For each positive pending iteration it advances gameTick and invokes these phases:

| Order | Work | Profile bucket charged afterwards |
| --- | --- | ---: |
| 1 | Receive packets, when its argument is nonzero | 0 |
| 2 | Update all units | 1 |
| 3 | Update projectiles | 7 |
| 4 | Update explosions/debris | 8 |
| 5 | Update players | 2 |
| 6 | Features, global GAF sequences, wind, meteors, camera follow | 8 |
| 7 | Particles | 6 |
| 8 | Blink state | 8 |
| 9 | Local resource sharing and queued outgoing packet flush, only with nonzero argument and packet manager enabled | 0 |

The argument named showStats controls actual networking/sharing, not only visual statistics. Its value is supplied by the [battle frame's network/local branch](game-main-loop.md). Each profile charge uses elapsed milliseconds since the previous charge and accumulates that span in the listed bucket. The grouped environment charge includes several services rather than attributing independent times to each.

After all iterations, three post-step hooks run, the oldest-message and eyeball-marker expiry functions run, and bucket eight is charged. Those tail actions occur once per RunGameSteps invocation, including a zero-count call. The ordinary caller invokes it only with pending work. A negative count is not defensively rejected here; the pacer normally prevents that input.

This sequence places unit-created projectiles before the projectile phase and unit resource requests before the player economy phase. It does not imply a newly created object is always processed immediately regardless of that subsystem's own iteration/authority rules. [Time and simulation](../02-time-simulation.md) explains the cross-system consequences with the unit/player references.

## Debug event dispatch

HandleDebugHotkey (`0x4956c0`) has six recognized integer events:

| Event | Effect |
| --- | --- |
| `0x69` | Toggle endgame flag bit 0 |
| `0x6d` | Increment debugMode, wrapping exactly five to zero |
| `0x50` | Enable page flipping |
| `0x70` | Disable page flipping |
| `0x5d` | For the hovered unit when its definition index is nonzero, set last-attacker slot ten, set flag bit 14 and clear attacker pointer |
| `0x3d` | For active types 1/2/3 with index not ten, assign stored energy/metal to their capacities |

Other event values do nothing. The hovered-unit path indexes the current ID before testing its definition and imposes no caller permission check here. The resource-fill path changes stored balances directly, without an economy request or resource-transfer packet in this body. The dispatcher should not be described as a complete mission-script interpreter simply because an overview uses that phrase; actual callers and permission gates determine how events can reach it.

## Completion and remaining dependencies

All 23 entries are covered across dialogs, drawing, audio policy, scheduling and debug actions. Generic gadget lookup, focus, layer dismissal and allocation behavior belong to the GUI modules; network recipient interpretation and share packets belong to networking; console handlers determine individual plus commands. Asset-defined names and actual session state determine presentation and interaction outcomes. No game, build, network session, GUI interaction or audio playback was executed for this reference.
