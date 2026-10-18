# 01. Startup, assets, and match lifecycle

[Manual index](index.md) | [Next: time and simulation](02-time-simulation.md)

## 1. Responsibilities and boundaries

Startup establishes the Windows environment, global game context, archive/file access, translations, audio, shared graphics resources, GUI, settings, and the initial application state. Starting a battle is a later operation that loads map and unit content and creates match-specific state. Restoring a save is a branch of match setup, not a replacement for all initialization.

Three lifetimes are therefore relevant: process/platform services, application resources used by menus and battles, and battle resources associated with a particular match. A resource's presence in a global context does not establish that it should survive a match teardown.

Primary implementation: [main entry](../../src/game/main_49eda0.cpp), [application startup](../../src/game/main.cpp), [game-state lifecycle](../../src/game/game_state.cpp), [match loading](../../src/game/game_load.cpp).

## 2. Process entry and early exits

The Windows entry point at `0x49eda0` wraps the main game entry with structured exception handling. The main body at `0x49e830` initializes debug support and installs an out-of-memory handler before building the application environment. The exception boundary is a reporting mechanism; it does not prove recovery from an arbitrary failed initialization.

The main body creates the global game object and checks for an existing semaphore using the application name. Finding one causes an early return. Otherwise it creates that semaphore, seeds the C runtime random generator, and parses the command line. Registration-only invocation can return without entering ordinary gameplay startup.

Display defaults establish an initial 640 by 480 environment, instance/window information, and flags derived partly from command-line state. Failed environment initialization returns before the ordinary application loop. Successful setup initializes timers with a base argument of 30 and registers the data archives.

An investigation of startup failure should therefore distinguish an existing-instance exit, a command-line early exit, environment initialization failure, missing data, and a later exception. They occur at different stages and leave different resources initialized.

Evidence: [WinMain and parser](../../src/game/main_49eda0.cpp), `0x49eda0`, `0x49ee30`; [GameMain](../../src/game/main.cpp), `0x49e830`.

## 3. Configuration is gathered in stages

The command-line parser splits on spaces and tabs. Unprefixed tokens assign the preferred-language buffer. Recognized switches set or delegate options for display mode, online configuration, archive-drive bypass, hosting, connection provider/address, packet rate, simulated network dropping, sound selection, and registration. Not every branch has identical argument consumption, and some recognized names have empty actions. A complete command-line reference must preserve those actual branch details rather than presenting a generic modern option grammar.

After archive registration, startup reads the preferred language from the registry if it was not already specified and falls back to the default language when necessary. It then loads translations. Later initialization loads other settings and applies audio/brightness choices. An INI preference reader constructs `totala.ini` beside the executable and reads the `Preferences` section.

The application initialization reads `UnitLimit` with a default of 250 and clamps this preference to 20 through 500. Match setup can obtain its effective limit from a different source, including host information. The preference clamp is therefore not a complete description of every source of the current match limit.

The displayed build/version values, data compatibility check, user preferences, and multiplayer content checks are distinct mechanisms. A successful read of one does not validate the others.

Evidence: [parser](../../src/game/main_49eda0.cpp), `0x49ee30`; [language and INI preference reading](../../src/game/main.cpp), `0x49e830`, `0x49f5a0`; [InitGame](../../src/game/game_state.cpp), `0x491200`.

## 4. File lookup and archive mounting

### 4.1 Mount sequence

The archive-registration routine contains a gated mounting sequence. When its controlling state enables the sequence, it removes missing archives from its existing registry, changes to the executable directory, and enumerates:

1. `rev31.GP3` using the keep-stream-open argument.
2. Current-directory CCX files using the same keep-stream-open argument.
3. Current-directory UFO files with the other argument.
4. Current-directory HPI files, stopping after ten successful registrations or exhaustion.
5. HPI files at the root of enumerated CD-ROM drives.

It then resolves shadowed archive files. This establishes mount enumeration order and stream-retention arguments. The [completed HPI reference](modules/util-hpi.md) explains loose-file priority, first-mounted archive priority and separate shadow flags used by discovery. The actual archive order still depends on enumeration results.

The CD-identification helper separately checks a `TOTALA.ID` record for campaign or multiplayer content. The drive bypass and the archive scan are related configuration paths but are not the same as successfully parsing a requested file.

Evidence: [archive registration and CD checks](../../src/game/cd_check.cpp), `0x41d4c0`, `0x41d6a0`.

### 4.2 Loose files and archive-backed handles

The common file opener first attempts a normal filesystem open for the requested name and mode. If successful, it returns a handle with no shared archive. If it fails, it walks registered archives, searches their directory trees, and skips directory entries. Direct opens do not test the shadow flag; enumeration does.

An archive-backed handle refers to shared archive state and its entry information. The archive file may be reopened on demand, and an open count tracks shared use. Compressed entries additionally load a block-size table; an archive obfuscation key can transform that table's bytes.

Thus a loose file can satisfy a read before archive fallback is attempted. A file that exists in an archive is not necessarily selected if a loose file already opened or an earlier archive supplied the same name. Missing an archive's underlying file while reopening can fail the operation even when its directory metadata remains registered.

The source's block-count calculation is integer division by 65536 rounded upward. Despite misleading â€œpixelâ€ terminology in a nearby comment, this context is file-block accounting, not bitmap dimensions. It is an example of why callers and operations take precedence over informal comments.

Evidence: [file opener and closer](../../src/util/hpi.cpp), `0x4bb2e0`, `0x4bb5d0`; [archive records](../../src/util/archive_entry.h), [file handles](../../src/util/file_handle.h).

### 4.3 HPI archives and HapiBank are different

HPI provides archive-backed virtual file access. HapiBank provides structured storage of named accounts, typed items, and binary boxes. The latter is used by save and persistence code. Their similar names do not mean that unit serializers write directly into the game asset archive directory format.

SQSH is another layer: compression/chunk processing with checksums and related helpers. File resolution, archive directory traversal, compressed chunk expansion, and save-account lookup answer different questions and should be traced independently.

Evidence: [archive APIs](../../src/util/hpi.cpp), [HapiBank interface](../../src/util/hapi_bank.h), [HapiBank implementation](../../src/util/hapi_bank.cpp), [SQSH](../../src/util/sqsh.cpp).

## 5. Language-sensitive data paths

`BuildDataPath` at `0x4290f0` uses the preferred-language value to try a suffixed directory before the common directory. For example, the candidate directory is the requested directory followed by a hyphen and the configured language. The candidate must actually open through the file layer before it is accepted.

If it does not open, the function constructs the ordinary directory/name path. When a nonempty extension is requested, it removes the prior extension and appends the requested one in both paths. It returns the constructed fallback path even if that later read will fail.

A local variable and comment call the suffix a side, but its producer is `GetPreferredLanguage`. This helper is language fallback, not proof of faction-specific asset lookup.

Derived consequence: copying only a common file into a documentation fixture may not reproduce the file actually selected when a language-specific candidate exists. First establish the resolved path and file-layer winner, then inspect content.

Evidence: [path construction](../../src/game/data_files.cpp), `0x4290f0`; [preferred language](../../src/game/main.cpp), `0x49f580`.

## 6. TDF parsing and definition values

### 6.1 Record structure

The TDF record parser handles nested named records and key/value fields. It searches for the closing name delimiter and opening record delimiter, recursively constructs child records, and ends on the closing record delimiter. A field requires an equals delimiter and a terminating semicolon. Whitespace trimming happens before stored names and values are constructed.

Missing delimiters and an unexpected end of a nested record enter the parse-error path. Numeric getters do not repair a structurally malformed record; they run only after the field structure exists.

### 6.2 Lookup and duplicate fields

Field storage is sorted with a case-insensitive comparator, and field lookup uses a lower-bound search. During parsing, a matching entry is updated rather than blindly appending a second field. The exact duplicate-case behavior also depends on the parser's final equality comparison, which is not identical to every comparator in the file; unusual differences in capitalization should be investigated against both operations.

Child-record selection uses its own search over child pointers. A nested record and a field within that record are different lookup levels. Selecting the wrong record can cause every getter to use its default while the file itself loaded successfully.

### 6.3 Missing and malformed numeric values

Integer and floating getters return the supplied default when a key is absent. When present, they use C numeric conversion routines. A present but malformed string is therefore not equivalent to a missing field. The loader's default and subsequent type conversion both matter.

Definition loaders can also multiply, truncate, narrow, clamp, or replace the retrieved value. A weapon time in seconds becomes tick counts; a movement class can replace locally read movement constraints; a boolean-like field can be reduced to a bit. The text value is the input to the loader, not necessarily the final runtime representation.

Evidence: [record parsing](../../src/util/tdf.cpp), `0x4c3e40`; [lookup and getters](../../src/util/tdf.cpp), `0x4c4630`, `0x4c46c0`, `0x4c4760`; [unit loading](../../src/units/unit_types.cpp), [weapon loading](../../src/weapons/weapon_types.cpp).

## 7. Shared application resources

`InitGame` at `0x491200` creates an offscreen surface, resets selected application flags, establishes initial limits and interface state, and loads shared resources. Its calls include sound initialization, frontend reset, packet tables, order-type registration, fonts, palette, sound definitions, alpha/shade/light tables, and gray/blue remapping tables.

It allocates skirmish options, loads settings, restores CD track classifications, configures the music object, applies brightness/volume, loads faction side data and logos, and configures GUI paths, fonts, common animation resources, and cursor frames. It ends by installing the initial frame handler and reading the clamped unit-limit preference.

The order matters as a dependency graph. A later GUI can reference fonts and animation frames already loaded; a model's colors can rely on palette remapping tables; battle command handlers can refer to a registered order-type table. These dependencies do not imply that every individual resource load has the same failure policy.

The GPF-version check is another separate path. It attempts to read the expected version record and compares the value against `v3.0`, displaying a compatibility warning on the shown mismatch/missing-record conditions. If the version file itself does not load, the inspected outer branch does not enter those warning paths. This should not be rewritten in prose as an unconditional fatal version gate.

Evidence: [application initialization](../../src/game/game_state.cpp), `0x491200`; [resource loading and version check](../../src/game/data_files.cpp), `0x429000`, `0x429870`; [side definitions](../../src/game/side_data.cpp).

## 8. Application states and input dispatch

The game context holds a numeric application state and a callback used by the frame dispatcher. `SetGameMode` installs the following mapping:

| State | Handler |
| --- | --- |
| 0 | Initial frame |
| 1 | Return to main menu |
| 2 | Menu frame |
| 3 | Prebattle frame |
| 4 | Campaign setup |
| 5 | Loading screen |
| 6 | Battle frame |
| 7 | Endgame frame |
| Other | Null handler in this setter |

Battle state installs a battle quit prompt callback; other mapped states use the leave-game callback. Other code can also assign the state/handler directly, so this setter is a map of one transition interface rather than proof that every transition passes through it.

`MenuFrame` refreshes information, advances the frontend state machine, and selects campaign, loading, or prebattle handlers based on mission type and lobby/menu flags. This is the boundary between a menu selection and the heavier loading process.

Evidence: [state mapping](../../src/game/game_state.cpp), `0x490b30`; [menu transitions](../../src/game/main_loop.cpp), `0x496bb0`.

## 9. Match loading and its dependency order

### 9.1 Frontend and loading worker

The loading-screen handler performs presentation setup, cursor changes, palette preparation, progress tracking, network synchronization activity, and battle-entry transitions. The load-thread wrapper calls `LoadMatch` inside a separate exception-reporting boundary identifying the load thread.

These are separate paths coordinating through loading state. A responsive loading screen does not prove that the worker finished, and a worker waiting on multiplayer start-position flags does not by itself identify a disk-loading failure.

Evidence: [loading screen](../../src/game/game_load.cpp), `0x497f40`; [load-thread body](../../src/game/game_load_497c70.cpp), `0x497c70`.

### 9.2 Mission type and settings

`LoadMatch` at `0x497180` reseeds random sources and resets the game tick. Its mission-type switch distinguishes campaign (1), skirmish (2), and multiplayer (3). Campaign applies its mission options and unit restrictions. Skirmish uses local options. Multiplayer obtains mission and match settings from host information and can wait for connection/setup state while servicing packets.

A pending save can restore player-controller information before the shared battle asset load. This ensures that later reconstruction can use the appropriate controller setup. The `BetweenMissions` summary marker changes which restoration path applies.

### 9.3 Battle assets

`LoadBattleAssets` at `0x4917d0` performs this major sequence:

| Phase | Confirmed work |
| --- | --- |
| Reset | Chat/input/build/hover and selected match flags; speech/lightbar state |
| Visual/content foundations | Texture GAFs and feature-file list |
| Definition foundations | Unit categories, particle lists, projectile allocation, weapon definitions |
| World | TNT map load and camera reset |
| Units | Unit definitions, downloadable menus, runtime unit allocation |
| Navigation/content fixup | Feature links, feature-file-list release, movement pass maps |
| Timing/environment | Wind state, scratch transform buffers, tick/budget reset |
| Runtime services | Meteors, radar, players, pathfinder, explosions, commands |
| Final reset | Unit-created counters, loading/interface flags, activity samples |

The map loader is physically located in `map/line_of_sight.cpp` in this checkout. Similarly, file names do not always reflect the clean conceptual boundary. Search for the function body rather than assuming a module-like filename contains the implementation.

The established load order explains several dependencies: weapons exist before unit weapon-name resolution; terrain and unit geometry exist before pass-map construction; feature links are resolved before the temporary feature-file list is discarded. These are derived explanations of the observed order, not recovered original design comments.

### 9.4 TNT header and environment selection

The TNT loader at `0x483610` explicitly handles versions `0x1020` and `0x2000`. Other versions enter an unknown-version fatal-error path. The supported versions select different header positions and attribute/feature representations.

For the newer format, nonnegative mission wind/gravity settings can override header defaults. Otherwise it uses the header values or its fallback gravity. Tidal strength uses a nonnegative mission value or a default of 0.5. Sea level is narrowed from the header into a byte. Width/height and derived world dimensions feed later grid and visibility allocations.

This is an overview of the loader's initial contract. The complete tile, feature, sight-grid, and attribute layouts belong to the terrain chapter, not to an assumed generic TNT specification.

Evidence: [match orchestration](../../src/game/game_load.cpp), `0x497180`; [battle assets](../../src/game/game_state.cpp), `0x4917d0`; [TNT loader](../../src/map/line_of_sight.cpp), `0x483610`.

## 10. Binding content into runtime definitions

Units resolve movement classes, weapon names, explosion weapons, corpse features, script/model data, categories, and build options. A definition is not just its original text record: it includes links to already loaded resources and derived bounds/flags. Runtime units then refer to those shared definitions while holding individual state.

The model loader constructs an objects3d path, loads the model, reports a fatal error if missing, mirrors/prepares it, and binds textures. Model loading and texture binding are separate stages, so finding a model file does not certify all referenced visual resources are available.

The unit script-checksum routine is broader than its name: it includes the COB file, matching GUI files, an optional download definition, and the weapon hash contribution. It does nothing after a nonzero checksum has already been cached. A checksum mismatch cannot safely be attributed only to changed COB instructions.

Evidence: [unit definitions and options](../../src/units/unit_types.cpp); [model load](../../src/game/data_files.cpp), `0x42a2c0`; [unit content checksum](../../src/game/data_files.cpp), `0x42a610`.

## 11. Fresh starts, restores, and entry into battle

After common battle loading, fresh noncampaign setup creates starting units using side data and configured start positions. Skirmish can use fixed or shuffled assignments. Multiplayer obtains host settings and coordinates start-position readiness before continuing. Exact starting resources are handled later and differ by mission type.

The loader recalculates line of sight. If a pending save exists without the between-missions marker, it restores saved game state. Otherwise a campaign start creates mission units and centers the camera; noncampaign setup follows its own prior creation branch.

The final stage constructs the side-specific battle HUD name, loads that layout, installs its handler, announces player information, updates players, initializes starting resources where appropriate, closes the pending save bank, rebuilds AI feature cells, and marks the load stage complete. A saved match therefore returns through content loading and reference reconstruction before ordinary battle execution.

The loading-screen transition can restore the requested battle resolution, draw the first battle frame, show the software cursor, install battle state, and select gameplay music. Completion of a single progress bar is not the entire transition contract.

Evidence: [LoadMatch](../../src/game/game_load.cpp), `0x497180`; [loading-screen transition](../../src/game/game_load.cpp), `0x497f40`; [starting resources](../../src/game/players_464290.cpp), `0x465e30`.

## 12. Application loop, focus, and teardown

The outer Windows loop dispatches queued messages and runs application frames while idle. If the application is inactive and the relevant network flag is clear, it stops entering ordinary idle frames and returns to message handling. Network conditions can keep work active without ordinary foreground focus.

The loop also manages CD audio across focus loss/recovery and periodically reaps finished audio buffers. Those tasks do not necessarily run at the same cadence as world simulation. Teardown later restores the saved CD-shell setting as part of process cleanup.

Battle teardown and application shutdown are separate. Battle teardown stops/recategorizes music, collects endgame statistics, frees units, particles, explosions, pathfinder, players, radar, map resources, transform buffers, download menus, unit/weapon definitions, textures, movement classes, and categories, then closes the session under its mission-type condition. Application shutdown releases additional shared resources and platform state.

Derived ownership rule: serialize or collect any required unit/player state before freeing its backing storage. The observed endgame-statistics call before unit/player cleanup is relevant to that dependency. It is not permission to assume every destructor can run in an arbitrary order.

Evidence: [message loop and focus](../../src/game/main.cpp), `0x49e830`; [battle teardown](../../src/game/game_state.cpp), `0x491b60`; [application shutdown](../../src/game/game_state.cpp), `0x4916a0`.

## 13. Investigation checklist and open details

| Question | Start with |
| --- | --- |
| Why did this asset win? | Preferred language, loose-file open, archive registry, shadow flags |
| Why did a key use its default? | Resolved file, record selection, key presence, numeric conversion, later overwrite |
| Why does loading wait? | Worker completion, loading flags, start-position synchronization, pending save branch |
| Why does a checksum disagree? | COB, matching GUI files, download file, weapon hash, cache state |
| Why does a new match retain or lose something? | Application lifetime versus battle teardown/reset |
| Why is a source routine in an unexpected file? | Function address/body and repository module history |

Still to be documented in their dedicated chapters: complete archive precedence and binary formats, every command-line branch, settings schemas, complete map layouts, all asset bindings, every save serializer, frontend state transitions, and exact audio-backend lifecycle. No unnamed branch has been assigned a guessed gameplay meaning merely to make the chapter appear complete.
