# Doc writeup roadmap

Last updated: 2026-10-17. Source baseline: `97d7ac59b460417bcdfa629cf90da99f31818c65`.

This is the completion tracker for the [systems manual](systems/index.md). It counts the 110 distinct folder/module pairs in `data/modules.csv`, combining repeated address ranges for the same pair. The [module inventory](systems/module-inventory.md) provides their address ranges and chapter assignments.

## Current progress

| Measure | Count |
| --- | ---: |
| Module groups in scope | 110 |
| Finished module writeups | 110 |
| Partially written module groups | 0 |
| Module groups not yet written up | 0 |
| Module groups remaining to finish | 0 |
| Finished coverage | 100.00% |

Three substantive general chapters and one naval case study have been written. Their combined coverage spans parts of several modules. Those chapters remain drafts. Separate reviewed module writeups cover the completed groups listed below, including world foundations, campaign selection, players, unit definitions/lifecycle, squads, references/targets/damage, callbacks/cache allocation, persistence, mission/ownership commands, positioning/replication and order infrastructure. Listing every module in an index does not count as documenting it.

## Status rules

- **Not started:** no substantive behavioral writeup yet. A roadmap entry, incidental mention, or source link alone does not change this status.
- **Partial:** some behavior is explained with implementation evidence, but significant responsibilities or interactions remain unwritten.
- **Finished:** the module has a reviewed writeup covering its major responsibilities, inputs, state, execution, outputs, failure paths, dependencies, and persistence/networking where applicable. Implementation references have been checked. Any remaining asset-dependent details or unresolved points are identified explicitly.

A finished module writeup does not require copying every function into prose. It does require accounting for every major functional area in that module; describing one naval use or one caller is insufficient.

## Required update after each finished module

1. Update that module row to **Finished**, link the completed writeup, and enter its completion date.
2. Recalculate the finished, partial, not-started, remaining, and percentage totals above. Count each folder/module pair once.
3. Add a dated completion-log entry identifying the module and the new finished count out of 110.
4. Update chapter status in the manual index when appropriate and validate the new document links.
5. Make these changes in the same documentation edit as the completed writeup. Update this file for new partial coverage as well.

The [saved workflow and method](doc-writeup-workflow.md) explains source tracing, completion criteria, auditing and continuation.

## Writing order

Finish the foundations, then work through world/terrain/features, players and units, orders, construction and service interactions, movement, aircraft, scripts, combat, sight, rendering, interface, AI, networking, missions/victory, saves, audio, and runtime support. The manual index supplies the detailed chapter plan.

## Module checklist

| Module group | Status | Existing writeup | Planned chapter(s) | Finished date |
| --- | --- | --- | --- | --- |
| ai/ai_player | Finished | [AI player, production and squads](systems/modules/ai-ai-player.md) | 15. Computer-player behavior | 2026-10-16 |
| ai/ai_profile | Finished | [AI profile tokenizer](systems/modules/ai-ai-profile.md) | 15. Computer-player behavior | 2026-10-16 |
| ai/pathfind | Finished | [Route search and scheduling](systems/modules/ai-pathfind.md) | 08. Ground movement and pathfinding | 2026-10-16 |
| debug/debug_helper | Finished | [Optional DebugHelper value adapter](systems/modules/debug-debug-helper.md) | 20. Diagnostics | 2026-10-17 |
| debug/debug_lib | Finished | [reference](systems/modules/debug-debug-lib.md) | 20. Diagnostics | 2026-10-17 |
| debug/memory_stats | Finished | [Movie statistics and summary display](systems/modules/debug-memory-stats.md) | 20. Diagnostics | 2026-10-16 |
| frontend/campaign_menu | Finished | [Campaign menus and briefing artwork](systems/modules/frontend-campaign-menu.md) | 17. Campaign selection | 2026-10-17 |
| frontend/endgame | Finished | [Endgame snapshots, fades and outcome controls](systems/modules/frontend-endgame.md) | 17. Endgame | 2026-10-17 |
| frontend/frontend | Finished | [Frontend states, menu and picture cache](systems/modules/frontend-frontend.md) | 14. Frontend states | 2026-10-17 |
| frontend/multi | Finished | [reference](systems/modules/frontend-multi.md) | 16. Multiplayer setup | 2026-10-17 |
| frontend/options | Finished | [reference](systems/modules/frontend-options.md) | 14, 19. Options | 2026-10-17 |
| frontend/skirmish_menu | Finished | [Skirmish setup, maps and launch checks](systems/modules/frontend-skirmish-menu.md) | 14, 15. Skirmish setup | 2026-10-17 |
| game/campaign | Finished | [campaign sequences](systems/modules/game-campaign.md) | 17. Missions and campaigns | 2026-10-16 |
| game/cd_check | Finished | [Archive mounting and content gates](systems/modules/game-cd-check.md) | 01. Archives and content checks | 2026-10-17 |
| game/console_commands | Finished | [reference](systems/modules/game-console-commands.md) | 20. Console and diagnostics | 2026-10-17 |
| game/data_files | Finished | [Asset paths and shared resources](systems/modules/game-data-files.md) | 01, 13. Asset resolution and visual resources | 2026-10-17 |
| game/economy | Finished | [account lifecycle](systems/modules/game-economy.md), [03](systems/03-economy.md) | 03. Economy | 2026-10-16 |
| game/game_load | Finished | [Match loading and worker handoff](systems/modules/game-game-load.md) | 01. Match loading | 2026-10-17 |
| game/game_state | Finished | [Lifetimes, wind, speed and interface state](systems/modules/game-game-state.md) | 01, 02. Lifecycle and environment | 2026-10-17 |
| game/main | Finished | [Process entry and command line](systems/modules/game-main.md) | 01, 02. Startup and frame dispatch | 2026-10-17 |
| game/main_loop | Finished | [Battle frames and setup transitions](systems/modules/game-main-loop.md) | 02. Time and simulation | 2026-10-17 |
| game/players | Finished | [players](systems/modules/game-players.md), [02](systems/02-time-simulation.md), [03](systems/03-economy.md), [naval](naval-systems.md) | 05, 16, 18. Players and persistence | 2026-10-16 |
| game/savegame | Finished | [Save/load UI and restore handoff](systems/modules/game-savegame.md) | 18. Save and restore | 2026-10-17 |
| game/settings | Finished | [Persistent options and unit restrictions](systems/modules/game-settings.md) | 01, 14. Configuration | 2026-10-16 |
| game/settings_campaign | Finished | [Save orchestration and campaign settings](systems/modules/game-settings-campaign.md) | 01, 17. Campaign configuration | 2026-10-16 |
| game/share_commands | Finished | [Sharing preferences and diagnostic toggles](systems/modules/game-share-commands.md) | 03, 12, 16. Resource and information sharing | 2026-10-17 |
| game/side_data | Finished | [Side definitions and interface geometry](systems/modules/game-side-data.md) | 01, 05. Side definitions | 2026-10-17 |
| game/statistics | Finished | [Score tables and reporter DLL lifetime](systems/modules/game-statistics.md) | 17. Statistics | 2026-10-17 |
| game/victory | Finished | [Mission conditions and replicated goals](systems/modules/game-victory.md) | 17. Victory conditions | 2026-10-17 |
| graphics/blit | Finished | [Assembly blitters and indexed pixel operations](systems/modules/graphics-blit.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/display | Finished | [Display environment, messages and timers](systems/modules/graphics-display.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/draw | Finished | [Primitives and polygon spans](systems/modules/graphics-draw.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/font | Finished | [Bitmap text metrics and drawing](systems/modules/graphics-font.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/gaf | Finished | [GAF frames and sprite services](systems/modules/graphics-gaf.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/model_render | Finished | [Model state and construction rendering](systems/modules/graphics-model-render.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/movies | Finished | [Smacker movie playback and output ownership](systems/modules/graphics-movies.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/palette | Finished | [Indexed palettes and transforms](systems/modules/graphics-palette.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/palette_tables | Finished | [Palette transform caches](systems/modules/graphics-palette-tables.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/particles | Finished | [Particle pools and six effect families](systems/modules/graphics-particles.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/pcx | Finished | [PCX/BMP, screenshots and 3DO helpers](systems/modules/graphics-pcx.md) | 13. Rendering and visual effects | 2026-10-16 |
| graphics/surface | Finished | [Surfaces, presentation and texture mapping](systems/modules/graphics-surface.md) | 13. Rendering and visual effects | 2026-10-16 |
| gui/dialogs | Finished | [Dialog layers, editing and palette tools](systems/modules/gui-dialogs.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/file_list | Finished | [Directory lists and blinking text](systems/modules/gui-file-list.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/gui | Finished | [GUI contexts, layers and widgets](systems/modules/gui-gui.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/gui_file | Finished | [GUI record files and GAF ownership](systems/modules/gui-gui-file.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/keyboard | Finished | [Keyboard translation and queue](systems/modules/gui-keyboard.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/listbox | Finished | [Listbox skins and bevel primitives](systems/modules/gui-listbox.md) | 14. Interface infrastructure | 2026-10-17 |
| gui/mouse | Finished | [Software cursor and mouse-event queue](systems/modules/gui-mouse.md) | 14. Interface infrastructure | 2026-10-17 |
| ingame/build_placement | Finished | [Battle input and placement routing](systems/modules/ingame-build-placement.md) | 07, 14. Placement and input | 2026-10-16 |
| ingame/camera | Finished | [Camera centering, bookmarks and persistence](systems/modules/ingame-camera.md) | 14. Camera | 2026-10-17 |
| ingame/control_panel | Finished | [Production, work and camera services](systems/modules/ingame-control-panel.md) | 07, 14. Production and controls | 2026-10-16 |
| ingame/dialogs | Finished | [Dialogs, score, music and scheduling](systems/modules/ingame-dialogs.md) | 02, 14. Simulation scheduling and dialogs | 2026-10-17 |
| ingame/info_panel | Finished | [Sensors, battle display and panels](systems/modules/ingame-info-panel.md) | 12, 13, 14. Sensors and battle display | 2026-10-16 |
| ingame/keys | Finished | [Battle keys and bitmap mosaics](systems/modules/ingame-keys.md) | 14. Game keys | 2026-10-17 |
| ingame/markers | Finished | [Footprints, paths and range overlays](systems/modules/ingame-markers.md) | 12, 14. World overlays | 2026-10-17 |
| ingame/screenshots | Finished | [Application-frame input and dispatch](systems/modules/ingame-screenshots.md) | 02, 14. Frame dispatch and capture | 2026-10-17 |
| ingame/selection | Finished | [Selection, picking and command distribution](systems/modules/ingame-selection.md) | 14. Selection | 2026-10-17 |
| map/features | Finished | [features and wrecks](systems/modules/map-features.md), [naval](naval-systems.md) | 04. Features | 2026-10-16 |
| map/line_of_sight | Finished | [Terrain, sight and fog](systems/modules/map-line-of-sight.md) | 04, 12. Map loading and sight | 2026-10-16 |
| map/los_tables | Finished | [Sight ray tables and support](systems/modules/map-los-tables.md) | 12. Sight tables | 2026-10-16 |
| map/map_list | Finished | [map enumeration](systems/modules/map-map-list.md) | 04, 14. Map metadata and selection | 2026-10-16 |
| map/map_load | Finished | [mission loading](systems/modules/map-map-load.md) | 04. Map support | 2026-10-16 |
| map/meteors | Finished | [Meteor events](systems/modules/map-meteors.md) | 04. Environment | 2026-10-16 |
| map/plot_map | Finished | [Plot-map persistence](systems/modules/map-plot-map.md) | 04, 18. Map persistence | 2026-10-16 |
| map/radar | Finished | [Minimap and contacts](systems/modules/map-radar.md) | 12, 14. Radar and minimap | 2026-10-16 |
| map/terrain | Finished | [terrain](systems/modules/map-terrain.md), [naval](naval-systems.md) | 04, 08. Terrain and occupancy | 2026-10-16 |
| network/hapinet | Finished | [DirectPlay transport, sessions and lobby](systems/modules/network-hapinet.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/net_chat | Finished | [Message HUD, chat and unit jumps](systems/modules/network-net-chat.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/net_condenser | Finished | [Payload compression and receive cache](systems/modules/network-net-condenser.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/net_game | Finished | [reference](systems/modules/network-net-game.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/net_stats | Finished | [Bit streams and traffic counters](systems/modules/network-net-stats.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/online | Finished | [Online service DLL and configuration](systems/modules/network-online.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/packets | Finished | [Packet channels and receive-frame queues](systems/modules/network-packets.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| network/unit_sync | Finished | [Pregame unit catalogue and restrictions](systems/modules/network-unit-sync.md) | 16. Multiplayer and synchronization | 2026-10-17 |
| orders/order_dispatch | Finished | [command resolution](systems/modules/orders-order-dispatch.md), [naval](naval-systems.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| orders/order_list | Finished | [order execution and persistence](systems/modules/orders-order-list.md), [naval](naval-systems.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| orders/order_queue | Finished | [order helpers](systems/modules/orders-order-queue.md), [naval](naval-systems.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| orders/order_targets | Finished | [Order destinations and path goals](systems/modules/orders-order-targets.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| orders/unit_orders | Finished | [Ground and common actions](systems/modules/orders-unit-orders.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| orders/vtol_orders | Finished | [Aircraft actions and service](systems/modules/orders-vtol-orders.md) | 06, 07, 08, 09. Orders and movement | 2026-10-16 |
| runtime/basic_string | Finished | [Runtime string buffer copying and detachment](systems/modules/runtime-basic-string.md) | 20. Runtime support | 2026-10-16 |
| sound/cd_audio | Finished | [CD audio track policy, categories and fades](systems/modules/sound-cd-audio.md) | 19. Audio | 2026-10-16 |
| sound/play_sound | Finished | [Windows waveform playback fallback](systems/modules/sound-play-sound.md) | 19. Audio | 2026-10-16 |
| sound/samples | Finished | [DirectSound samples, streaming and mixer state](systems/modules/sound-samples.md) | 19. Audio | 2026-10-16 |
| sound/sound | Finished | [Game sound routing and unit speech](systems/modules/sound-sound.md) | 19. Audio | 2026-10-16 |
| sound/sound_types | Finished | [Sound definitions and speech choices](systems/modules/sound-sound-types.md) | 19. Audio | 2026-10-16 |
| units/cob | Finished | [COB execution](systems/modules/units-cob.md) | 10. COB execution | 2026-10-16 |
| units/movement_class | Finished | [movement classes](systems/modules/units-movement-class.md), [naval](naval-systems.md) | 08. Navigation | 2026-10-16 |
| units/squads | Finished | [squads](systems/modules/units-squads.md) | 05, 15. Squads | 2026-10-16 |
| units/unit_commands | Finished | [mission and ownership commands](systems/modules/units-unit-commands.md) | 05, 06. Unit commands | 2026-10-16 |
| units/unit_motion | Finished | [callbacks and cache](systems/modules/units-unit-motion.md), [03](systems/03-economy.md) | 03, 13. Extraction and rendering cache | 2026-10-16 |
| units/unit_position | Finished | [positioning and replication](systems/modules/units-unit-position.md), [02](systems/02-time-simulation.md), [naval](naval-systems.md) | 02, 05, 08, 16. Unit updates and synchronization | 2026-10-16 |
| units/unit_save | Finished | [unit persistence](systems/modules/units-unit-save.md) | 18. Unit persistence | 2026-10-16 |
| units/unit_script_calls | Finished | [COB unit interface](systems/modules/units-unit-script-calls.md), [naval](naval-systems.md) | 10. Script interfaces | 2026-10-16 |
| units/unit_scripts | Finished | [steering and script geometry](systems/modules/units-unit-scripts.md), [naval](naval-systems.md) | 07, 10. Script callbacks and model queries | 2026-10-16 |
| units/unit_targets | Finished | [references, targets and damage](systems/modules/units-unit-targets.md), [naval](naval-systems.md) | 05, 11. References and combat state | 2026-10-16 |
| units/unit_types | Finished | [definition loading](systems/modules/units-unit-types.md), [01](systems/01-startup-assets-lifecycle.md), [naval](naval-systems.md) | 01, 05. Definition loading | 2026-10-16 |
| units/units | Finished | [unit lifecycle](systems/modules/units-units.md), [naval](naval-systems.md) | 05, 08. Unit and terrain helpers | 2026-10-16 |
| util/commands | Finished | [Command parser, registration and dispatch](systems/modules/util-commands.md) | 20. Command infrastructure | 2026-10-16 |
| util/hapibank | Finished | [HapiBank accounts, boxes and file format](systems/modules/util-hapibank.md) | 18. Structured persistence | 2026-10-16 |
| util/hpi | Finished | [HPI archives, files and discovery](systems/modules/util-hpi.md) | 01, 20. Archive and file services | 2026-10-17 |
| util/int_map | Finished | [COB checksum cache and integer tree](systems/modules/util-int-map.md) | 20. Containers | 2026-10-16 |
| util/math | Finished | [Checksums, random state and angle arithmetic](systems/modules/util-math.md) | 08, 11, 20. Math support | 2026-10-16 |
| util/operator_new | Finished | [Global C++ allocation adapters](systems/modules/util-operator-new.md) | 20. Allocation | 2026-10-16 |
| util/registry | Finished | [Registry and environment utilities](systems/modules/util-registry.md) | 01, 20. Settings storage | 2026-10-16 |
| util/sqsh | Finished | [SQSH chunks, LZSS and compression contracts](systems/modules/util-sqsh.md) | 20. Compression | 2026-10-17 |
| util/tdf | Finished | [TDF parsing, records and translations](systems/modules/util-tdf.md) | 01. Structured text | 2026-10-16 |
| weapons/explosions | Finished | [Explosion effects and debris](systems/modules/weapons-explosions.md) | 11. Combat | 2026-10-16 |
| weapons/weapon_types | Finished | [Weapon definitions and assets](systems/modules/weapons-weapon-types.md) | 11. Combat | 2026-10-16 |
| weapons/weapons | Finished | [Live weapons and projectiles](systems/modules/weapons-weapons.md) | 11. Combat | 2026-10-16 |

## Completion log

| Date | Module group | Change | Finished total |
| --- | --- | --- | ---: |
| 2026-10-16 | All | Created tracker and audited existing drafts; 40 partial, 70 not started | 0 / 110 |
| 2026-10-16 | map/plot_map | Completed all six persistence entry points, data contracts, read/write guards and asymmetries | 1 / 110 |
| 2026-10-16 | map/meteors | Completed configuration, scheduling, projectile handoff, save/load and static support | 2 / 110 |
| 2026-10-16 | map/terrain | Completed occupancy, placement, yard state, passability, spatial visitors and resource sampling across 26 entry points | 3 / 110 |
| 2026-10-16 | map/map_list | Completed OTA enumeration, cache ownership, mission lifetime and static checksum-cache support | 4 / 110 |
| 2026-10-16 | game/campaign | Completed campaign sequences, mission selection, localized paths, briefings and metadata access | 5 / 110 |
| 2026-10-16 | map/map_load | Completed OTA/header loading, schema choice, initial objects, start positions, checksum and cache support | 6 / 110 |
| 2026-10-16 | map/features | Completed definitions, footprints, spots, wreck motion, reclaim, fire, authority and save/restore | 7 / 110 |
| 2026-10-16 | units/units | Completed slot allocation, initialization, creation, death, cargo cleanup, corpses and terrain helpers | 8 / 110 |
| 2026-10-16 | units/squads | Completed allocation, membership consistency, order dispatch and restoration boundaries | 9 / 110 |
| 2026-10-16 | units/unit_targets | Completed listeners, target encodings, aiming, damage/heal/paralyze, property formatting and alignment | 10 / 110 |
| 2026-10-16 | units/unit_motion | Completed extraction/wind callbacks and all arena, bitmap, eviction and handle-lifetime operations | 11 / 110 |
| 2026-10-16 | units/unit_save | Completed records, recursive relationships, component/order restoration and mission-unit lookup | 12 / 110 |
| 2026-10-16 | game/players | Completed setup, runtime services, update gates, resources, conditions/watch mode, visibility and persistence | 13 / 110 |
| 2026-10-16 | units/unit_commands | Completed initial mission text, creation, ownership handoff, category masks, lookup and sort support | 14 / 110 |
| 2026-10-16 | Completed groups audit | Enumerated annotations by module address range across src; expanded unit_targets with six shared-file methods, campaign with its separate probe and unit_commands with category population | 14 / 110 |
| 2026-10-16 | units/unit_position | Completed positioning/height, attachment, activation, per-unit tick order and bitstream state replication | 15 / 110 |
| 2026-10-16 | units/unit_types | Completed discovery/compatibility, FBI field/flag interpretation, yard maps, assets, build menus and teardown; corrected provisional flag names in prior lifecycle/height docs | 16 / 110 |
| 2026-10-16 | orders/order_queue | Completed carried orders, build-script calls/waits, reclaim pulses, type access, status flags and goal ownership | 17 / 110 |
| 2026-10-16 | orders/order_list | Completed all 49 entry points across queue policy, execution results, cleanup, registry, save/restore, overlays and adjacent motion helpers | 18 / 110 |
| 2026-10-16 | orders/order_dispatch | Completed every command mode, contextual priority, surface/air/water attack selection and service/feature eligibility | 19 / 110 |
| 2026-10-16 | units/movement_class | Completed class parameters, packed pass maps, footprint/edge grading, dynamic refresh, lifecycle and adjacent helpers | 20 / 110 |
| 2026-10-16 | units/unit_scripts | Completed ground/air steering, motion lifecycle/save, carried position, script motion/SFX state, piece geometry and command cursors | 21 / 110 |
| 2026-10-16 | units/unit_script_calls | Completed all unit-value properties, piece/cache mutations, scripted wake/thrust/smoke/bubbles, explosions, transports, aim return and grid helpers | 22 / 110 |
| 2026-10-16 | units/cob | Completed channels, all decoded opcode families, waits/signals/callbacks, piece integration and checksum-guarded save/restore | 23 / 110 |
| 2026-10-16 | game/economy | Completed account lifecycle/persistence/support-function companion to the detailed production, settlement and shortage chapter | 24 / 110 |
| 2026-10-16 | orders/order_targets | Completed all 111 entry points: area geometry, linked/fixed targets, aircraft maneuvers, arrival/repath policy, waypoint replication and persistence | 25 / 110 |
| 2026-10-16 | orders/unit_orders | Completed all 50 functions: common/ground actions, factory/mobile construction, assistance, capture, reclaim, resurrection, repair, combat, guard, transport and registration | 26 / 110 |
| 2026-10-16 | orders/vtol_orders | Completed all 30 functions and 22 aircraft actions: terrain/pad landing, air transport, attack families, guard/patrol, construction, repair and reclaim | 27 / 110 |
| 2026-10-16 | ai/pathfind | Completed passability/knowledge, probe, weighted expansion, heap/pool storage, bounded traceback, adaptive heuristic and shared scheduler accounting | 28 / 110 |
| 2026-10-16 | ingame/control_panel | Completed production UI, selection aggregation, paging, placement intent, build/repair/decay/completion and all owned camera services | 29 / 110 |
| 2026-10-16 | ingame/build_placement | Completed all four functions: left/right intent routing, retained placement, drag/radar priority, battle tick and endgame/restart transitions | 30 / 110 |
| 2026-10-16 | weapons/weapon_types | Completed all three functions: 256-slot discovery, every numeric/flag field, model sharing, effect/sound binding, damage overrides and teardown | 31 / 110 |
| 2026-10-16 | All 110 groups | Audited 110 unique rows against 31 substantive module references (850 to 3764 words), 20 partial groups and 59 not-started groups; broad chapters remain drafts | 31 / 110 |
| 2026-10-16 | weapons/weapons | Completed all 52 functions: aim/reach/fire/reload, projectile families/bursts, collision, direct/area/interceptor damage, replication, compaction and all eight render styles | 32 / 110 |
| 2026-10-16 | weapons/explosions | Completed all 18 functions: procedural frames, finite effect/piece pools, whole-piece and face-shard physics/rendering, teardown and feature lookup | 33 / 110 |
| 2026-10-16 | Documentation workflow | Saved source-tracing method, completion gates, audit procedure, progress maintenance and continuation checklist; workflow is not an additional module | 33 / 110 |
| 2026-10-16 | map/los_tables | Completed all 50 entries: ray-table TDF/schema, quadrant transforms, accessor contracts, nested storage, string-pair helpers and unit-definition sort phases | 34 / 110 |
| 2026-10-16 | map/line_of_sight | Completed all 29 functions: TNT loading/environment, terrain/derived grids, current/permanent sight, ray/mask updates, temporary eyes, tiles/fog and projected picking | 35 / 110 |
| 2026-10-16 | map/radar | Completed all eight functions: minimap pictures/surfaces, current/mapped shading, unit/projectile symbols, range overlays, picking and camera rectangle | 36 / 110 |
| 2026-10-16 | ingame/info_panel | Completed all 29 entries: radar/sonar/jam/cloak contacts, battle layering, resource/unit/feature HUD, model/outline helpers, status/messages and probes | 37 / 110 |
| 2026-10-16 | ai/ai_profile | Completed all nine functions: caller-owned buffer, token priority/types/limits, integer/error/pending contracts and unit-definition protection adapters | 38 / 110 |
| 2026-10-16 | ai/ai_player | Completed all 115 entries: policy commands, economic production, placement, caches, squad timers, automatic targeting, retaliation, lifecycle and container support | 39 / 110 |
| 2026-10-16 | graphics/palette | Completed all 32 entries: palette installation/readback/brightness, five indexed transforms, allocation/copy ownership, matching and filename helpers | 40 / 110 |
| 2026-10-16 | graphics/palette_tables | Completed all five wrappers: fixed cache paths/sizes, load/generate branches, copied versus dangling returns, persistence failures and gray/blue generation | 41 / 110 |
| 2026-10-16 | graphics/model_render | Completed all 35 entries: runtime piece trees/posing, cache paths, lit/ordinary polygons, construction bands/edges, attached-product depth composition, water/cloak, shadows and support copies | 42 / 110 |
| 2026-10-16 | graphics/gaf | Completed all 38 entries: loading/fixups, sequence references/timing, draw trees/effects, clipping/scaling, raw/depth/lens frames, water/cutouts and row compression | 43 / 110 |
| 2026-10-16 | Coverage audit | Reconciled all 110 unique CSV keys and roadmap rows against 43 substantive module references; inspected scope/section coverage and behavioral samples, retained 14 Partial and 53 Not started; corrected stale chapter summaries without promoting draft coverage | 43 / 110 |
| 2026-10-16 | graphics/draw | Completed all 37 entries: clipping, lines/rectangles/circles, remap effects, convex scan conversion, flat/shaded depth spans, construction endpoints, containment and text-style accessors | 44 / 110 |
| 2026-10-16 | graphics/particles | Completed all 127 entries: 1000-slot emitter pool, ten layers, six family lifetimes/emission/movement/rendering, nanos/wakes/bubbles, sight tests, smoke variants and container support | 45 / 110 |
| 2026-10-16 | graphics/surface | Completed all 51 entries: storage/save boxes, screen locking/presentation/restoration/cursor, affine textured depth/light polygons, edge interpolation and reference-counted StringRef | 46 / 110 |
| 2026-10-16 | graphics/font | Completed all four entries: byte-glyph metrics, first-line fitting, containment, centered/outlined text, global style side effects and glyph rasterizer contract | 47 / 110 |
| 2026-10-16 | graphics/pcx | Completed all 21 entries: packed PCX RLE/palettes, image ownership/failures, screenshot numbering, BMP headers/rows, 3DO relocation/sorting/bounds and exception display cleanup | 48 / 110 |
| 2026-10-16 | graphics/blit | Completed all 28 callable entries: dimensional clipping, keyed/rectangle/tile copies, four compressed sprite decoders, blend/shadow/mask transforms, normal/remapped/legacy lines, fills, glyphs and fixed-point sampling; documented actual descriptor/caller discrepancies | 49 / 110 |
| 2026-10-16 | graphics/display | Completed all 30 entries: context/options, DirectX comparison, mode discovery, GDI/DirectDraw recreation and partial failures, Windows input/palette messages, cooperative timers and count reuse, frame-rate side effects and teardown | 50 / 110 |
| 2026-10-16 | graphics/movies | Completed all 12 owned entries: Smacker setup/close, borrowed/private DirectDraw state, palette/format limits, real offscreen pitch/pixel arguments, decode/presentation, repaint, focus/input/end/loop behavior and build-site getters | 51 / 110 |
| 2026-10-16 | debug/memory_stats | Completed both entries across debug and movies: decoder-summary fields, stats.txt arithmetic/overwrite behavior, fixed-buffer message display, unclosed input stream and absent demonstrated live callers | 52 / 110 |
| 2026-10-16 | sound/play_sound | Completed all six entries: constant availability, asynchronous borrowed waveform images, loop versus no-stop resume, disk ownership/replacement, stop cleanup and return declaration mismatch | 53 / 110 |
| 2026-10-16 | sound/samples | Completed all 51 entries: DirectSound lifecycle, four-voice sets, 32-channel tracking/eviction, eight temporary sets, streaming halves/delayed requests/EOF, raw/DIGI/WAV chunks, mixer/3D settings, stubs and compression initialization | 54 / 110 |
| 2026-10-16 | sound/cd_audio | Completed all 34 entries: MCI/disc discovery, logical/physical tracks, orders/category search, fades/delayed tracks, notification routing, pause/stop/enable, external player and activity thresholds | 55 / 110 |
| 2026-10-16 | sound/sound_types | Completed all six entries: effect/catalog versus speech-category assets, 23 message keys, parallel 64-byte alternatives/captions, suffix-gap rules, shared aliases, registry/unit binding and allocation/cleanup limits | 56 / 110 |
| 2026-10-16 | sound/sound | Completed all 31 entries: backend/lifecycle, named/positional/fog/3D sound, actual network flag behavior, 23 message kinds, queue/priority/cooldown/text rules, death cleanup and adjacent unordered containers/squad constructor | 57 / 110 |
| 2026-10-16 | util/registry | Completed all 16 entries: HKCU namespace/access/partial reads, typed write adapters, inclusive rectangle tests, byte/name/text helpers, CRT thread launch, sleep and media callback storage | 58 / 110 |
| 2026-10-16 | game/settings | Completed all six entries: complete option/default/writeback key families, audio side effects, name overrides/capture dirtiness, skirmish fields and count weakness, write asymmetries, track bytes and use-only availability restrictions | 59 / 110 |
| 2026-10-16 | game/settings_campaign | Completed all nine entries: bank ownership/open filters, complete state load/save order and Summary schema, live versus advanced-mission images, campaign byte strings and explicit flag/player-count registry saves | 60 / 110 |
| 2026-10-16 | util/operator_new | Completed all four scalar/array allocation/free adapters and traced allocator lock/retry/debug-fill/null-free dependencies without promoting the larger diagnostic heap module | 61 / 110 |
| 2026-10-16 | util/int_map | Completed all 20 entries: COB raw-image checksum/address registry, in-place relocation, missing-key insertion/free, generated map lifetime, fourteen tree members traced in existing MAP/XTREE headers and adjacent InitBank | 62 / 110 |
| 2026-10-16 | util/hapibank | Completed all 25 owned entries: hierarchy/type/box ownership, cursor/nontruncation contracts, exact file/account records and string pool, compression/load filters, failure/nontransactional behavior and audits | 63 / 110 |
| 2026-10-16 | util/math | Completed all 25 entries: raw-byte checksum, separate RNG state/advancement, vector algebra, actual 512-sample lookup indexing, scaled trig/multiply/divide, actual FPATAN orientation, pair rotations and shared matrix contracts | 64 / 110 |
| 2026-10-16 | util/commands | Completed all 21 entries: token/access grammar, buffer/ShiftArgs limits, borrowed percent substitutions, sorted registration and case mismatch, context/default dispatch, line execution and emitted vector/name lifetime | 65 / 110 |
| 2026-10-16 | util/tdf | Completed all 62 entries across two ranges/seven files: parser/comment/tree/load failures, navigation/typed getters/checksums, case/order rules, translation caching/reverse/localized reads and generated field/vector/string/exception support | 66 / 110 |
| 2026-10-16 | runtime/basic_string | Completed both owned entries: existing XSTRING buffer/reference representation, rounded capacity and exact-size exception retry, content-preserving replacement and shared-storage detachment | 67 / 110 |
| 2026-10-17 | util/sqsh | Completed all 22 game-source entries and linked zlib contracts: ring/tree/preset/lock lifetime, exact LZSS tokens/final-group behavior, SQSH schema, buffer/error/decryption/checksum limits and existing zlib 1.0.4 wrappers/algorithm | 68 / 110 |
| 2026-10-17 | util/hpi | Completed 58 owned entries: loose/archive precedence, shared handles/cursors, block decoding, load ownership and partial reads, discovery/wildcards, metadata relocation, shadows, package writing, drive/path helpers and failure limits | 69 / 110 |
| 2026-10-17 | game/data_files | Completed all 22 entries: language paths/version warning, bitmap/font/palette/radar/progress adapters, sound and GAF caches, complete resource names, side panels, texture/model binding, lifetimes, checksums and generated TDF deletion | 70 / 110 |
| 2026-10-17 | game/cd_check | Completed six entries: mount order/limits/retention, disc TDF identification, bypass controls, alternate path branch, compatibility getters and interior-pointer zero allocation | 71 / 110 |
| 2026-10-17 | game/side_data | Completed five entries: contiguous faction discovery, scalars, all 30 rectangle records/cursor restoration, font/logo lifetimes and five-slot boundary behavior | 72 / 110 |
| 2026-10-17 | game/main_loop | Completed ten entries: profiling/frame gates, paused networking, selection/drawing/capture, menu/loading callback transitions, mission bits, prefix resource maxima and commander/camera setup | 73 / 110 |
| 2026-10-17 | game/game_load | Completed seven entries: worker/main-thread flags and waits, settings/host/save branches, placement/HUD finalization, loading bars/sync and transition, three coordinate helpers with actual 16.16 output and OR validity flag | 74 / 110 |
| 2026-10-17 | game/game_state | Completed 22 entries: exact application/battle resource order, modes/surfaces, wind sampling/notification, speed packets, CD cache/reopen, cursor/HUD defer behavior and preview cleanup | 75 / 110 |
| 2026-10-17 | game/main | Completed eleven entries: startup/early exits, full command-line consumption and inert B branch, DirectPlay registration, language/INI, focus/pump, AudioCD shell preservation and diagnostic/OOM termination | 76 / 110 |
| 2026-10-17 | ingame/screenshots | Completed the single owning frame entry: all key gates, directory/capture delegation, timestamp reset, two-snapshot mouse arbitration, audio/timers and callback suppression | 77 / 110 |
| 2026-10-17 | ingame/dialogs | Completed 23 entries: share/gift/chat/command permissions, unit-info ownership, tab/HUD handling, score/rank mutation, combat music ring, exact pacing/profiled tick order and debug events | 78 / 110 |
| 2026-10-17 | game/share_commands | Completed twelve entries: five preference flags, four-toggle ShareAll without LOS, threshold clamps/messages, exact registration masks, outgoing compression and local ranges/BPS/effect allocation toggles | 79 / 110 |
| 2026-10-17 | ingame/camera | Completed eight owned entries: radar/start centering, HapiBank coordinates, bookmark/reset semantics and ineffective array-address validity check, plus launch flag setters | 80 / 110 |
| 2026-10-17 | ingame/markers | Completed six entries: flat build-footprint sweep versus model wireframe, terrain circles, diagnostic/kamikaze rings, target cache/icon mutations, 48-unit path samples and target ellipse | 81 / 110 |
| 2026-10-17 | ingame/keys | Completed five entries: all event/modifier cases, STOP interface state, self-destruct mixed-selection policy, movie/debug/speed gates, numbered paths and BMP band capture/restoration/failure limits | 82 / 110 |
| 2026-10-17 | ingame/selection | Completed 29 entries: visibility lists, all selection/visit/cycle predicates, hull/minimap picks, cursor/overlay aggregation, squads, per-unit formation-offset command dispatch, vector wrapper and adjacent condition constructor | 83 / 110 |
| 2026-10-17 | gui/keyboard | Completed fourteen entries: exact ring capacity/drop/sentinel rules, every virtual-key mapping and held query, Windows message boundary, blit event, cursor wrappers and retained mouse snapshots | 84 / 110 |
| 2026-10-17 | gui/mouse | Completed seventeen entries: three-buffer cursor composition, hide/show/overlay modes, sprite borrowing, tag/event lock, 33ms worker, stop/shutdown hazards and full/empty ring semantics | 85 / 110 |
| 2026-10-17 | gui/listbox | Completed seven entries: exact bevel color argument order, fill/raised/sunken geometry, background whole-surface tiling, reachable skin paths and nine-frame/minimum-size constraints | 86 / 110 |
| 2026-10-17 | gui/file_list | Completed fourteen entries: paired sorting/key directions, exact directory attributes, requester navigation/callback/lifetime, fixed buffer bounds and all blink-slot allocation/timing/font operations | 87 / 110 |
| 2026-10-17 | gui/gui_file | Completed 33 entries: full common/panel/type schema/defaults, translation/narrowing asymmetries, backup writes/source mutation, top-level enumeration/count limits and common/font GAF load/free/baseline handling | 88 / 110 |
| 2026-10-17 | GUI tracing checkpoint | Wrote substantive context/state/text/geometry/list-rendering coverage after tracing main through line 1909, both companions and context tail; retained Partial while remaining renderer/input/layer bodies are unwritten | 88 / 110 |
| 2026-10-17 | gui/gui | Completed all 130 entries: context and gadget state, text layout, all widget render/input paths, layer resource lifetime, focus/quick keys, callbacks and update-loop activation/automatic closing. Audited 1377 local links with zero failures | 89 / 110 |
| 2026-10-17 | gui/dialogs | Completed 49 entries: load/append/stack failure paths, dynamic controls, cursor/mouse admission, text editing and clipboard, named dialog callbacks, message sizing, wrap/truncation and palette editor/remap helpers | 90 / 110 |
| 2026-10-17 | frontend/frontend | Completed 38 entries: full state/substate transitions, lobby and loading handoff, main-menu/CD-player callbacks, pixel spark animation, movie/warp/debug flow, cache ownership and no-op hooks | 91 / 110 |
| 2026-10-17 | frontend/campaign_menu | Completed 34 entries: campaign/side discovery and ownership, single/new-game callbacks, mode-dependent selection, narration/color-markup pagination, all planet/panorama bindings and animation, setup-slot queries; 1410 local links audited | 92 / 110 |
| 2026-10-17 | frontend/skirmish_menu | Completed nineteen entries: setup schema, dynamic row/skin construction, controller/color/alliance rules, map previews and ownership, resource/rule editing, full launch validation, live-slot application and typed count controls | 93 / 110 |
| 2026-10-17 | game/statistics | Completed six entries: allocation schema and partial-failure cleanup, player/ally/score population, dual HAPINET/DLL reporting setup, export/version/init paths and stale-pointer lifetime; audited 1434 local links | 94 / 110 |
| 2026-10-17 | frontend/endgame | Completed 28 entries: game allocation, mission-result snapshots/maxima, artwork ownership, all fade routines, packed mission markers, result bars/buttons, CD retry and complete nine-state outcome flow plus cache exit helpers | 95 / 110 |
| 2026-10-17 | game/victory | Completed 99 annotated entries: all condition registration/predicates/event timing/save pairs, mode-specific allied/elimination/mismatch checks, Packed/Lite/received goal state and generated lifetimes; audited 1468 local links | 96 / 110 |
| 2026-10-17 | network/net_stats | Completed fifteen entries: bit-level layout/boundaries/ownership and scalar-growth defect, direction/per-kind accounting, nominal-versus-encoded packet counters, reset/sample arithmetic, VTOL registration and GAF stepping | 97 / 110 |
| 2026-10-17 | network/online | Completed six entries: executable-relative DLL/version/export contracts, button/result handling and caller exit, link-name translation/count assumptions, raw configuration lifetime and exception/default paths; 1490 links audited | 98 / 110 |
| 2026-10-17 | network/net_condenser | Completed nine entries: both global lifetimes, overwrite/resizing contract, complete envelope/codec/checksum selection, simulated-drop/counter semantics, raw/system receive gating and expanded-buffer retry ownership | 99 / 110 |
| 2026-10-17 | network/packets | Completed 68 entries: manager/channel pool lifetime and growth/reuse, queue mutation and send failures, pacing, sequence/saved-buffer ordering, command splitting/smoothing, player helpers and no-op hooks; audited 1505 links | 100 / 110 |
| 2026-10-17 | network/hapinet | Completed 44 entries: COM/interface ownership, packet/guaranteed flags and HRESULT conventions, session create/join/update/enumeration, providers/connections, lobby settings, player/address adapters and reporting stubs | 101 / 110 |
| 2026-10-17 | network/net_chat | Completed six entries: record/ring settings mismatch, flags and local echo, formatting/routing boundary, expiration, unit jump cycling and full filter/logo rendering behavior | 102 / 110 |
| 2026-10-17 | debug/debug_helper | Completed its single body: load/export/integer calling contract, identity fallback, debugger diagnostics, per-call library ownership and external boundary; 1518 local links and 110 CSV keys audited | 103 / 110 |
| 2026-10-17 | network/unit_sync | Completed 74 entries across nine files: catalogue/permission/checksum protocol, participant progression and acknowledgments, apply/lifetime, independent sequencing, report dispatch and all map/vector/list support; 1538 links audited | 104 / 110 |
| 2026-10-17 | game/savegame | Completed eight entries: save discovery/filter ownership, summary/radar previews, save/delete outcomes, load disc/full-bank validation, teardown-before-validation, pending-bank restore and BetweenMissions paths | 105 / 110 |
| 2026-10-17 | frontend/options | Completed seventy entries: snapshot/default/undo, live sliders and device effects, shell surfaces/animation, help/briefing/settings, restart and surrender. Validated 1563 local links without failures | 106 / 110 |
| 2026-10-17 | game/console_commands | Completed 83 entries and all three command tables: exact masks/callers, player/resource/visual/audio controls, scripts and unit spawning, deliberate failure callbacks, terrain contours and diagnostic overlays. Audited 1594 local links without failures | 107 / 110 |
| 2026-10-17 | frontend/multi | Completed 95 entries across seven files: providers/address ownership, session browser/version gates, reporting, battle-room rows/readiness/launch, observers/alliances, restriction catalogue/portraits and LST persistence. Validated 1610 links. Reconciled all 110 unique roadmap keys and 107 earlier substantive references; retained net_game Partial and debug_lib Not started | 108 / 110 |
| 2026-10-17 | network/net_game | Completed 91 entries across three files: identity/session lifecycle, native protocol table/mode gates, full dispatch, load/start handshake, colors/alliances, rejection/timeouts, heartbeat/ping, unit/script/construction/weapon handoffs and economy/sharing. Audited 1633 local links without failures | 109 / 110 |
| 2026-10-17 | debug/debug_lib | Completed all 267 owned entries across 24 files: allocator/reservations/guards, trees/history, stack/PE/IMAGEHLP reporting, fatal/debug thread, memory/performance UI, GDPERF, timers, registry/window state and runtime string support. Validated 1661 local links before promotion | 110 / 110 |
| 2026-10-17 | Final completion audit | Confirmed 110 unique roadmap rows exactly match CSV module groups, all 110 Finished with substantive module references, no structural exceptions, and 1663 local Markdown links with zero broken targets. General chapter drafts remain explicitly separate. Documentation-only changes in naval-reference worktree | 110 / 110 |
