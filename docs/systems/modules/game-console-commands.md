# Console commands, debug scripts and terrain diagnostic overlays

Status: finished. `game/console_commands` owns 83 annotated functions in `0x416240..0x418cd0`, all in [console_commands.cpp](../../../src/game/console_commands.cpp). Besides command callbacks it contains world-to-screen helpers, selected-goal drawing and terrain diagnostic rendering. The three command tables are data, not three additional functions. This reference describes their behavior without reproducing source listings or executing the commands.

## Registration, entry paths and permission masks

[InitCommands](../../../src/ingame/control_panel.cpp) registers all three tables and installs DefaultCommandHandler with mask four. The ordinary table has 43 commands with mask one; the cheat table has ten with mask two; the debug table has thirty with mask four. Null-name records terminate the tables. Registration does not mean a callback is available in every context: [command infrastructure](util-commands.md) requires an intersection between the caller's mask and the registered mask. It retains token zero as the command name, supplies atoi/atof conversions and can invoke the default for a known but mask-ineligible name.

ExecuteCommandLine (`0x417b50`) initializes a local argument object, tokenizes and dispatches with the supplied mask. A null input uses the global saved command buffer. A nonnull input copies at most 79 bytes into that buffer without explicitly terminating it, but tokenizes the original input, so this copy is not the execution-length limit. A later null-input repeat can encounter an unterminated saved command. The local CommandArgs declaration here is a provisional layout view; the canonical tokenizer's actual token/text capacities and overflow behavior are documented in the infrastructure reference.

The [talk dialog](../../../src/ingame/game_dialogs.cpp) strips leading spaces and recognizes a leading plus. It starts with mask one, selects seven when UI-option bit one is set, and adds cheat mask two for a noncampaign game. A dispatch result containing bit two changes chat mode to zero; it is the selected handler mask, not proof its action succeeded. Chat processing continues afterward. [Keyboard handling](ingame-keys.md) also exposes repeat execution with mask minus one while that UI-option bit is enabled. Therefore the table's label “debug” does not by itself establish a compile-time exclusion: these callers and runtime bits determine access in the traced source.

Now (`0x416e90`, ordinary mask) sets UI-option bit one only when there are exactly six tokens and the five following tokens are the exact, case-sensitive sequence Film, Chris, Include, Reload, Assert. Every other invocation clears it. DebugBreak has an additional gate, described below. None of these callbacks implements a separate server authorization transaction or common rollback layer.

## Ordinary commands: visual settings and display state

The visual flag word at game offset `0x37f06` is distinct from UI-option flags at `0x37f2f` and map visibility flags at `0x14281`. Commands touching similarly named bits do not necessarily have identical cache or persistence effects.

| Command and callback | Literal effect |
| --- | --- |
| Shading, `0x416420` | Toggle visual bit five, flush unit bitmap cache, save settings. |
| AntiAlias, `0x416510` | Toggle visual bit one, flush cache, save settings. |
| Shadow, `0x416550` | Toggle visual bit two, flush cache, save settings. |
| Dither, `0x416590` | Toggle visual bit six and save, with no cache flush here. |
| SwitchAlt, `0x4165c0` | Without an argument toggle visual bit eight and save. With an argument assign its integer value to the one-bit field, without saving in that branch. |
| TShadow, `0x416630` | Toggle vehicle-shadow bit three, without flush/save here. |
| FShadow, `0x416660` | Toggle feature-shadow bit four, without flush/save here. |
| LOSType, `0x416690` | Toggle map flag bit two and recalculate LOS with argument zero, without saving here. |
| Light, `0x4166c0` | Pass three integers, each default zero, to SetLightVector, then flush the unit cache. |
| RCache, `0x416710` | Flush the unit bitmap cache. |
| Gamma, `0x417290` | Set brightness to argument one times 0.1, store the raw integer gamma and save. No clamp is applied. |
| ScrollSpeed, `0x416cf0` | Narrow argument one to a byte, store and save. No minimum is imposed. |
| IFace, `0x416d20` | Store argument one as interface type and save, without range validation. |
| ScreenChat, `0x417130` | XOR screenChat with one and save. This is not cycling every HUD filter mode. |
| Clock, `0x417300` | Toggle UI-option bit six and save. |
| NoShake, `0x416e60` | Toggle UI-option bit four without saving here. |
| ShootAll, `0x418ca0` | Toggle UI-option bit ten; actual firing/target consumers determine its consequences. |
| Drop, `0x4177a0` | Set UI-option bit zero when argument one is zero, clear it otherwise. |
| Sing, `0x4172e0` | Toggle the independent g_cmdSingEnabled global. |

These flags are consumed by [rendering](graphics-model-render.md), [drawing](graphics-draw.md), [line of sight](map-line-of-sight.md), [weapon behavior](weapons-weapons.md) and [settings](game-settings.md). They are not all equivalent to selecting a corresponding options-page stage. In particular the options page's gamma conversion differs from this command's direct tenths conversion, and grouped shadow controls can change several bits at once.

Contour (`0x416db0`) stores spacing as float argument one times 256, default zero, and offset as float argument two times 256, default 0.75. Truncation produces fixed-point integers. Zero spacing disables the overlay's contour call; negative spacing is accepted and can prevent the contour loops from making terminating progress. It is not validated as a positive distance.

Selectable (`0x416460`) walks unit records from index one through unitsEnd inclusive and adds flag `0x20` to records already carrying `0x10000000`. It does not simply select every unit or add the selected-unit bit. Selection eligibility and actual selection storage remain in [selection](ingame-selection.md).

## Ordinary commands: players, resources, audio and diagnostics

Several handlers validate a player after narrowing an integer to an unsigned byte. Their common check requires byte index below ten, active player, type one/two/three and player.index not ten. This rejects many invalid slots but can wrap a large or negative input before checking. Some handlers subsequently reuse the original integer as an array index, making that distinction significant.

Give (`0x416bd0`) takes target player in argument one, integer amount in argument two and resource word in argument three. After the common player check, case-insensitive “metal” calls TransferEnergy and “energy” calls TransferMetal, with localPlayer as source and final flag one. Those helper names are provisional and reversed relative to their actual fields: TransferEnergy spends/adds metal and TransferMetal spends/adds energy. The resource words therefore agree with the downstream resource behavior; the misleading helper names do not establish a reversed-resource gameplay bug. It re-reads the original integer target for the call, without an amount clamp or local success report. [Economy](game-economy.md) and [player services](game-players.md) explain downstream transfer policy.

NoMetal (`0x417150`) and NoEnergy (`0x4171f0`) directly assign a resource float from integer argument two. With only the command token they target localPlayer and use missing argument two's default zero. With any extra token they take the player from argument one. A two-token invocation therefore selects that player and still assigns zero; it does not interpret the lone number as the requested resource amount. These assignments do not call the transfer helpers or implement capacity/negative-value validation here.

Logo (`0x4168d0`) requires a nonnegative logo argument below the first ushort count of logos32, then checks a player from argument two, default zero. On success it writes the logo into that player's data.color and flushes unit cache. The validation uses a narrowed byte but the final array access uses the original integer, so wrapped player inputs are unsafe. Failure posts “Invalid logo setting.” It changes a display/logo field rather than unit ownership.

CDPlay (`0x4167f0`) calls PlayCdTrack with argument one, default zero, and flag one. CDStop (`0x416810`) stops CD playback. Sound3D (`0x416820`) queries and toggles the device's 3D state, saving settings in either branch. MusicMode (`0x4175e0`) passes argument one directly to SetTrackCategory. These commands delegate device/track limits to [sound](sound-sound.md) and [CD audio](sound-cd-audio.md).

BigBrother (`0x4174e0`) toggles autoFollow. Disabling also clears camera-follow state; enabling sets autoFollowTicks to one. It does not immediately choose a target here. NetStats (`0x417570`) calls the no-op FUN_004161f0, stores the current game tick, clears aggregate message/packet counts/bytes and per-kind rows one through 44. It leaves row zero and the separate compression/rate sampling state untouched, as detailed in [network counters](network-net-stats.md).

The ordinary table also registers ShareMetal, ShareEnergy, ShareMapping, ShareRadar, ShareAll, SetShareMetal, SetShareEnergy, ShowRanges, Compression, BPS and SFX. Their bodies belong to [share_commands](game-share-commands.md); they are accounted for there rather than counted as functions in this address interval. That reference covers toggle versus threshold behavior and diagnostic settings. There is no ShareLOS entry in this particular table.

## Cheat table: visibility, economy, capture and meteors

| Command | Behavior |
| --- | --- |
| Radar (`0x417090`) | Toggle UI-option bit nine. |
| ATM (`0x4170c0`) | Add 1000 energy and 1000 metal to playerIndex, the viewed player, which can differ from localPlayer. No storage cap is enforced here. |
| View (`0x416b50`) | After the common narrowed-byte player check, write the original argument into the viewed-player byte only. Control and view can diverge. |
| LOS (`0x416d50`) | Toggle map visibility bit one, save settings and recalculate LOS with argument zero. |
| Mapping (`0x416d80`) | Toggle map bit zero, save and recalculate with argument one. |
| DoubleShot (`0x417030`) | Toggle UI-option bit seven. The name does not mean projectiles are duplicated by this callback. |
| HalfShot (`0x417060`) | Toggle UI-option bit eight. |
| NowISee (`0x417540`) | Clear map bits zero and one, then recalculate with argument one; do not save here. |
| Meteor (`0x417760`) | With an argument, enable or disable periodic meteors according to its integer truth value. With none, start a shower now. |
| MakePoster (`0x417600`) | Capture a bounded map rectangle through WriteScreenshot, as described below. |

Visibility changes interact with [LOS](map-line-of-sight.md), [radar](map-radar.md) and rendering. Radar/DoubleShot/HalfShot callbacks merely write flags, so their downstream behavior must be read in those consumers. [Meteor scheduling and impacts](map-meteors.md) remain separate from the command adapter.

MakePoster defaults to 3200 by 2400. Integer arguments replace width/height; argument one case-insensitive “all” selects map pixel dimensions. Each dimension is raised to at least the visible tile span times sixteen, then lowered to the map dimension. It centers on the current view, clamps origin against the resulting map bounds, creates the screenshots directory below game.path and calls WriteScreenshot with label BIGSHOT. It then resets lastSimBudgetTick from GetTicks, avoiding charging the capture delay as ordinary elapsed frame time. Invalid map/view dimensions are not independently repaired. Screenshot encoding and repeated viewport rendering belong to [screenshots](ingame-screenshots.md).

## Debug table: simulation and mission manipulation

AI (`0x416280`) checks argument one, default localPlayer, with the common player rule. Active type one becomes type two; any other allowed type becomes type one. It delegates the transition to SetType rather than reallocating every AI object here. Control (`0x416ab0`) uses the same validation and writes both localPlayer and playerIndex, unlike View. Neither routine transfers every unit to a new owner.

Assign (`0x416310`) resolves a mission/order type from argument one's text. If found, it issues that type to the selection, using arguments one and two again as integers for the final parameters. Consequently the same first token serves both symbolic lookup and atoi conversion; it is not a conventional separate “order, x, z” grammar. [Order dispatch](orders-order-dispatch.md) owns the actual issue/cancel policy.

Kill (`0x4164b0`) kills all units with only one token, otherwise calls KillPlayerUnits with argument one narrowed by the callee's byte parameter. It then deactivates mission conditions in both cases. It does not mean only the selected unit is killed. IWin (`0x4169d0`) kills player zero if the local player's data.side is one, otherwise player one, then sets endgame bits four, five and two. ILose (`0x416a30`) kills the player indexed by local data.side, clears bit four and sets bits six and two. These use side-derived zero/one assumptions, not a generalized coalition calculation. [Victory](game-victory.md) explains ordinary mission predicates separately.

BurnAll (`0x416390`) removes all features. BurnOne (`0x4163a0`) tests current cellFeature below `0xfffb`, fetches the cursor's map cell and calls RemoveFeature with flag one. Feature (`0x4163d0`) resolves argument one's feature type, rejects `0xffff` and places it at the current cell with null position/activity arguments and owner sentinel ten. They delegate actual reclaim/destruction/placement consequences to [features](map-features.md).

SeaLevel (`0x416a90`) directly narrows argument one to the global sea-level byte; it does not rebuild all water/pathing/terrain caches here. Edge (`0x416730`) sets map pixel width to world width minus argument one (default 32) and pixel height to world height minus argument two (default 128), without bounds checks. Search (`0x416780`) writes pathfinder.value only when argument one is nonzero, so it cannot clear that field through zero. With exactly three tokens it also converts argument two using atof times 65536 into pathfinder.fixed. More than three tokens prevent that second write.

Move (`0x416860`) additionally case-insensitive checks token zero equals “move,” then adds integer arguments one/two shifted left twenty to the current construction/debug position's x/z. World coordinates elsewhere use 16.16, so a unit increment here corresponds to sixteen whole world units. This shifts the stored position, not a ship's velocity or an issued movement goal. Negative/overflowing shifts have no defensive handling.

SelBoxes (`0x416e00`) toggles UI-option bit two; TreeDeath (`0x416e30`) toggles bit three; Profile (`0x417520`) toggles profileBarsEnabled between zero and one. They do not persist settings here. The debug map mode and contour renderer below are distinct from selecting units or enabling profiling.

## Debug table: files, reloads and retained no-op callbacks

MemDump (`0x416240`) creates/truncates memdump.txt and closes it; it writes no memory records. The success test is handle not zero rather than INVALID_HANDLE_VALUE, so failed creation can still cause an invalid CloseHandle attempt. Mem (`0x416270`), Assert (`0x416370`), DPrint (`0x416380`) and ZBuffer (`0x416500`) have empty bodies. Their historical names do not establish hidden diagnostics or a depth-buffer renderer in this executable.

Film (`0x417330`) requires an extra token, unbounded-copies argument one to game.path, strips one trailing slash/backslash, marks imageOutputDirDirty and saves settings. An empty token would index before the string while checking its final character; long input can overflow the provisional path region, which overlaps movieOutputRate in this layout. FilmSpeed (`0x4173e0`) accepts only positive argument one, stores it, marks its dirty field and saves. Frame scheduling/capture behavior belongs to main loop and keyboard consumers, not these setters alone.

Save (`0x417430`) formats savegame plus argument one's name and .sav into a 256-byte stack buffer, creates the directory and calls SaveGameFile with description Generic Game Description and identifier `0x29a`. It does not validate names/path traversal or report the serializer's outcome. The full serializer is documented under [campaign/settings persistence](game-settings-campaign.md); [savegame UI](game-savegame.md) has a separate filename/description workflow.

Reload (`0x417490`) finds the named unit type, rejects zero, kills all units of that type and reloads its definition. It is not a nondestructive model refresh. ReloadAIProfiles (`0x4174d0`) calls ResetAIPlayers. PrintWeights (`0x418bb0`) requires exactly three tokens, validates a player and opens argument two with w+b, delegates to DumpPlayerAI, then closes. It writes an AI diagnostic file, not a complete game save, and reuses the original integer player after narrowed validation.

Senderror (`0x418c70`) requires exactly two tokens. Values outside zero through one hundred become zero; the resulting integer sets netSendDropPct. [Condenser](network-net-condenser.md) describes simulated drops and their successful-return accounting. Merely setting this field does not immediately drop or enqueue a packet.

## Script inclusion and default unit creation

Include (`0x4177e0`) builds debugdat plus token zero and .txt in a 60-byte stack buffer. It therefore ordinarily looks for a file named after Include itself, not the filename in argument one. On an open file it snapshots game.pos, loads all bytes, executes the block with current arguments as substitution variables and mask `0xffffffff`, frees the block, closes and restores position. Other command effects survive. The formatting is unbounded; load failure still closes/restores. This is command text execution, not merely a file dump despite a provisional source comment.

DefaultCommandHandler (`0x417890`) starts at game.pos and visits unit definitions one through count minus one. Token zero is a wildcard pattern against each internal unit name. For every match it optionally offsets x by modelMinX after the first match, snaps to the type footprint and calls CreateUnit with owner argument one (default zero), mode arguments one/one and no specified unit ID. It then advances x by modelMaxX plus `0x200000`; if reaching mapPixelWidth shifted sixteen, it resets x to `0xa00000` and increases z by that amount. It does not check creation success or bound z. This lays out matched definitions, not a production queue, paid shipyard order or construction wireframe.

Only when no definition names matched does it attempt debugdat/token-zero.txt, with the same unbounded 60-byte formatting, all-mask execution, substitutions, memory/file cleanup and position restore as Include. A matched type whose creation fails prevents script fallback because match count still increases. Recursive scripts have no local depth/iteration cap, and restoring position does not undo resource, ownership, flags, created units or nested file effects. [Command tokenization](util-commands.md) supplies line parsing and percent-variable substitution, with no general quoted-string grammar or transaction semantics.

## Intentional crash callback

DebugBreak (`0x417a60`) requires UI-option bit one and endgame bit one. With argument one equal to one it repeatedly allocates 32 MiB through operator new; value two loops through GameAllocIgnoreTag using the FORCE OUT-OF-MEMORY label; value three evaluates division by zero before exit. With missing/zero argument it optionally toggles display mode when windowed, sleeps 500 ms and invokes the debugger breakpoint. Other nonzero values perform no action. These are deliberate failure paths, not normal recoverable error handlers. No command was run for this documentation.

## World projection and selected-goal drawing

WorldToScreenWithHeight (`0x417bb0`) reads whole x/z from 16.16 coordinates, obtains terrain height and projects to x minus scrollX plus 128, z minus scrollY minus half height plus 32. It uses sampled ground height, not simply the position's y component. WorldToScreenTileOffset (`0x417c00`) constructs fixed-point x/z from whole integer arguments and applies that projection; the name does not multiply each argument by tile width.

DrawWorldDeltaLine (`0x417c70`) constructs an endpoint by adding whole dx/dz shifted sixteen, samples/project both endpoints and draws with the low byte of color. DrawWorldSegmentLine (`0x417d30`) starts from a pair of signed shorts, builds fixed-point endpoints from that pair plus deltas and draws identically. Sloping ground can alter the screen-space direction independently of the horizontal world delta.

AiSearchGoal::DrawOnSurface (`0x417e00`) first outlines its owner's tile region, then draws an open polyline through count stored short point pairs. Color comes from game color slot nine when active bit zero is set, otherwise twelve. There is no closing last-to-first segment, point-count clamp or owner-null guard. The goal class's other behavior belongs to [order targets](orders-order-targets.md). DrawSelectedGoal (`0x417f30`) follows three nested pointers only when each is nonnull and calls virtual slot `0x28` with ctx. It does not itself pick the goal or enumerate every selected unit.

## Contour generation

DrawCellContours (`0x4181d0`) computes the screen center as rounded averages of four corner points and its height as the sum of byte corner heights times 64. It splits the quad into four corner-corner-center triangles, sending corner heights scaled by 256. The center's interpolated fractional height is retained rather than first narrowing to a byte.

DrawTriangleContours (`0x417f60`) sorts vertices by height, starts from the highest spacing multiple plus configured offset, decreases until no higher than the maximum, then draws descending levels strictly above the middle height and strictly above the minimum. Each segment interpolates one endpoint on the long edge and the other on the appropriate short edge using integer arithmetic. Palette selection indexes g_contourColors by the level's whole height relative to sea level, biased 256 and divided by sixteen.

The strict comparisons avoid ordinary flat-edge zero denominators when spacing is positive; they do not supply general validation of spacing, arithmetic overflow or color-table index. Calling the triangle helper directly with spacing zero divides by zero. Negative spacing can make descending loops nonterminating. Contours are terrain diagnostics, separate from ship construction mesh/wireframe drawing and weapon targeting ranges.

## Map debug overlay and its limits

DrawMapDebugOverlay (`0x418310`) returns only when debugMode and contour spacing are both zero. It uses playerIndex's player/fog. The first cell is scroll divided by sixteen, horizontal end is min(viewWidthTiles plus firstX plus one, mapWidthTiles minus one); rows run to mapHeightTiles minus one until the offscreen break. It forms four projected corners from neighboring tile heights, reserving the final row/column for safe neighbor access under valid scroll/map inputs. It does not independently clamp negative starting coordinates.

| Mode | Output and data source |
| --- | --- |
| One | Uses the first selected unit's movement class if present. Extracts the two-bit state for each tile from the class's packed column/row-group words; states below three draw colored diagonals. Path-grid flag four prints G in a random text color. Other eligible path flags draw direction arrows from g_dirDeltaX/Z. |
| Two | Draws top/left cell edges, using one color above sea level and another at/below it. Unit occupancy fills the quad using unit ID's low byte; otherwise non-`0xffff` object occupancy uses object minus 56's low byte. Feature IDs draw diagonals; tile flag two adds an inset outline. |
| Three | Draws the same land/water edges and prints the tile's metal byte in decimal. |
| Four | Draws edges in color slot zero and a ten-pixel square when the viewed player's half-resolution fog cell is nonzero. |
| Other nonzero | No mode-specific marks, but configured contours still render. |

Contours are drawn after mode-specific output for every visited cell. There is no complete viewport culling per polygon here. The row break checks only whether any first corner was above screenHeight, not all four corners or the top boundary; absent horizontal cells can cause an immediate break. Drawing primitives provide their own clipping policies.

Path-arrow color is assigned only for full flags one or two. Flags five/six pass the masked eligibility test but do not assign a new color; they can reuse the previous arrow's value or encounter it uninitialized. Direction indexing is also trusted. Debug G rendering calls rand and changes font/text colors; this is diagnostic rendering with side effects, not a pure snapshot of gameplay data. Neither the overlay nor these command callbacks serialize themselves as a network protocol. Actual movement, fire decisions, unit creation and save persistence are delegated to the linked systems.

## Coverage boundary

All 83 owned function annotations, all three registration tables and their direct initialization/chat/keyboard callers have been traced. Handler-only no-ops, exact token positions, partial player validation, unchecked file formatting, direct flag writes and destructive failure callbacks are explicitly retained as observed behavior. Asset/device outcomes and downstream gameplay policies remain the responsibility of their referenced modules. No builds, dependency installations, source modifications or source listings were used.
