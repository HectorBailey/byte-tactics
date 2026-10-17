# Endgame snapshots, outcome screens, fades and result controls

Status: finished. `frontend/endgame` owns 28 entries in `0x41d920..0x420620`. Implementation is [endgame.cpp](../../../src/frontend/endgame.cpp) and the [statistics-collection companion](../../../src/frontend/endgame_41dc20.cpp). The group includes the initial game-object allocation and debris-cache lifetime helpers as well as endgame presentation. It consumes outcome flags; it does not determine victory from units or mission conditions.

## Object allocation and retained state

CreateGameObject (`0x41d920`) allocates sizeof(Game) plus an offset of GetTickCount modulo 1000 times seven, zeroes the whole allocation and placement-constructs Game at the interior pointer. The constructor supplies build date/time strings and initializes two grid-related integers, while embedded Player constructors run normally. g_game is therefore not necessarily the original allocation address or naturally aligned. This body does not retain an explicit allocation-base pointer or provide ordinary delete semantics for that interior address. [Application startup](game-main.md) owns its call sequence.

Endgame state at game offset `0x39057` is separate from wider game mode and frontend menu state. It is accompanied by timing/completion fields, current statistics-column activation, saved display brightness, last-frame/glamour surfaces, three palette/fade buffers, seven column maxima, current mission/result and the campaign result array. SetEndGameState (`0x41d9f0`) masks its input to eight bits and stores the resulting integer; it does not initialize associated resources or timestamps.

InitMissionStatus (`0x41da30`) fills twenty-five mission results with U and appends NUL at index twenty-five. SetMissionStatus (`0x41da10`) stores W for a nonzero result or L otherwise at caller base plus index, without validating either. This is in-memory progression display state; settings/save consumers own its persistence.

## Statistics collected before battle teardown

CollectEndGameStats (`0x41dc20`) is called by ShutdownIngameSystems in [game_state.cpp](../../../src/game/game_state.cpp) before unit/player/map resources are released. It copies global winner bit four at `0x3923b` into the retained won field. For campaign type one it records the mission index and writes W/L into the result array, without checking the twenty-five-mission bound.

It initializes column maxima to ten for kills/losses and one hundred for produced/wasted resources and score, then zeroes ten packed 58-byte rows. A player is included when active, type one/two/three, not sentinel index ten and not observer bit six, **or** when unitsCreated is nonzero. Thus historical unit creation can admit a slot that the active/type/observer test excludes. A nonzero rejectReason then suppresses the row regardless. Included names are copied into thirty bytes without forced termination; seven integer values follow immediately.

The values are kills, losses, total energy produced, total metal produced, energy wasted, metal wasted and aggregate score. Resource doubles convert to integer. Score is integer conversion of `(gameTick / 60) * timeMul` plus separate integer conversion of kills times killMul; gameTick division truncates first. Negative final scores clamp to zero. Each column maximum increases only above its initial floor. This local results snapshot differs from [online reporting tables](game-statistics.md), whose inclusion, negative-score and winner-field policies are not identical.

## Preparing outcome artwork and fade buffers

SetUpEndMissionScreen (`0x41da60`) allocates three 1024-byte buffers for signed channel increments, target palette and current palette. It saves display paletteBrightness and forces brightness one, then allocates another 1024-byte palette for optional glamour artwork. A null mission name-slot-five clears the glamour-image pointer. For a won campaign with a nonnull slot, it constructs a bitmaps/glamour PCX path using the name after its first byte. A missing resource falls back to glamour/Arm01.PCX; otherwise it keeps a path substring beginning eight bytes into the constructed path. It loads that bitmap and retains its palette pointer.

Other cases preload Outcome1 for campaign or Outcome0 otherwise through LoadPictureCached with fourth argument one, which loads/caches without attaching a background. The separately allocated local palette is not retained or freed on that nonglamour branch. A nonnull name without the winning campaign condition also does not explicitly clear an old glamour-image pointer. The ordinary lifecycle depends on prior cleanup/initial zero state; repeated preparation is not idempotent and does not release old fade buffers first. Missing loads and fixed path lengths are not fully checked.

## Screen and palette fades

StartScreenFade (`0x41dee0`) sets ten steps, schedules nextTime to GetTicks plus one and clears done. StepScreenFade (`0x41df20`) builds a rectangle from origin to screenWidth/screenHeight, then when nextTime is strictly less than current ticks calls FadeRectangle with level steps minus twenty-nine, reschedules to now plus one, decrements steps and sets done at zero. It performs at most one step per call, without catching up missed deadlines. The bounds are passed exactly as stored, leaving clipping to [drawing](graphics-draw.md).

StartPaletteFade (`0x41dfc0`) copies all 1024 bytes of source/target palettes, uploads the initial palette, clears done and computes one signed-byte increment for every byte, including each entry's fourth byte. Equal channels get zero; other channels use integer difference divided by steps with a minimum magnitude of one. It does not set the next deadline. Zero steps divides by zero when unequal channels exist, and increments outside signed-byte range wrap on storage. Normal glamour use supplies five steps, but this is not a fully general bounded fade interface.

ScheduleFadeTick (`0x41e240`) schedules now plus one. IsFadeDone (`0x41e260`) returns the shared completion field. StepPaletteFade (`0x41e270`) runs when now is at least the deadline, adds the saved signed increment to every current byte, clamps toward target according to increment sign, and marks done when all 1024 bytes match. It uploads the palette and schedules now plus one. It does not stop merely because done was already set; the caller controls whether stepping continues. Screen/palette fades share timing and completion fields, so overlapping use is not independent.

## Result labels and seven animated columns

FillEndGameStatistics (`0x41e420`) appends one visible row for each retained nonempty name. It creates a PlayerColorN hotspot bound to logos32 and the live PlayerInfo color, a width-ninety name label using font slot one, and seven type-thirteen bars using font slot zero. Rows start at y 93 and advance twenty pixels. Bars are 67 by 18 at x 112, 186, 260, 334, 408, 482 and 556, corresponding to Kills, Losses, EProduced, MProduced, EWasted, MWasted and Score.

Each bar starts at zero, targets the snapshot value, uses the retained column maximum for scaling, interval one and a per-update increment stored as float max(value divided by fifteen, one). Generic progress rendering converts that increment to its own update representation, so values and call cadence determine the visible animation. Bars have showText and internal active set, but their outer gadget flag starts zero; the endgame machine activates columns over time. Repeated FillEndGameStatistics calls append more records unless the caller provides a fresh/reset layer. Record and name-length limits remain those of [dynamic GUI additions](gui-dialogs.md).

ActivatePlayerGadgets (`0x41f5c0`) enables every named prefix-plus-slot control for nonempty retained names, without drawing directly. AreStatBarsComplete (`0x41ea30`) scans indices zero through count minus one, returning zero for a type-thirteen gadget whose outer flag is zero or current is below target. It **omits index count**, although ordinary gadget tables use controls one through count. The machine's inline StatsComplete has the same bound. Completion can therefore ignore the last bar rather than proving every displayed bar finished.

## Mission-list decoration and next/retry eligibility

BuildScrollItems1 (`0x41eaa0`) allocates count times 128 zeroed bytes, walks packed input names and prefixes L with byte `0xff` plus space, W with `0xfe` plus space, and U with `0xfd` plus space. It appends each name immediately after the previous NUL-terminated result, not at fixed 128-byte strides. It frees the input allocation and transfers the result to the caller. BuildScrollItems2 (`0x41eb60`) allocates b times 258 bytes, caps a at b, copies the first a packed names directly and prefixes the remaining names with &G. It also consumes/frees input. Neither helper validates aggregate lengths, negative counts, result-array length or input-string termination; allocation units are estimates, not per-record bounds.

ShouldShowNextMission (`0x41f040`) returns true for campaign type one when either won is zero or the next mission exists. The source expression repeats MissionExists, but for a stable result it simplifies to that rule. Consequently a lost final mission can still present retry/mission controls even though no next mission exists; the helper's name alone is misleading. A won final campaign with no next mission returns false. The inline HasNextMission uses the same test.

OpenEndMissionScreen (`0x41f0a0`) blanks/presents, loads ENDMSN.GUI with `0x80`, installs its callback and allocates a 32-byte data object with null list storage. When the next/retry rule is true it reselects the completed mission, attaches outcome1 and makes Start the default; otherwise it attaches outcome0 and focuses MainMenu. In the true case it obtains the mission-name list, consumes it into decorated scroll items, configures Missions, adjusts KNOB range, scrolls first to current mission and then current plus one if won, and applies difficulty buttons. A loop finds the first U result but never uses that index.

It draws badge sequence A for won with the local player inactive or not observing; otherwise badge B. That is a separate badge condition from global won. Lobbied flag four changes MainMenu's text to OK. It enables keyboard input, redraws and chooses cursor mode `0x13`. Required layout/knob/badge lookups and next-index validity are trusted.

EnableEndMissionButtons (`0x41f400`) enables Start, LoadGame, SaveGame, KNOB, Missions, Difficulty, AdjustDiff and MainMenu in the next/retry case and focuses Missions. Otherwise it enables MainMenu, moves its y to 416 and focuses it, relying on other buttons' layout/default state rather than explicitly deactivating all of them here. It marks layer/context changed. ShowEndMissionScreen (`0x41f630`) opens the screen, fills statistics, enables buttons and selects wider game mode and endgame state seven. It assumes prerequisite snapshot/fade state exists.

## Endgame state-machine sequence

RunEndGameState (`0x41f7f0`) selects the game offscreen surface and processes network packets on every call, including waiting UI phases. It handles one numbered state then flips the screen at the common tail:

| State | Work and next condition |
| --- | --- |
| 0 | For multiplayer type three, allocate a last-frame surface sized from display fields, copy through DrawSurface using those same fields as x/y arguments, report event seven and select state one. A local rejection/message code other than zero/two opens its translated reason and clears the code. Other game types clear lastFrame and select two. |
| 1 | While the top screen is MSGBOX.GUI, draw the retained frame/messages and process/composite GUI. Otherwise select two. The frame-draw call again uses display width/height as position arguments, not origin. |
| 2 | Initialize ten-step screen fade, disable cursor overlay and select three. |
| 3 | Perform at most one screen-fade step; once done force 640 by 480 surfaces, select four and reset offscreen target. |
| 4 | Campaign without its disc opens CDCHECK.GUI and selects eight. Otherwise select five. |
| 5 | Allocate/setup outcome artwork. A won final campaign with no next mission and noMovie clear chooses ending frontend movie state four/five from player-zero side when display bit one is set, otherwise main-menu state two; then selects wider mode two. A won campaign with glamour image starts five-step palette fade from black, draws glamour and selects six. Other paths open result GUI, append statistics, enable controls and select seven. |
| 6 | Step palette fade until done, refreshing a deadline to now plus tick rate and clearing the glamour-sound latch. After completion, start glamour sound once; after the deadline, a popped key or cursorKeyFlags stops it and opens the results screen/state seven. Five further tick-rate intervals later, draw Click to continue. |
| 7 | If scanned bars are complete, enable GUI key commands, select eight and restore cursor mode nineteen/overlay. Otherwise update/composite GUI and activate statistics columns according to deadline/key behavior below. |
| 8 | Process GUI with cursor hidden/restored, flip, then composite layers with cursor hidden/restored. The common tail flips again. This also serves the CD retry dialog. |

In state six the fetched mouse snapshot is not directly tested; cursorKeyFlags supplies the pointer/button activity condition. The deadline prevents immediate dismissal during the fade and first postfade interval. If no palette step ever runs, the surrounding retained deadline/latch state is not independently normalized by entering six; the ordinary path does run the fade.

State seven initially activates columns in order zero through six, normally one every ten game ticks, with EndGameStatBar sounds and EndGameScore for the final column. A popped key bypasses the deadline only outside multiplayer. Once the timed/skip condition is entered, any popped key activates every column and plays ActivateAllStatBars, even in multiplayer where it did not independently bypass the deadline. This enables bars rather than setting all current values to their targets. Multiplayer sends local player economy twice during each activation step. It advances bar and deadline even after bar exceeds six, although the switch then activates no new column. GUI progress callbacks own actual bar advancement.

DrawEndGameFrame (`0x41f760`) is a separate callback that performs the state-one retained-frame/messages/GUI presentation and returns one only in that state, otherwise zero. Its use of display width/height as draw coordinates is preserved as observed call behavior rather than silently described as an origin blit. [Surface/drawing contracts](graphics-surface.md) determine clipping and the actual visible result.

## CD retry and result-screen activation

OpenCdCheckDialog (`0x41f700`) loads CDCHECK.GUI with `0x101`, installs HandleCdCheckClick, restores overlay, enables keyboard input and redraws. Its handler (`0x41f680`) ignores cleanup. OK plays Options and selects endgame state five if campaign-disc lookup succeeds. Otherwise it shows the insert-disc message; nonaccepted/unknown paths clear selection. The dialog is therefore an explicit retry gate for preparation, distinct from the result-screen Start path below.

HandleEndMissionClick (`0x41ec50`) handles LoadGame/SaveGame by opening their screens and clearing selection twice. Start/Missions checks the campaign disc, but a failed lookup only displays a prompt and clears selection, with **no return** before archive registration and mission selection. It requests frontend substate ten, restores overlay and busy cursor, then tries the selected mission. Success resets main-menu state/background, clears start/network flags as specified, enables campaign bit three, selects mission type one, clears outcome bits, chooses frontend state thirteen and wider game mode two. That returns through briefing before a new battle.

MainMenu selects frontend state two, wider game mode one and restores overlay/busy cursor. Difficulty cycles zero/one/two in both game and setup options and clears activation. Other/failing paths clear activation. Selection indices and lists are trusted, and popup callbacks can change the top layer during the retained Start path.

Cleanup notification index `-1` blanks the screen, frees retained last-frame/glamour surfaces, optional glamour palette and all three fade buffers, nulls their pointers, frees the mission-list buffer/data object, leaves the network game when lobby flag four is set, selects CD category four and restores saved brightness. It runs before generic layer destruction. It does not free shared picture-cache entries, badges, logos or the setup table. Cleanup depends on the ENDMSN callback actually being reached; paths leaving preparation for a final movie do not invoke this cleanup directly in the state machine body.

## Debris-cache helpers and coverage

InitMemoryCache (`0x4205f0`) clears pointers in the global debris memory cache and registers FreeMemoryCache with atexit. FreeMemoryCache (`0x420610`) releases that cache's buffer. Repeated initialization would register repeated exit callbacks; this helper has no registration latch. Cache allocation behavior is covered by the [particles reference](graphics-particles.md).

All 27 main-file bodies and the collection companion have been traced. Required assets, callback reentrancy, pointer lifetime and unchecked mission/count/path limits are identified explicitly. Endgame winner computation, save formats and network event protocols remain in their own modules. This writeup used existing source only, without a build, installed dependencies or game execution.
