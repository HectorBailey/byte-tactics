# Total Annihilation systems manual

This manual documents the game's implementation from the existing Byte Tactics source, starting with the dependencies needed to understand later systems. The reference revision is `97d7ac59b460417bcdfa629cf90da99f31818c65`.

All 110 inventoried module groups now have completed, source-traced technical references linked here. They were written in dependency order. The first three general chapters below remain substantive drafts. The reading-order table distinguishes finished module coverage from broader chapter work still pending. The [module inventory](module-inventory.md) accounts for every distinct folder/module pair recorded in `data/modules.csv`, including infrastructure that is easy to overlook when documenting only gameplay.

The existing [naval systems reference](../naval-systems.md) is a detailed cross-system case study. It remains useful alongside this manual; it is not a replacement for the general chapters.

The [documentation workflow](../doc-writeup-workflow.md) records source-tracing methods, completion gates, audits and continuation instructions. The [doc writeup roadmap](../doc-writeup-roadmap.md) tracks finished, partial, and unwritten module groups. Update it whenever a module writeup is finished, in the same edit as the writeup, including its counts and completion log.

Completed module writeups within the next world chapter: [plot-map persistence](modules/map-plot-map.md), [meteor environment events](modules/map-meteors.md), [terrain placement, occupancy and spatial queries](modules/map-terrain.md), and [map enumeration and mission-object ownership](modules/map-map-list.md). These cover their complete module groups; the broader world chapter remains in progress.

The related [campaign sequences, mission selection and briefing paths](modules/game-campaign.md) module is also complete. It establishes how a campaign or selected map reaches the mission loader, with victory and progress persistence documented in their completed module references.

[Mission metadata, schemas, initial objects and checksums](modules/map-map-load.md) completes the map-loading support module. Terrain rendering and live object creation remain subjects of their own modules.

[Terrain features, wrecks, reclaiming and fire](modules/map-features.md) completes feature definitions, footprint/state storage, transitions, environmental updates, multiplayer damage authority and persistence.

[Unit storage, creation, death and terrain helpers](modules/units-units.md) completes the core unit module, including reusable IDs, construction initialization, replicated creation, death events, cargo cleanup, statistics and corpse creation.

[Unit squads and group command dispatch](modules/units-squads.md) completes the player-owned group storage and per-unit command dispatch module.

[Unit references, target storage, damage and ground alignment](modules/units-unit-targets.md) completes reference listeners, weapon target representations, aim-position resolution, damage/healing/paralysis and four-point model alignment.

[Extraction callbacks, wind callbacks and bitmap-cache allocation](modules/units-unit-motion.md) completes the provisionally named unit_motion module's resource-script callbacks and evicting picture arena.

[Unit save records and dependency reconstruction](modules/units-unit-save.md) completes unit record serialization, recursive attachment/attacker loading, component restoration and mission-unit lookup.

[Player slots, updates, visibility and persistence](modules/game-players.md) completes player setup/lifecycle, scheduling, resource channels, outcome/watch transitions, display visibility and save state.

[Mission command text, ownership transfer and unit categories](modules/units-unit-commands.md) completes initial command interpretation, initial unit creation, gifting/capture handoff, definition lookup and category-mask ownership.

[Unit positioning, attachment, tick execution and state replication](modules/units-unit-position.md) completes spatial changes, activation callbacks, attachment packets, local/remote unit scheduling and periodic motion/full-state synchronization.

[Unit definition discovery, FBI interpretation and asset binding](modules/units-unit-types.md) completes availability/compatibility, shared definitions, flag sources, yard-map decoding, models/scripts, builder lists and downloadable menus.

[Order helper contracts, script waits and movement goals](modules/orders-order-queue.md) completes carried-order insertion, build animation coordination, readiness waits, reclaim pulse calculation, registry access and goal attachment.

[Order instances, queue policy, execution and persistence](modules/orders-order-list.md) completes primary/secondary queue behavior, callback results, cancellation/count changes, cleanup, type registration, order saves and overlay dispatch.

[Command-mode resolution and unit interaction priorities](modules/orders-order-dispatch.md) completes explicit/contextual action selection, attack variants, service/transport priorities and visible-feature reclaim/resurrection gates.

[Movement classes, passability maps and occupancy refresh](modules/units-movement-class.md) completes class parameters, two-bit map construction, footprint clearance, edge grading and timestamp-driven refresh.

[Steering, motion state, script geometry and command cursors](modules/units-unit-scripts.md) completes ground/air motion execution, transport-following positions, movement/SFX callbacks, weapon/nano/sweet-spot queries and interface hints.

[COB unit-value interface, piece changes and scripted effects](modules/units-unit-script-calls.md) completes script-visible properties, piece/cache mutations, wake/thrust/smoke/bubble dispatch, debris requests, transport callbacks and aim completion.

[COB interpreter, cooperative channels and piece animation](modules/units-cob.md) completes runtime execution, all decoded instruction families, eight-channel scheduling, signals/callbacks, piece integration and interpreter-state persistence.

[Resource-account lifecycle and complete economy module contract](modules/game-economy.md) completes the core allocator's account persistence, interfaces and support routines alongside the detailed economy chapter.

[Order destinations, target geometry and path goals](modules/orders-order-targets.md) completes area acceptance, linked-unit and aircraft destinations, arrival tests, local path reuse/repath state, remote waypoint horizons and target persistence.

[Ground and common unit action state machines](modules/orders-unit-orders.md) completes controls, factory/mobile construction, inherited commands, service actions, ground combat/guard/patrol and ground transport/teleport.

[Aircraft actions, landing, combat and airborne service](modules/orders-vtol-orders.md) completes terrain/pad landing, air transport, four attack families, escort/search/patrol, airborne construction, repair and reclaim.

[Route search, terrain knowledge and shared search budgets](modules/ai-pathfind.md) completes the pathfinder's probe, weighted expansion, terrain/visibility costs, bounded traceback and per-player scheduling.

[Production controls, work helpers and camera services](modules/ingame-control-panel.md) completes menu/count/selection state, construction funding/health/decay/completion, repair arithmetic and the module's camera-follow/scroll/shake services.

[Battle-frame input, placement clicks and battle exit](modules/ingame-build-placement.md) completes click/interface routing, retained intent, selection gestures, radar/drag priority and endgame/restart frame transitions.

[Weapon definitions, flags, asset binding and damage tables](modules/weapons-weapon-types.md) completes TDF discovery/IDs, numeric conversions, all loaded flags, shared models, explosion/sound binding, per-unit damage and teardown.

[Live weapons, projectiles, impacts and weapon rendering](modules/weapons-weapons.md) completes firing/aim/reload, water/air reach rules, burst/flight/collision, damage/interception, packet reconstruction and eight render styles.

[Explosion effects, model fragments and debris](modules/weapons-explosions.md) completes generated explosion frames, animation/model effect pools, detached pieces, face shards, water/ground impacts and feature lookup.

[Sight ray tables, quadrant expansion and storage support](modules/map-los-tables.md) completes LOS geometry loading, four-direction line expansion, indexing, container lifetime and adjacent sorting/string support.

[Terrain loading, sight coverage, exploration and fog rendering](modules/map-line-of-sight.md) completes TNT/derived-map loading, live/permanent information layers, ray/circle updates, temporary eyes, terrain artwork, fog and picking.

[Minimap imagery, mapped terrain and contact display](modules/map-radar.md) completes picture/surface lifetime, information shading, unit/projectile symbols, sensor/interceptor overlays, picking records and the camera rectangle.

[Sensor contacts, battle composition and information panels](modules/ingame-info-panel.md) completes detector/jammer/cloak-proximity passes, contact identification, render layering, HUDs, outlines and diagnostics.

[AI profile tokenization and definition protection wrappers](modules/ai-ai-profile.md) completes lexical input rules, sticky errors, integer reads, pending tokens, buffer assumptions and protection adapters.

[AI player state, production, squads and target selection](modules/ai-ai-player.md) completes all 115 entries, including script limits/weights, economic choices, placement grids, contact caches, assault/escort/scout/artillery timers, acquisition, retaliation and storage support.

[Indexed palettes, color transforms and filename helpers](modules/graphics-palette.md) and [palette transform cache loading](modules/graphics-palette-tables.md) complete the two palette groups, establishing blend/shade/light/gray/blue tables, display ownership and derived-file behavior for rendering consumers.

[Model state, cached rendering, construction wireframes and attachments](modules/graphics-model-render.md) completes all 35 model-rendering entries, including piece posing, lighting, supersampling, materialization bands/edges, shipyard depth composition, water/cloak and shadows.

[GAF frames, animation references, sprite drawing and compression](modules/graphics-gaf.md) completes all 38 entries, including loaded-asset fixups, sequence timing, effect masks, scaling, depth/water composition, lens/background buffers and transparent run encoding.

[Drawing primitives, polygon spans and construction edges](modules/graphics-draw.md) completes all 37 entries, including clipping, remap/shape APIs, fill/edge scan conversion, byte-depth comparisons, point containment and text/font style accessors.

[Particle pools, emission, wakes, nanolathes, smoke and visibility](modules/graphics-particles.md) completes all 127 entries, including six emitter/record families, pool exhaustion, ten draw layers, sight gates, drift/timing and persistent TimedSub behavior.

[Surfaces, screen presentation, textured polygons and shared strings](modules/graphics-surface.md) completes all 51 entries, including screen/DC/DirectDraw paths, image ownership/persistence, texture/depth/light spans and adjacent StringRef methods.

[Bitmap font metrics, string fitting and outlined text](modules/graphics-font.md) completes all four font entries, including first-line byte metrics, trimming limits, whole-string containment, centering, outlines and shared color side effects.

[PCX/BMP images, screenshots, 3DO helpers and display exception cleanup](modules/graphics-pcx.md) completes all 21 entries, including image row/palette contracts, screenshot numbering, shared model relocation/sorting/bounds and output/cleanup failure paths.

[Assembly blitters, compressed sprites and indexed pixel operations](modules/graphics-blit.md) completes all 28 callable entries, including source cropping, transform formulas, line/fill/glyph behavior, texture sampling and retained legacy/caller-interface discrepancies.

[Display environment, mode changes, messages and timers](modules/graphics-display.md) completes all 30 entries, including DirectX checking, GDI/DirectDraw setup and failures, input/window dispatch, palette restoration, cooperative timer semantics, frame counting and shutdown.

[Smacker movie playback, output surfaces and input handling](modules/graphics-movies.md) completes all 12 owned entries, including borrowed/private output, palette/format handling, decoder call contracts, focus/stop/loop behavior, close ownership and adjacent build-site getters.

[Movie playback statistics and summary display](modules/debug-memory-stats.md) completes both entries in the provisionally named memory_stats group, including cross-file ownership, summary fields, arithmetic/stream limits and lack of demonstrated live calls.

[Windows waveform playback fallback](modules/sound-play-sound.md) completes all six entries, including asynchronous memory ownership, disk replacement, distinct loop/resume flags, stop cleanup and return-declaration limits.

[DirectSound samples, channels, streaming and mixer state](modules/sound-samples.md) completes all 51 entries, including owning sets/borrowed voices, overlap/eviction limits, PCM/DIGI/WAV loading, half-buffer streams, delayed requests, 3D/mixer state and adjacent compression initialization.

[CD audio devices, track policy, categories and fades](modules/sound-cd-audio.md) completes all 34 entries, including logical/physical tracks, playback orders, category scans, cooperative fades, MCI messages, pause/stop/enable, external-player handling and activity-policy consumers.

[Sound definitions, speech choices and category ownership](modules/sound-sound-types.md) completes all six entries, including allsound/sound TDF enumeration, parallel choice/caption arrays, consecutive suffix rules, shared message-key aliases, registry binding and cleanup.

[Game sound routing, positional audibility and unit speech](modules/sound-sound.md) completes all 31 entries, including backend selection, named/positional/network sounds, all message metadata, priority/cooldown/queue/text behavior, unit lifetime and adjacent container/squad helpers.

[Registry storage, rectangle tests and environment utilities](modules/util-registry.md) completes all 16 entries, including registry byte/type/error contracts, inclusive geometry, byte/text/name helpers, CRT thread launch, sleep and display notification callback storage.

[Persistent game options, defaults and unit restrictions](modules/game-settings.md) completes all six entries, including all registry key families, load/writeback defaults, numeric/string limits, audio side effects, skirmish fields, write asymmetries and mission availability masks.

[Save-bank orchestration, campaign bytes and small settings helpers](modules/game-settings-campaign.md) completes all nine entries, including summary/all-account openers, full subsystem load/save ordering, live/between-mission payloads, campaign registry bytes and explicit flag/player-count saves.

[Global C++ allocation and deallocation adapters](modules/util-operator-new.md) completes all four entries, including scalar/array forwarding, underlying retry/debug-fill/free behavior, declaration limits and object-lifetime boundaries.

[COB image relocation, checksum registry and integer tree support](modules/util-int-map.md) completes all 20 entries, including raw-image checksums, destructive fixups, mutating lookup/free, fourteen emitted tree members, static lifetime and HapiBank initialization tail.

[HapiBank accounts, typed items, binary boxes and file format](modules/util-hapibank.md) completes all 25 owned entries, including stateful selection/cursors, type replacement, packed records/pool/account compression, partial failures, ownership and audit output.

[Checksums, random state, vector algebra and angle arithmetic](modules/util-math.md) completes all 25 entries, including exact checksum/RNG rules, vector limits, ten assembly interfaces, actual 512-sample lookup indexing, heading operand order and shared fixed-point matrices.

[Command tokenization, substitution, registration and dispatch](modules/util-commands.md) completes all 21 entries, including byte grammar/bounds, percent-token borrowing, masks/default routing, mixed-case registration, retained ShiftArgs behavior and generated vector/StringRef lifetime.

[TDF parsing, record access, translations and shared field storage](modules/util-tdf.md) completes all 62 entries across two address intervals, including copied input/comment grammar, checksums/navigation/defaults, localization cache behavior and emitted string/exception/vector support.

[Runtime string buffer copying and shared-storage detachment](modules/runtime-basic-string.md) completes both owned entries, including byte-count representation, rounded/exact allocation retry, preserved contents and copy-on-write detachment.

[SQSH chunks, LZSS dictionaries, compression and obfuscation](modules/util-sqsh.md) completes all 22 game-source entries plus linked zlib contracts, including actual ring-index format, presets/locks, final-token behavior, chunk/error/capacity rules and existing library source dependencies.

[Asset paths, shared resources and model texture binding](modules/game-data-files.md) completes all 22 entries, including language fallback, version warnings, loading adapters, resource caches, global sequence names, texture binding, cleanup and unit checksum aggregation.

## Reading order and current coverage

| Chapter | Scope | Status |
| --- | --- | --- |
| [01. Startup, assets, and match lifecycle](01-startup-assets-lifecycle.md) | Process entry, configuration, archives, TDF, asset binding, application states, loading, restoration, teardown | Draft written |
| [02. Time and simulation execution](02-time-simulation.md) | Application frames, ticks, pacing, pause, update ordering, per-unit/player scheduling, rendering boundaries | Draft written |
| [03. Economy and resource accounting](03-economy.md) | Generation, consumption, requests, immediate spending, shortages, backlog, storage, construction, cloak costs, persistence | Module references complete; general chapter remains a draft |
| 04. World, terrain, and features | TNT/OTA details, coordinate conversions, occupancy, features, fire, wrecks, metal, environmental hazards | Module references complete; chapter synthesis planned |
| 05. Players and unit lifecycle | Controller roles, IDs, allocation, creation, removal, references, ownership, teams, transfers, unit restrictions | Module references complete; chapter synthesis planned |
| 06. Orders and command execution | Input-to-order routing, queue structure, primary/secondary orders, callbacks, state machines, cancellation, target objects | Module references complete; chapter synthesis planned |
| 07. Construction and service interactions | Factory/site production, assistance, repair, capture, reclaim, resurrection, transport attachment and unloading | Module references and naval case study complete; chapter synthesis planned |
| 08. Ground movement and pathfinding | Movement classes, pass maps, search scheduling, costs, steering, braking, collision, formations, hover/amphibious behavior | Movement-class, search and steering references complete; chapter synthesis continues |
| 09. Aircraft and transports | VTOL flight, approach, attack runs, landing, repair pads, pickup, cargo, unload, evasion | Module references complete; chapter synthesis planned |
| 10. COB scripts and animation | Script loading, execution model, instructions, threads, callbacks, model-piece operations, unit-value interface | Interpreter, unit interface and geometry references complete; asset-loading services remain in startup/data-file coverage |
| 11. Combat and weapon simulation | Selection, eligibility, aiming, firing handlers, trajectories, collision, damage, armor, experience, paralysis, interception | Weapon definition/runtime, acquisition, damage and explosion references complete; chapter synthesis continues |
| 12. Sight, sensors, and information | Explored/current sight maps, height, radar, sonar, jamming, sharing, cloak, target lists, minimap contacts | Module references including network sharing complete; chapter synthesis planned |
| 13. Rendering and visual effects | Surfaces, palettes, tiles, models, caching, depth, construction, shadows, particles, GAF, PCX, movies, text | All inventoried graphics module references complete; broader chapter synthesis remains |
| 14. Interface and player input | GUI files, layers, gadgets, focus, keyboard/mouse queues, selection, camera, build menus, markers, dialogs, screenshots | Module references complete; chapter synthesis planned |
| 15. Computer-player behavior | Profiles, production decisions, weights/limits, squads, timers, target lists, tactical reactions, strategic dependencies | All three AI module references complete; chapter synthesis continues |
| 16. Multiplayer and synchronization | DirectPlay wrapper, lobby, connections, online integration, packet pooling, pacing, compression, unit sync, chat, ownership | Module references complete; chapter synthesis planned |
| 17. Missions, campaigns, and victory | Briefings, mission metadata, restrictions, condition evaluation, commander rules, campaign progress, endgame, statistics | Module references complete; chapter synthesis planned |
| 18. Save and restore | Banks, account/box organization, object serializers, references, restore ordering, summaries, previews, between-mission saves | Module references including save/load browser complete; chapter synthesis planned |
| 19. Audio and speech | Sample loading, playback, positional sound, voice queues, sound categories, CD music, activity-driven selection, focus changes | All five inventoried audio module references complete; broader chapter synthesis remains |
| 20. Runtime support and diagnostics | Allocation, caches, reference strings, math, compression, registry, console commands, exceptions, profiling, memory diagnostics | All utility, runtime and diagnostic module references complete; chapter synthesis planned |

## Evidence conventions

Each chapter identifies its primary implementation and function addresses. Addresses are navigation anchors into this executable revision, not suggested hard-coded implementation dependencies.

- **Confirmed behavior:** directly established by a function body inspected for the chapter.
- **Derived behavior:** a consequence of those conditions or call relationships, with the dependency explained.
- **Asset-dependent behavior:** requires actual loaded FBI, COB, TDF, 3DO, GUI, GAF, map, or sound data to determine a named unit or mission's result.
- **Open detail:** not yet established at the required depth. Such details stay explicit rather than being completed from assumptions about how an RTS normally works.

These are static technical documents. No game execution, match checking, build, or screenshot comparison is implied. A matching-decompilation repository provides unusually strong implementation evidence, but it does not automatically supply original names, original design intent, decoded game assets, or confirmation that every source comment is accurate.

## How chapters are organized

Each chapter follows the system's inputs, state, execution, outputs, exceptional paths, interactions, and investigation points. Prose and tables explain behavior without reproducing source listings. Mathematical relationships are used where necessary to explain accounting or timing precisely.

The manual distinguishes definitions from instances, permissions from execution, simulation state from display state, and local observation from multiplayer authority. These distinctions prevent many misleading explanations, including treating an available command as a guaranteed action, a drawn resource bar as the exact account balance, or a visible contact as a valid target for every weapon.

## Cross-system dependency chain

Process setup makes file services and presentation available. Asset loading builds definitions and map state. Match setup creates players and unit storage. Simulation scheduling invokes unit actions, weapons, players, and the environment. Those systems produce authoritative state and transient effects. Rendering and UI interpret the observer's view of that state. Networking and persistence cross these boundaries and therefore require their own authority and reconstruction rules.

This chain is the reading order, not a claim that every subsystem runs once in that exact order on every application frame. [Chapter 02](02-time-simulation.md) records the actual tick sequence and its gates.

## Writing progress

All 110 distinct module groups have substantive technical writeups, including gameplay, rendering, construction, interface, startup, persistence, audio, multiplayer and diagnostics. The roadmap records 110 Finished, zero Partial and zero Not started. General chapter synthesis remains separate: chapters 01 through 03 are drafts and later chapters are reading-order topics supported by the completed module references. Asset-dependent results and unresolved original-code details remain explicitly identified within those references.


[HPI archives, file handles, discovery and package writing](modules/util-hpi.md) completes all 58 owned entries, including source precedence, block-cache reads, member/mount lifetime, whole-file ownership, shadow enumeration, metadata relocation, package creation and drive/path helpers. Startup mount selection remains separate.



[Archive mounting, CD identification and content gates](modules/game-cd-check.md) completes six entries. [Side definitions, interface rectangles, fonts and logos](modules/game-side-data.md) completes five entries, including all required rectangle records and resource lifetime rules.


[Battle-frame dispatch, setup transitions and starting units](modules/game-main-loop.md) completes ten entries, including paused network servicing, capture timing, callback transitions, mission options, starting-resource prefix maxima and commander spawning.


[Match loading, worker handoff, synchronization and cursor coordinates](modules/game-game-load.md) completes seven entries, covering both execution paths, pending-save distinctions, synchronization/progress, screen handoff and all three world-picking helpers.


[Application and battle lifetime, wind, speed and interface state](modules/game-game-state.md) completes 22 entries, including full resource order, mode callbacks, environmental sampling, requested/effective speed, CD preferences and HUD/preview support.


[Process entry, command line, application pump and failure reporting](modules/game-main.md) completes eleven entries, including exact argument cases, startup exits, focus/device handling, shell preference restoration and failure reporting.


[Application frame input, screenshots and callback dispatch](modules/ingame-screenshots.md) completes the frame-dispatch module. PCX owns the actual capture encoder; this reference covers keys, mouse arbitration, audio/timers and callback gating.


[In-game dialogs, score display, music activity and simulation scheduling](modules/ingame-dialogs.md) completes all 23 entries, including complete dialog lifecycles and actual transfer-field mapping as well as pacing, tick order and combat-driven CD categories.


[Sharing preferences, thresholds and diagnostic command toggles](modules/game-share-commands.md) completes twelve entries, including exact registration, broadcasts, threshold semantics, ShareAll omissions and direct compression/effect consumers.


[Camera centering, bookmarks, persistence and launch flags](modules/ingame-camera.md) completes eight entries, including radar/start differences, the Camera bank schema, bookmark validity behavior and direct launch-flag adapters.


[Build footprints, order icons, path trails and range overlays](modules/ingame-markers.md) completes six entries, including projection/timing, remembered target positions, all diagnostic rings and callback-specific output/failure behavior.


[Battle key dispatch, STOP mode and large bitmap capture](modules/ingame-keys.md) completes five entries, including every event branch, mixed self-destruct selection, capture numbering/band arithmetic and temporary state restoration.


[Selection flags, picking, groups, overlays and order distribution](modules/ingame-selection.md) completes 29 entries, distinguishing selected/visit flags, observer/local lists, model/minimap picks and per-unit command/formation interpretation.


[Keyboard queue, virtual-key translation and input snapshots](modules/gui-keyboard.md) completes fourteen entries, connecting Windows messages and modifiers to game event codes and covering cursor/blit-event helpers.


[Software cursor composition, worker lifetime and mouse-event queue](modules/gui-mouse.md) completes seventeen entries, including background buffers, lock/cadence, shutdown boundaries and retained-event fallback.


[Listbox skins and bevelled drawing primitives](modules/gui-listbox.md) completes seven entries. [Directory lists, file requester and blinking text slots](modules/gui-file-list.md) completes fourteen entries with sorting, filesystem/callback behavior and full blink-state lifetime.


[GUI record serialization, field readers and shared GAF assets](modules/gui-gui-file.md) completes 33 entries, covering every field pair, whole-file read/write behavior, backup/failure paths and common/font asset ownership.


The [generic GUI runtime reference](modules/gui-gui.md) completes context/state/text services, gadget rendering and input, focus/quick keys, layer resource lifetime and callback-driven update dispatch. Dialog construction and text-edit buffer support remain in the adjacent dialog module.



[Dialog layers, text editing, message layout and palette tools](modules/gui-dialogs.md) completes 49 entries, including append/loading lifetime, dynamic record limits, clipboard behavior, callback closing and source-observed unchecked paths.

[Frontend coordination, main menu, movies and picture cache](modules/frontend-frontend.md) completes 38 entries and distinguishes menu/substate requests from wider frame modes, with full network-session staging and background/spark ownership.

[Campaign menus, briefing pages and animated mission artwork](modules/frontend-campaign-menu.md) completes 34 entries, including list discovery/ownership, campaign versus any-mission selection, pagination/color spans, narration, all artwork mappings and setup-slot queries.

[Skirmish setup, player rows, map selection and launch checks](modules/frontend-skirmish-menu.md) completes nineteen entries, including stored-versus-live state, controller/team/color constraints, preview lifetime and exact launch/failure flow.

[Online score tables and reporter DLL lifetime](modules/game-statistics.md) completes six entries, covering snapshot layout, participant flags/score formulas, allocation and teardown failures, DLL export binding and HAPINET fallback.

[Endgame snapshots, outcome screens, fades and result controls](modules/frontend-endgame.md) completes 28 entries, accounting for capture-before-teardown, campaign retry/final-movie policy, resource cleanup and timed statistics presentation.

[Mission conditions, elimination rules and replicated movement goals](modules/game-victory.md) completes 99 entries, including every configured predicate and persistence pair, death/capture ordering, allied-victory policy and the goal classes sharing its address interval.

[Bit streams, traffic counters and sampled network rates](modules/network-net-stats.md) completes fifteen entries, including low-first bit encoding, unchecked reader/growth limits and exact counter/sampling semantics.

[Optional online.dll service buttons and configuration](modules/network-online.md) completes six entries with export/version contracts, service-button outcomes, link-array assumptions and configuration-versus-library lifetime.

[Network payload compression, envelope checksum and receive caching](modules/network-net-condenser.md) completes nine entries, including original-size versus encoded counters, unchecked internal capacities, checksum coverage and caller-buffer retries.

[Packet channels, retained pools, frame sequencing and receive queues](modules/network-packets.md) completes 68 entries, covering descriptor ownership, forced retention reuse, exact send-result limits and command due-tick/saved-buffer handling.

[HAPINET DirectPlay transport, sessions, providers and lobby interfaces](modules/network-hapinet.md) completes 44 entries, distinguishing raw HRESULT and Boolean adapters, borrowed descriptions, partial setup cleanup and external COM boundaries.

[Message HUD ring, chat formatting, filtering and unit jumps](modules/network-net-chat.md) completes six entries. [Optional DebugHelper.dll value adapter](modules/debug-debug-helper.md) completes its single-entry module, independently of the larger debug runtime.

[Pregame unit catalogue, compatibility, restrictions and report dispatch](modules/network-unit-sync.md) completes 74 entries, distinguishing local availability and limits from live-unit state, and accounting for every companion/container and report function.

[Save/load screens, summary previews and restore handoff](modules/game-savegame.md) completes eight UI entries and links the already documented full serializer/loader, with explicit ownership, failure and between-missions behavior.

[Options pages, live preferences, panel animation and battle exit dialogs](modules/frontend-options.md) completes seventy entries, including shared undo snapshots, runtime/frontend distinctions, animation ownership and destructive-action confirmation paths.

[Console commands, debug scripts and terrain diagnostic overlays](modules/game-console-commands.md) completes 83 entries and the three registration tables, tracing command access, literal handler effects, script fallback, selected-goal drawing and map diagnostics.

[Multiplayer connections, battle room, alliances and unit restrictions](modules/frontend-multi.md) completes 95 entries across seven files, including provider/address lifetimes, browsing and launch gates, observer/team rules, portrait loading and restriction-list persistence.

[Network game lifecycle, protocol dispatch and shared match state](modules/network-net-game.md) completes 91 entries with the complete initialized packet table, routing/validation limits, load/start handshakes, gameplay handoffs and session/economy coordination.

[Debug allocator, crash reporting, diagnostic windows and profiling](modules/debug-debug-lib.md) completes the final module, accounting for 267 entries across 24 implementation files and completing 110/110 module groups. It includes native/generated support, allocator ownership and guard layout, exception/stack reporting, timers, diagnostic preferences and external-driver boundaries.
