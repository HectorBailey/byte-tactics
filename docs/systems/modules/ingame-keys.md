# Battle key dispatch, STOP mode and large bitmap capture

## Scope and input boundary

The `ingame/keys` group owns five entries from `0x495860` to `0x496790`, all in [keys.cpp](../../../src/ingame/keys.cpp). They reset/arm STOP interaction state, choose numbered image paths, render large BMP mosaics and interpret one battle-key event. Key queue production, modifier encoding, selection implementation and generic GUI shortcuts belong to other modules.

[MainLoopTick](game-main-loop.md) invokes HandleGameKey only when its interface gate permits. [MainFrameTick](ingame-screenshots.md) separately intercepts certain global screenshot/fullscreen/dismissal keys before the battle callback. A key not handled here may therefore already have been consumed earlier, or may be handled by GUI dispatch.

## STOP interaction helpers

SelectStopOrder (`0x495860`) assigns orderMode one, clears input flag `0x20`, finds a current-layer gadget whose name contains STOP and clears that gadget's group status if found. It does not iterate units or enqueue a STOP order by itself. It selects the neutral/stop interaction state of the interface.

SetOrSelectStopOrder (`0x4958c0`) sets input flag `0x20` and returns when its argument is nonzero. With argument zero it performs the same mode reset and STOP gadget update. Both assume a current GUI layer; neither validates its existence before gadget lookup. Group display state and live unit order queues remain distinct.

## Numbered screenshot paths

BuildScreenshotPath (`0x495930`) constructs a search for directory plus optional backslash, prefix, wildcard and requested extension. Through HPI discovery it scans matching leaf names, parses the suffix after prefix length with atoi, retains the largest positive number starting from zero, closes enumeration and formats the next filename with at least four decimal digits.

It inserts a separator only for a nonempty directory that does not already end in a backslash. It does not normalize forward slashes, create directories, fill numbering holes or reserve the chosen filename atomically. Discovery includes the file layer's visible archive sources, not only loose files. Nonnumeric suffixes contribute zero and numeric prefixes can be accepted despite later characters. Caller/output capacities, max-number overflow and null strings are not checked.

This helper chooses BMP names for WriteScreenshot. The [PCX capture service](graphics-pcx.md) has its own similar naming path and different output behavior; the shared convention does not make them one function.

## Large BMP capture setup and ownership

WriteScreenshot (`0x495a30`) takes a directory/prefix, map origin X/Y and output width/height. It chooses a BMP filename, initializes a stack BmpWriter and opens it for that size. The [BMP writer](graphics-pcx.md) owns headers, palette, row placement and file closing. This wrapper returns void, so it does not report complete capture success to its caller.

It derives a horizontal tile width from viewWidthTiles times sixteen, a band height from viewHeightTiles times sixteen minus one, and saves battle-view left/top offsets. It temporarily removes the global out-of-memory handler, allocates a full-output-width ScreenShot frame one band high, retries once at half band height on null and reinstalls the standard handler. It does not restore an arbitrary prior custom handler. The destination file can already contain its BMP header when pixel allocation fails.

With a frame available, it saves actual camera scroll, map flag bits zero/one, pause bit zero, UI option bit six and the text-line count. It clears the two map flags and recalculates LOS, clears pause/UI bit six and sets text lines zero. These temporary changes prepare a clean capture view; this function does not call the simulation-step loop merely because pause is clear.

It wraps the allocated frame as a Surface, obtains a frame view of the retained display image and sets that frame's transparency byte zero. It then repeatedly repositions the camera, collects visible units, calls DrawBattleFrame with arguments one/zero and copies the rendered screen view into the band surface. The horizontal clip covers the current tile's output columns and the band's full height; the screen copy offset uses the actual clamped camera position and saved viewport borders.

The use of actual camera scroll is relevant near map edges: camera requests can be clamped, while the copy origin compensates for the difference. This is a mosaic of repeated renders from one maintained game state, not a higher-resolution single render target or a simulation replay.

## Band traversal, partial output and restoration

For each band the temporary frame is cleared to palette zero and columns advance by the view-derived tile width until output width is covered. The right edge is clamped to the band surface width minus one. Each band is written using BmpWriter::WriteRows, which seeks to its destination row region and writes selected source rows bottom-up.

The row bookkeeping includes a one-row seam adjustment. The first band uses source row zero and up to the full band height. On a continuing capture it advances the next camera row by band height minus one. Later bands skip source row zero, write at most band height minus one rows and retain the adjusted destination-row value. Final row counts use the remaining-height arithmetic rather than always writing a complete band.

The code passes the adjusted row, not the separate row-plus-one logical variable, to WriteRows. Thus the first later band starts one destination row before the previous band's nominal end, while its source starts at row one. This overlap/offset behavior is explicit in the traced calls; the routine does not contain a final pixel-coverage validation or a repair pass. It should not be described as guaranteeing every requested row is filled correctly solely because the file closes.

A failed WriteRows breaks band traversal, leaving a partial file, then follows ordinary state restoration. It frees the temporary frame, restores only the saved pause/UI/map bits and text-line count, restores camera position, recalculates LOS, rebuilds visible-unit IDs and redraws with arguments one/one. The whole original flag words are not blindly restored, preserving unrelated bits that may have changed during rendering.

Ordinary output failures still close the writer. An exception/fatal path has no local RAII/finally guard restoring every temporary game field. Invalid/zero tile or band dimensions, mismatched bitmap sizes and long paths are not validated comprehensively here. BmpWriter's source-stride/padding assumptions remain applicable. [CmdMakePoster](../../../src/game/console_commands.cpp) normally bounds requested output dimensions to map/view sizes, creates the screenshots directory and uses prefix BIGSHOT; direct callers would need to honor the same prerequisites themselves.

## One-event battle dispatch and modifiers

HandleGameKey (`0x495e90`) pops one key and returns immediately for event zero. It samples IsKeyDown with code `0xf9` into a modifier value, and some branches separately test `0xfb`. These are engine key-state identifiers. The mapping from physical keyboard combinations belongs to keyboard/input production; the table below records exact consumed event values without inventing platform shortcuts.

| Event(s) | Action and gates |
| --- | --- |
| `0x1b` | Escape behavior described below |
| `0xc5..0xcd` | Create squads 1..9 and play CreateSquad |
| `0x31..0x39` | Select squads 1..9 or open build-menu pages 0..8 according to visual option bit 8 and key-state `0xfb` |
| `0xd2..0xd5` | Save camera bookmark 0..3 and play SelectSquad |
| `0xe6..0xe9` | Restore camera bookmark 0..3 and play SelectSquad |
| `0x21`, `0x23`, `0x2a`, `0x60`, `0x7e` | Toggle visual flag bit 0 and save settings |
| `0x2c`, `0x2e` | Step build-menu page backward/forward with argument one |
| `0xf8` | Toggle pause and broadcast three-byte control packet `0x19`, subtype zero, new pause bit |
| `0xe2` | With modifier, track the hovered unit for unit-info display or clear tracking if none; without modifier, open unit-info dialog |
| `9` | In type 3, toggle tab menu only when orders-panel bit 2 is clear; otherwise fall through to the `0xe3` behavior |
| `0xe3` | With modifier, set/clear builder-hover tracking; without modifier, open in-game options when panel bit 0 is clear and set that bit |
| `0xe4` | Cycle message-associated units |
| `0xd7` | Toggle/start movie capture when UI option bit 1 permits |
| `0xec` | Toggle debug mode-enable state when UI option bit 1 permits |
| `0x5c` | With that same UI permission, execute the retained console command buffer using mask -1 |
| `0xe5` | Toggle visual flag bit 7 |
| `0xed` | Reset chat HUD indices |
| `0xaa` | Select all idle units |
| `0xc3` | Select units of the currently selected types |
| `0xad` | Toggle selection self-destruct orders as described below |
| `0x68` | Open share dialog only for mission type 3 |
| `0x6e` | Focus next local unit |
| `0xbc` | Select all visible units |
| `0x74`, `0x54` | Cycle camera follow with argument zero/one |
| `0x0d` | Play SmallButton and open talk dialog |
| `0x2d`, `0x5f` | Decrease speed within allowed state/bounds |
| `0x2b`, `0x3d` | Increase speed within allowed state/bounds |

For numeric squad/build events, visual bit 8 chooses the default role: when set, an unmodified `0xfb` state selects squads and a set state chooses build pages; when clear, those roles reverse. The separate `0xf9` value is forwarded to SelectSquad, retaining additive/selection policy in its callee. This dispatcher does not implement squad membership storage itself.

Category-selection events are `0xab`, `0xae..0xbb`, `0xbd..0xc2`. They form a seven-byte name CTRL_ followed by the character obtained by subtracting `0x69` from the event, then call SelectUnitsByCategory with the sampled modifier. Event `0xac` separately selects CTRL_C and finds the local commander. Event `0xbc` is deliberately the visible-selection case, not part of this generic category range.

## Escape, self-destruct and speed rules

Escape first handles an open options panel: clears its flag and, if the current layer is not the named HUD, clears unitIndex and closes that top screen. Otherwise, a nonneutral orderMode is reset to one, input bit `0x20` is cleared and the STOP gadget group is updated. If already neutral, it clears selection and force-pops layers until the named HUD. It does not universally stop all selected units' movement or erase their order queues.

Self-destruct creates the mission-type key SELFDESTRUCT and collects the current selection. It searches each selected unit for an existing order of that type and deletes found orders, remembering whether any existed. Only when none existed does it issue a new self-destruct order to the selection. With a mixed selection, finding one existing timer cancels those found without arming the other units. Order execution, delay and effects belong to [unit orders](orders-unit-orders.md).

Speed changes are blocked when endgame/debug flag value 2 is set. They also return when the local player is active and PlayerInfo bit `0x40` is set. Otherwise requested speed can move within one through twenty through SetGameSpeed with broadcast one. There is no additional host-only test in these branches. [Game-state speed control](game-game-state.md) and network packet policy remain the authority/replication boundaries.

## Movie capture and debug state

With UI option bit 1 set, movie event `0xd7` stops capture by clearing a nonzero index. Starting scans base-path MOVIE* entries, parses their numeric suffixes, chooses the next maximum index and forms a MOVIE directory with at least three digits. It creates that directory, draws a battle frame with arguments zero/one, captures an initial FRAM image and sets the next capture deadline to current gameTick. It does not validate directory creation/capture success or clamp movieOutputRate here.

Later [battle frames](game-main-loop.md) capture at their simulation-tick deadline and advance it by integer 30/rate. This initial deadline can allow an immediate further capture on the next qualifying pass. The index is a capture-on sentinel plus directory number, not a frame counter incremented on every screenshot.

Debug event `0xec` toggles endGameFlags bit 1. Enabling calls SetDescListCleanupFlag with zero. Disabling clears bit 0, sets debugMode zero and calls it with one. The backslash event replays the existing global console buffer: [ExecuteCommandLine](../../../src/game/console_commands.cpp) uses that buffer for a null text pointer, not a new empty command.

At the end of every nonzero key event, if debug-enable bit 1 is set, HandleDebugHotkey receives the original event as well. Thus an event can have an ordinary action and a later debug action; enabling debug can also make the same event reach that tail. [Dialog/debug dispatch](ingame-dialogs.md) documents the six recognized debug event values.

## Completion and ownership boundaries

All five owned entries and every key-switch branch are covered. Selection sets, GUI behavior, order queues, camera state and image-file services retain their own ownership. Screenshot path probing does not reserve a name, selecting STOP mode does not issue an order, and receiving a key is not a permission guarantee for every downstream action. No key event, image capture, console command or game session was executed for this reference.
