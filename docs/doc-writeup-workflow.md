# Technical documentation workflow and continuation method

[Systems manual](systems/index.md) | [Progress roadmap](doc-writeup-roadmap.md) | [Module inventory](systems/module-inventory.md)

Recorded 2026-10-16 at the user's request. This document preserves the working method used to develop the comprehensive Byte Tactics systems references. It is a practical continuation guide, not another completed game module.

## 1. Scope, authorization and workspace

Document all 110 distinct folder/module groups recorded in data/modules.csv. Explain behavior from existing source, including dependencies, state, execution, interactions, visuals, failure paths and persistence/networking. Naval/ship behavior remains an important cross-system case study, but completion requires the full module contract rather than only a ship-related example.

Keep every edit inside D:\bytetactics\.worktrees\naval-reference on branch docs/naval-reference. The documented baseline is revision 97d7ac59b460417bcdfa629cf90da99f31818c65. Do not edit the main checkout, change game source, run builds or matching checkers, install dependencies or execute the game. Do not copy source listings into the documentation. Prose, behavioral tables, compact formulas, function addresses and source links convey the implementation without reproducing it.

Work is authorized to continue until all 110 groups are comprehensively covered or the user explicitly stops/pauses it. Progress questions and repeated scheduled follow-ups do not cancel the work. A later explicit stop/pause takes precedence over a scheduled follow-up. Do not spawn another task, writer or agent. Continue as one writer and do not duplicate scheduling infrastructure.

Repository cleanup instructions describe a different source-refactoring workflow. For this documentation task, the user's explicit docs-only scope overrides instructions to build, claim cleanup issues or avoid docs edits. Preserve the repository writing rule against em dashes. Do not claim MATCH or game-execution evidence from static reading.

## 2. Start or resume without losing work

Read [the roadmap](doc-writeup-roadmap.md) and [the manual index](systems/index.md). The roadmap, rather than a remembered count in a chat message, is the current completion authority. Inspect the existing module document before creating or replacing it. Review relevant repository instructions and inspect worktree status without resetting or discarding existing files.

Use git with core.symlinks=false for status inspections in this Windows worktree when needed; otherwise toolchain symlink representation can create misleading source-change noise. Documentation is currently uncommitted work. Do not create an unsolicited commit, push or PR, and never use a hard reset to make the tree look clean.

Pick the next unfinished dependency. The established sequence has moved through world loading/terrain/features, players and unit lifecycle, orders, definitions/scripts/steering, construction/service control and combat. Existing broad chapters can guide dependencies but do not replace module references. Complete a module before recording its completion; for a large module, preserve substantive partial work and honest open responsibilities.

## 3. Establish the real module boundary

Treat a module group as the distinct folder/module pair in data/modules.csv. Combine repeated ranges for the same pair. Sort all recorded start addresses; each annotated function belongs to the latest start not greater than its address, up to the next boundary.

Search all source translation units for FUNCTION address annotations and assign them by those ranges. Do not infer ownership solely from a filename. Shared translation units contain methods belonging to other address-assigned modules, and separately retained files can contain essential members of the current module. Account for constructors, deleting destructors, library specializations and static initialization wrappers as well as obvious gameplay callbacks.

Useful examples already encountered:

- Six unit-target/reference methods live in unit.cpp; its main target file alone would undercount coverage.
- AddToCategories is physically in unit_types.cpp but belongs to unit_commands by address.
- Motion services in order_list.cpp belong to unit_scripts in their address range.
- RegisterVtolOrders lives in net_stats and belongs to that module by address, while the aircraft registration table is physically in unit_orders.
- Feature-at-position lookup belongs to explosions even though reclaim/resurrection consume it.

Compare both annotation count and unique addresses. Avoid corrupting an outer match/capture variable inside nested PowerShell pipelines: save the address and filename before doing an inner range lookup. Library support entries can be explained together when their responsibilities are genuinely shared; they must not disappear from coverage accounting.

## 4. Trace behavior in dependency order

Begin with declarations, record layouts, flag sources and public entry points. Then read complete function bodies, not only names or leading comments. Use rg and targeted file slices to keep output manageable. Read independent source searches together; keep edits and dependent follow-ups sequential.

For each responsibility, establish:

1. Who calls it, which state/arguments arrive, and which owner has authority.
2. Which state it reads and writes, including packed flags and reused fields.
3. The exact condition order, branch priority, early exits and return contract.
4. Timing units, thresholds, truncation, signedness, limits and retry behavior.
5. Calls into other systems and what those callees actually guarantee.
6. Outputs: queues, targets, health/resources, occupancy, packets, files or effect requests.
7. Allocation/lifetime, failure behavior, invalidation and cleanup.
8. What remains asset-dependent or explicitly unresolved.

Trace the producer of an uncertain flag through its loader. A private declaration saying hover, armor or speed can be provisional or wrong. For example flags1 bit 12 comes from canhover, bit 19 from floater and bit 20 from upright. A weapon handler's local speed variable can actually be weapon range. Definition flags and runtime flags are different storage and must be named separately.

When a comment conflicts with the body, document the literal body and the discrepancy. Do not silently repair the original algorithm in prose. Conversely, do not promote a comment alleging a machine-level bug into confirmed behavior when the C++ being inspected does not establish it. Mark an inference as derived and say why it follows.

## 5. Particular tracing methods

For state machines, follow every state and interruption path, then connect returned values to the executor. The same numeric callback result can mean different operations on primary and secondary queues. Track wait deadlines together with event masks; setting a time alone does not describe when an order wakes.

For construction, distinguish preview validity, approach, site recheck, unfinished-unit allocation, factory attachment, buildLeft progress, health, script stance, resource request acceptance and completion/release. Trace the renderer separately for wireframes and partial-model display. A nano-particle request is not proof that funded work occurred, especially where a caller ignores a helper result.

For movement, keep world 16.16 coordinates, world-unit shorts, footprint-relative cells, screen projection and visibility-grid coordinates separate. Route generation, waypoint horizons, steering, live collision and rendered position have different contracts. Document integer rounding and finite horizons explicitly.

For combat, distinguish information/target acquisition, category exclusions, unit-versus-position reach, aiming callbacks, reload/ammunition, shot spending, projectile flight, direct collision, area damage and drawing. Waterweapon, toairweapon, canfly, canhover and floater interact through separate tests. Do not claim a named ship/weapon matchup without the loaded FBI/TDF/COB/model data needed to establish it.

For particles/effects, record emitter trigger, count, source/target geometry, update-versus-draw cadence, finite pool behavior and water/ground alternatives. Separate requests for visual effects from health damage, terrain wrecks and resource recovery.

For persistence/networking, record account/box names, size, field order, optional flags, IDs/references, load ordering, short-read/write checks and authority. A packet and a disk save containing similar data are not interchangeable formats. Do not call uninitialized padding a stable magic header or portable schema.

## 6. Write a substantive reference

Use docs/systems/modules/<folder>-<module>.md for a finished module reference. Start with links to the manual and tracker, identify module/range/source files and state its responsibilities. Organize the rest by behavior and dependencies rather than dumping function-by-function paraphrases in arbitrary address order.

Use headings for substantial references, tables for parallel contracts and formulas when they clarify exact arithmetic. Include function addresses near the supported behavior and local links to implementation files. Explain observed differences between similar paths: factory versus mobile construction, ground versus air repair, direct versus area collision, and local versus remote movement.

Account for lifecycle/support entries in a final coverage section. State static-inspection limits and asset dependencies precisely. Avoid generic disclaimers or invented design intent. No em dashes or fenced source listings. Existing prose should remain readable by someone who has not seen the chat or the decompiler's provisional naming history.

## 7. Completion gate

A module is Finished only after its major functional areas, inputs, state, execution, outputs, failure paths, dependencies and applicable save/network behavior are covered and implementation links are checked. An index entry, source link, function list, long word count or a single naval example does not satisfy this gate.

Partial means substantive behavior exists but significant responsibilities remain. Not started means no substantive behavioral reference exists; incidental mentions do not change that. Broader chapter drafts remain drafts even when several supporting modules finish. It is acceptable for a completed module to identify asset-dependent results or an original-code limit, provided that uncertainty does not conceal an untraced major subsystem.

Read the finished document back against the entry-point/behavior inventory. Confirm small support functions and out-of-file methods were not skipped. Check exact comparisons, array capacities, representation widths and branch order, since these frequently change the meaning of an otherwise plausible explanation.

## 8. Update progress with every completion

In the same documentation edit as completion:

1. Change that one roadmap row to Finished, link the reference and set completion date.
2. Recalculate counts from all 110 rows: finished, partial and not-started sum to 110; remaining is 110 minus finished; coverage is finished divided by 110 times 100, rounded to two decimals.
3. Append a dated completion-log row naming concrete responsibilities and the new finished total.
4. Add the module link/description to the manual index and update chapter status only where justified.
5. Validate links and counts before announcing completion.

A partial writeup also needs the corresponding honest row/link update, without incrementing finished. Workflow documents, audits and chapter synthesis do not count as additional module groups. Keep a dated audit entry when reconciling coverage, but do not invent a completion event for an unchanged module.

## 9. Source-link, scope and roadmap checks

Collect Markdown files under docs/systems plus the roadmap, naval reference and this workflow. Resolve each relative Markdown link against its containing file, excluding external URLs and same-document fragments. Confirm every target exists. If adding a fragment/line-specific link, check the referenced heading/line as well; a filesystem existence test alone cannot validate an anchor.

Compare roadmap keys against the 110 distinct CSV keys and flag missing, extra or duplicate rows. Recompute status totals. For every finished row, inspect the linked module document and its substantive coverage, then compare that document set with completed rows. Existence/length/heading checks can screen mistakes, but they cannot establish technical completeness. Earlier checks falsely flagged two complete files only because their headings were unnumbered; reading them resolved that without downgrading them.

Check module docs for accidental em dashes and fenced source listings. Inspect git status to confirm source/config/tool files were not changed. No build/test suite is appropriate for these prose-only changes under the user's explicit source-only instruction. Report link/status audits as documentation checks, never as proof of runtime matching or gameplay testing.

## 10. Communication and sustained continuation

Give concise progress updates about completed coverage, concrete findings and the next dependency. Avoid leaving the user without an update for more than about a minute of active work. A long tool read should not block communication unnecessarily. Use a short wait/poll for a running command rather than starting a duplicate operation.

Do not end work after a module merely because its document is written. Perform completion bookkeeping and continue to the next unfinished group. User corrections and requests, including this workflow record, steer the active task; they do not silently cancel the comprehensive documentation objective.

Keep questions for genuinely missing required information. Source inspection, reversible documentation edits, normal naming/layout choices and link checks are already authorized. Do not add approval steps for routine progress. A later stop is binding and should prevent scheduled follow-ups from resuming work until the user explicitly resumes it.

## 11. Handoff checkpoint

At this workflow's creation, 33 modules are Finished, 20 Partial and 57 Not started, leaving 77. The [roadmap](doc-writeup-roadmap.md) is authoritative after subsequent work. All order groups, movement classes/search/steering, COB interfaces/interpreter, unit lifecycle/reference/persistence/definitions, several world foundations, construction/control services and the three weapon groups have module references. Three broad chapters and the naval case study remain separate drafts/cross-system coverage.

The next useful combat dependency is acquisition/information: FindWeaponTarget resides in ai/ai_player.cpp, and sight/radar behavior spans map/line_of_sight, map/los_tables and map/radar. A complete ai_player writeup must cover its much broader computer-player responsibilities, not just that acquisition function. Choose supporting profiles/sight tables first when they clarify the next module. Rendering/model construction, particles, UI, networking, mission/victory, saves, sound and runtime support still contain unfinished groups.

When handing off, preserve current module, source files and address inventory already read, unresolved branches, any draft path, last completed count and last validation result. Resume that work rather than repeating the whole investigation or starting another writer. This checklist and linked tracker make the method reviewable independently of conversation history.

## 12. Continuation checkpoint, 2026-10-16

The current reviewed total is 110/110, with 0 Partial and 0 Not started, leaving 0. Since this workflow was first saved, completed modules include LOS tables, line of sight, radar, info panel, AI profile, AI player, palette services/cache loading, model rendering, GAF frames, drawing and particles. The roadmap remains authoritative after this checkpoint.

All inventoried graphics groups now have finished references; chapter synthesis remains separate. Font now has a four-entry reference and PCX has a 21-entry reference. Surfaces now have a 51-entry reference, including the StringRef::Assign dependency at 0x4c93b0 physically in map/los_tables.cpp. Drawing now has a complete 37-entry reference, particles 127 entries, and GAF bitmap/frame operations 38 entries. Model-render coverage includes three address-owned methods physically in units/unit_motion.cpp, for 35 entries total. The AI player audit includes 115 entries across 12 translation units. Palette services/cache wrappers cover 32 and five entries respectively. The initial truncated PCX read was subsequently replaced by a complete read and 21-entry ownership audit; the screenshot/model-helper middle and actual HAPI append/file-length semantics are now covered.

No source or configuration edits were made. Latest completion validation should recheck every local Markdown link and the 110 roadmap keys before continuing. Preserve the single-writer, existing-worktree, source-only constraints.



The scheduled coverage audit reconciled all 110 unique roadmap keys with the CSV, checked all 43 then-finished references for declared scope, section breadth and behavioral samples, and left partial/general chapters unpromoted. It corrected stale index chapter summaries. Subsequent drawing, particle, surface, font and PCX completions brought the total to 48. Particle boundary audit found 100 entries in particles.cpp plus 27 entries across 18 companions; TimedSubParticles' deadline-independent emission and always-false IsFinished are documented as actual source behavior.



For the blit module, inspect both FUNCTION and ENTRY markers. Its hand-written assembly places multiple callable entry points inside large naked translation-unit functions; a FUNCTION-only inventory would undercount the actual drawing interfaces. All 28 entry bodies have now been read and documented, including the long line sampler, compressed decoders and legacy descriptor discrepancies. Its completion brings the total to 49. Check both caller declarations and actual assembly field/stack reads: DrawTile and XorRectangle expose incompatible local interfaces, and remapped-line pointer behavior differs from ordinary lines. Display now has a complete 30-entry reference. Movies now have a completed 12-entry reference. Its physical file has 13 functions: 12 owned by graphics/movies and WriteSmackStats at 0x47c530 owned by debug/memory_stats. That diagnostic group also owns ShowStatsSummary in debug/memory_stats.cpp.





The adjacent debug/memory_stats group is complete with two entries across two files. Audio is next: begin with shared Sound/sample storage and initialization before documenting CD policy and higher sound categories. Validate ownership against CSV intervals, since the movie file crosses module boundaries. Display numeric system-parameter actions were checked against the existing Windows header: 0x5d/0x5e set/get mouse trails, not work-area rectangles.


Windows waveform fallback is complete (six entries). The DirectSound samples file has been read through all routines, including mixer volumes, RIFF/DIGI/raw loading, stream halves and four compression-support entries before the sqsh boundary at 0x4d0ab0. The subsequent global address audit found 51 owned entries, all now documented. CD audio is now complete with 34 entries. Next are game-level sound registry/speech and sound category loading. Both main source files have been read; complete their ownership audits and references before promotion.




Sound types are complete (six entries). Game-level sound ownership audit found 31 entries in sound.cpp, including the three adjacent vector/squad helpers before 0x480190. The category-key and speech-priority symbols are offset views of g_unitMessages in units/unit_messages.cpp, not independent metadata arrays. The completed 31-entry sound reference now covers the table, packet flag discrepancy, all queue/audio/text gates and adjacent helpers. All five sound groups are finished. Next document utility registry/settings support before the broader settings/startup modules.



Registry support is complete (16 entries). Last comprehensive link/key audit at 57 modules checked 968 local links, zero issues, 110 unique CSV-matching keys and exactly 57 reference files. Next read and document game/settings, then settings_campaign; broader startup chapters remain drafts.


Game/settings is complete with six globally audited entries. All settings_campaign source has been read: nine entries, including save-bank adapters and complete save/load orchestration, not just registry progress. Write that reference next; underlying HapiBank/save-browser modules remain separate unfinished groups.


Settings_campaign is complete (nine entries). Save orchestration is now documented without promoting HapiBank, save-browser or victory modules. Continue with shared allocation/container/persistence support and remaining startup assets. Note InitBank at 0x4b3620 sits before the util/hapibank boundary, so the util/int_map address group owns it despite its physical file.


Global allocation adapters are complete (four entries). Next is util/int_map: the full 104-line game source has been read, but its 14 compiler-generated tree methods require inspection in the existing MSVC 5 MAP/XTREE headers. Include its two static lifetime entries, three COB cache methods and HapiBank::InitBank tail before declaring completion.


Int_map is complete with 20 entries after inspecting existing MAP/XTREE source in the main checkout toolchain. No toolchain files were edited or executed. HapiBank has been read through LoadAccount (first 568 lines); read the remaining CRUD/box/audit bodies and audit the 0x4b3630..0x4b4f10 interval next.


HapiBank is complete with 25 owned entries after reading all bodies and globally auditing boundaries. Next close math/checksum/fixed-point support used by COB/model systems, then compression/parser/archive dependencies. Do not count the adjacent InitBank twice.


Math is complete with 25 entries. Assembly source overviews were corrected in the reference: lookup byte indexing selects 512 angular samples, and the FPATAN stack computes atan2(first,second). Next document command parser/registry support before console handlers, then complete TDF/compression/archive foundations.


Commands are complete with 21 owned entries after reading the existing VECTOR insertion body. Registration sorts case-insensitively but tests exact equality; document that rather than claiming unique case-insensitive registration. Next is util/tdf, currently Partial. Do not promote it until its actual parser/record/file/cache/storage behavior is covered.


TDF is complete: 62 entries across two intervals, six util files and four owned methods physically in runtime/basic_string.cpp. Existing VECTOR/STDEXCEPT/XSTRING headers were inspected for generated support. Promoted the actual Partial row, reducing Partial to 11 while Not started remains 33. Next inspect the adjacent runtime/basic_string interval, then SQSH/archive foundations.


Runtime/basic_string is complete with two globally audited entries, _Copy and _Split, traced in existing XSTRING. Other functions physically in basic_string.cpp retain their own address ownership. Next inspect util/sqsh across all companion files and helper entries before archive support.


2026-10-17: SQSH is complete with 22 game-source entries. Existing zlib 1.0.4 source in the main checkout thirdparty directory was inspected read-only for delegated codec contracts. Last full link/key audit at 67 checked 1092 links, zero issues, 110 matching unique keys and 67 module references. Next is util/hpi (Partial): inspect the complete main/companion bodies and archive ownership before promotion. No new task or agent was spawned.


2026-10-17: HPI is complete with 58 globally audited entries across six implementation files. The audit at 68 checked 1103 local links with zero issues and confirmed 68 Finished, 11 Partial, 31 Not started before this promotion. The HPI reference now accounts for its complete file/discovery/archive/writer responsibilities. Continue with startup data-file mounting, preserving broader chapter draft status.


2026-10-17: Data_files is complete, 22 entries. Validated 1132 local links before promotion with zero issues. Corrected the broad startup draft's archive mode/shadow descriptions using the completed HPI implementation. Continue with game/cd_check and side_data, both source files now opened; trace all remaining bodies and their address ownership before completion.


2026-10-17: CD_check (six entries) and side_data (five) are complete. Checked 1149 local links with zero issues before these promotions. Finished 72, Partial 8, Not started 30. Next close main_loop and its profiled frame work, using chapter 02 as existing partial coverage rather than assuming its chapter presence proves completion.


2026-10-17: Main_loop is complete with ten globally audited entries. Its commander name-table view was reconciled with the side-definition commander offset, and starting resources are explicitly documented as prefix maxima. Next read game/game_load in full, including thread handoff, mission options, start assignment, synchronization and save-restoration branches.


2026-10-17: Game_load is complete with seven globally audited entries, after reading both complete implementation files and direct start/projection/thread dependencies. Latest link audit: 1177 local targets, zero issues. Roadmap has 110 unique rows. Re-read compact finished squad/allocation references as behavioral audit samples; their short lengths match their four-entry scopes, not missing index-only coverage. Next finish game/game_state, then application main and remaining frame/scheduling groups.


2026-10-17: Game_state complete, 22 owned entries ending at 0x491ec0 (the next savegame entry must not be included). Link audit 1193 targets, zero issues before promotion. WindEnabled is explicitly a fresh-sample notification, and CD cache insertion does not require a sixteen-track disc despite the overview comment. Next finish game/main across its main file, command-line/WinMain companion and debug-keyword companion.


2026-10-17: Main complete with eleven globally audited entries, including all command-line branches and registration-only behavior. Latest audit 1210 local links, zero issues. Third companion is ReportViaException, not a debug-keyword body; IsDebugKeyword remains debug-owned. Next close ingame/screenshots (one frame dispatcher, already read in full), then ingame/dialogs (23 annotated routines) including all dialogs beyond pacing.


2026-10-17: Screenshots complete (one owned frame entry). All 23 ingame/dialogs routines now read, including talk/share/unit info/tab/score/music and pacing/ticks/debug events. Important callee correction before drafting: TransferEnergy actually spends/credits metal, TransferMetal actually spends/credits energy in players_464290.cpp, so SHARE's apparently swapped names do not establish swapped resource behavior. Preserve this finding explicitly.


2026-10-17: Ingame/dialogs complete with 23 globally audited entries. Full link audit checked 1241 targets with zero issues; roadmap's 110 unique keys exactly match modules.csv. The two remaining Partial groups are share_commands and net_game; all other unfinished groups remain Not started honestly. Continue with the twelve sharing/diagnostic command adapters before larger console/network/UI consumers.


2026-10-17: Share_commands complete with twelve entries. ShareAll toggles four preferences and omits LOS; LOS helper is absent from the shown general registration table. Negative thresholds and requested-versus-stored confirmation are preserved. Next complete camera/input dependencies before larger interface consumers. Net_game is the sole remaining Partial group, not promoted by these direct dependency traces.


2026-10-17: Camera complete with eight globally audited entries. Current link audit printed above this checkpoint; source links all resolve. Camera bookmark restore tests the embedded array address, not a selected valid byte. Next read all ingame/markers drawing routines and their ownership range, then keys/selection and GUI foundations. No writer, task, build or dependency installation was started.


2026-10-17: Markers complete, six globally audited entries. Prior link audit at 80 checked 1269 targets, zero issues. Latest audit printed before this promotion. Build footprint timing uses unsigned age arithmetic, cursor validity is OR, and weapon-three display flag/cache differences are explicit. Next read all five ingame/keys entries, including capture/mosaic state restoration and the complete event switch, then selection/GUI input foundations.


2026-10-17: Keys complete, five globally audited entries. Main source read in full, PCX row writer and console replay/poster callers traced. ExecuteCommandLine null means retained command-buffer replay; ingame/dialogs caller locally treats its void-defined forwarding result as an int, so avoid presenting clean result typing. Next complete ingame/selection and then GUI keyboard/mouse foundations before larger frontend groups.


2026-10-17: Selection complete with 29 globally audited entries, including MissionConditions constructor at 0x48df90 and generated vector wrapper. Read existing VECTOR insertion read-only. ClearSelection preserves bit 5; focus-next marks visit state rather than selecting; formation threshold compares approximate squared distance to count*3000. Next complete GUI keyboard and mouse input before generic layers/widgets and frontend consumers.


2026-10-17: Keyboard complete with fourteen globally audited entries; Windows message call sites and mouse-event header traced. Queue capacity thirty means twenty-nine pending entries. Mouse source now read in full (17 entries): draft cursor composition/thread lock lifetime and ring behavior next, including shutdown timeout freeing and fixed save-background capacity.


2026-10-17: Mouse complete with seventeen owned entries. Cursor scratch buffers allocate 1600 pixels then mutate dimensions without growth; running is a thread-start boolean; mode remains one after StopMouseThread. Shutdown's conditional/timeout release hazard is explicit. Next inspect generic GUI file-list/listbox storage and GUI file parsing before larger layer/dialog/frontend consumers.


2026-10-17: Listbox (seven entries) and file_list (fourteen) complete after full-source and direct cleanup/scan/font dependency tracing. The HPI GetCurrentDriveLetter definition writes a caller buffer, so file requester drive reads are consistent; avoid the older summary's character-return assumption. Latest link audit printed before promotions. Next complete gui/gui_file (33 annotated entries), then generic dialog/layer machinery and frontend consumers.


2026-10-17: GUI_file complete with 33 globally audited entries. Common help is read then cleared before translation; source panel ypos can mutate during export; slider text is read but omitted from writer; missing glyph 0x49 uses uninitialized baseline. Latest link audit printed before promotion. Continue generic GUI runtime across both ownership intervals, accounting for companion files and shared declarations, before frontend consumers.


2026-10-17: Generic GUI draft added honestly as Partial, not finished. Main gui.cpp read continuously through line 1909 plus context tail 6723 onward. Both companion bodies read in full. Draft covers state/name/focus/text/quickkey/geometry/list rendering and associated-slider calculations. Continue main at line 1910 (DrawGafBar), then all remaining bodies through 6722. Important discovery: FindBackgroundCell always returns zero, so listbox reference now marks nonzero background-cell branch unreachable through that implementation. Completion still 88; Partial 2; Not started 20.

2026-10-17: Generic GUI complete, all 130 globally owned entries traced through main line 6758 and both companion files. Full audit checked 1377 local links with zero failures. FindBackgroundCell always returns zero; listbox reference corrected. Continue gui/dialogs (49 entries), then frontend consumers. Network/net_game remains the sole Partial group.

2026-10-17: GUI dialogs complete, 49 globally audited entries. Main and companion read in full; 1386 local links validated before promotion, zero failures. OpenMessageBox captures first appended index before additions, contrary to its nearby bug comment. LoadGuiLayer missing-file/read-failure paths are unsafe; palette interpolation copies 1024 bytes through 768-byte buffer. Continue frontend/frontend coordinator before its specialized screens.

2026-10-17: Frontend/frontend complete, 38 globally owned entries. Main source and checksum companion fully read, MenuFrame/MoviePlayer/Surface callers traced. UpdateMenuSparks local terrain names actually alias GUI surface pixels, so document artwork low-nibble rules rather than map features. Continue campaign_menu specialized frontend, then skirmish/options/endgame/network consumers.

2026-10-17: Campaign_menu complete, all 34 globally owned entries. Audited 1410 local links, zero failures. Panorama count+one loop uses modulo, so repeats first frame rather than requesting out of bounds. Preserve brief markup/page buffer hazards and setup FindFreeColor counting inactive records. Continue skirmish_menu before options/endgame/network.

2026-10-17: Skirmish_menu complete, nineteen globally audited entries and validated local links. Start missing-disc warning has no return; validation continues. AllPlayersOneGroup tests team (not controller), team five is unaffiliated. Typed slot counts decoded from globals.csv. Continue supporting statistics before endgame/victory consumers, then remaining options/network/diagnostics/save modules.

2026-10-17: Statistics complete, six globally owned entries and direct reporting consumers traced. Audited 1434 local links, zero failures. AllocScoreTables loop produces nine pointers, not eighteen despite source comment; LoadReporterDll tests failure two versus actual one. Continue frontend/endgame state machine with these score/snapshot foundations established.

2026-10-17: Endgame complete with 28 globally owned entries. Missing-disc Start callback continues; AreStatBarsComplete omits last counted gadget. BuildScrollItems uses packed strings despite allocation units; ShouldShowNextMission includes losing-final-mission retry. Continue game/victory for actual outcome computation, then remaining save/options/network/diagnostics. No source changes or builds.

2026-10-17: Victory complete, 99 owned annotations across main and CheckVictory companion. Death notifications precede unlink; capture precedes owner change and can precede failed transfer. Radius center uses terrain projection helper, not simple height. Included PackedGoal/LiteGoal/PackedPosGoal and memory probe. Audited 1468 local links, zero failures. Continue network/net_stats bit reader/writer foundation, then networking consumers.

2026-10-17: Net_stats complete, fifteen globally owned entries. Verified CountPacket condenser argument order directly: first uncompressed size+3, second encoded total, including simulated successful drops; do not call first counter actual wire bytes. Reader no length, writer growth allocates one dword. Continue online DLL adapter (six entries already fully read) then condenser/packet/transport foundations.

2026-10-17: Online complete, six owned entries across four files. Full link audit 1490 zero failures. DLL translation callback discards Translate return; link span/count consumer assumptions differ; config persists after unload. NetCondenser main already fully read for next writeup. Accumulate overwrites instead of appending; packet output capacity not grown with oversized accumulation.

2026-10-17: Net_condenser complete, nine entries plus packets-owned SendPacketTo traced. Accumulate replaces; checksum/XOR skips final three payload bytes; receive decodes before caller-capacity retry, internal buffer not bounded. Continue network/packets reliable queue/channel ownership before HAPINET and game protocol dispatch.

2026-10-17: Packets complete, 68 owned annotations across main and global init/destruction companions. Receiver/header and main read in full; gap callback confirmed no-op. FrameQueue.ResetFrames leaks prior recvBuffer and retains ring contents, SendQueued ignores condenser result, command copy-outs omit caller-capacity checks. Audited 1505 links, zero failures. Continue network/hapinet transport before remaining protocol/net_game/net_chat/unit_sync and options/save/diagnostics.

2026-10-17: HAPINET complete, 44 owned main-file entries, validated links. Defaults sixteen/1500; COM result conventions vary; lobby setup can retain partial refs; report functions are stubs. Continue net_chat display foundation before larger unit_sync/net_game/frontend multiplayer modules.

2026-10-17: Net_chat (six entries) and DebugHelper (one) complete. Audited 1518 local links zero failures and all 110 unique roadmap/CSV keys. Chat filters mode one has uninitialized show for category two; AddMessage preserves high visit bits and mixes configurable collision modulus with fixed thirty wrap. Remaining seven: frontend/multi, frontend/options, game/console_commands, game/savegame, network/net_game (Partial), network/unit_sync, debug/debug_lib. Continue unit_sync coordination and reporting before net_game/frontend multiplayer consumers.

2026-10-17: Unit_sync complete, 74 annotations across all nine cpp files plus existing XTREE/VECTOR/LIST methods read. ReceiveSyncPacket zeroes ID and bypasses sequencer; compatibility w/h are permissions/matches not rectangle geometry. Finish assumes nonnull; sent counters not transport confirmations. Audited 1538 links zero failures. Continue game/savegame (eight UI entries), then options/console and larger multiplayer/protocol/debug modules.

2026-10-17: Savegame complete, eight owned UI entries. Full save serializer was already settings_campaign; pending restore game_load. Load tears battle down before full bank validation; cleanup differs CANCEL vs -1; missing-description bank and rebuild lists can leak. Continue frontend/options then game/console_commands, before remaining frontend/multi, net_game and debug_lib. Roadmap105/1Partial/4NS, five remaining.


2026-10-17: Options complete, seventy annotations across main and sound-restore companion. All 1563 local links valid. Shell closing flag has writes but no named read; do not invent timed closing. Speed label discards formatted qualifier, sound undo restores speech bit six from saved bit five, and slider open callbacks mutate values. Continue game/console_commands, then frontend/multi, network/net_game (Partial), debug/debug_lib. No agents, builds or source edits.


2026-10-17: Console_commands complete, 83 globally owned entries all main file, plus three tables and direct callers read. Audited 1594 links zero failures. Give metal/energy helper selection reversed, ATM uses viewed player, Include filename token zero, debug mask runtime-accessible, contour negative spacing unsafe, arrow flags5/6 lack initialized color. Continue frontend/multi all seven files (main5019lines), then network/net_game and debug/debug_lib. Remaining three, no completion inferred from incidental coverage.


2026-10-17: Multi complete, 95 annotations in seven files read in full for bodies and layouts. Audit1610 links valid, 110 unique roadmap keys exact CSV, 107 prior references all substantive sections (smallest DebugHelper438words appropriate1function). Important corrections: console Give helper names reversed but actual resource behavior correct, wording fixed; multi heartbeat60GetTicks normally2seconds not minute; Cancel display dialog does not exit; restrictions save LST notSAV, cancel unlimited101 not-1, SaveLSTdeclares skippeddefinitions. Continue net_game main3900lines plus two companions, then debug_lib. Do not stop at checkpoint.


2026-10-17: Net_game complete, 91annotations (89main+2companions). All bodies/layouts read, 1633links zero failures. HandleNetPackets caches ptr before reallocating ReceiveNetPacket, CommandAllowedindexesbeforeimpossiblerangeANDcheck; resource transfer stackamountrawfloatbits; startup21/30/31differentfromcatalogue1a. Continue final debug/debug_lib. Main first330lines read; remaining main331..7180 plus20debug_libcompanions and separately named free_block_map.cpp/memory_status_dialog.cpp, audit entireaddressintervalglobally. Do not stop at109.


2026-10-17: All110modulegroups complete. Final debug_lib267annotations globally audited, main237+27in20numberedcompanions+free_block_map+memory_status_dialog+runtime/basic_string. Main/code bodies and existing VECTOR/XSTRING support traced, source-only. Final index distinguishes completed module references from three draft general chapters and planned chapter synthesis. Subsequent scheduled follow-ups should verify completion and report it, without inventing unfinished groups, spawning writers or rewriting completed work absent a new user request. No game/source/build/config changes. Final link/key/status audit follows.


2026-10-17 final validation: 110rows/110uniqueCSVkeys, 110Finished/0Partial/0Notstarted, 110substantive module references and zero structural exceptions; 1663local links zero broken. Git reports only documentation additions in this worktree, branch docs/naval-reference, no tracked source changes. All authorized module documentation complete; broader chapter synthesis is explicitly not represented as finished.
