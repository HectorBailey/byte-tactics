# Online score tables and reporter DLL lifetime

Status: finished. The `game/statistics` interval, `0x46bce0..0x46c620`, contains six entries. Five are in [statistics.cpp](../../../src/game/statistics.cpp); FillScoreTables is in [its companion](../../../src/game/statistics_46c2a0.cpp). This group allocates/populates external reporting structures and loads reporter.dll. It does not maintain the underlying combat/economy counters or decide victory.

## Consumers and reporting paths

The multiplayer frontend's InitScoreReporting in [multi.cpp](../../../src/frontend/multi.cpp) calls this module only for mission game type three. It temporarily selects a busy cursor, calls LoadReporterDll, and opens a report-choice dialog when the result is zero with a positive returned choice count. Result four is silently treated as unavailable; other errors show a report initialization message. No choices means no dialog, even when reporting support was otherwise initialized.

ReportGameEvent and ReportGameChat are physically in the adjacent [unit_sync.cpp](../../../src/network/unit_sync.cpp) and belong to `network/unit_sync`. They consume these tables, invoking FillScoreTables before reporting. Reporting can use HAPINET's callback interface, reporter.dll, or both. Event reporting carries session/provider identity, mission name, local index, participant count and the arrays; chat uses the local report-player pointer. A periodic callback calls the DLL timer only while its module handle is nonzero. [Info-panel callbacks](ingame-info-panel.md) forward reporter events and display notifications. The six entries here support those consumers without defining their event-code protocol.

## Allocation schema and return convention

AllocScoreTables (`0x46bce0`) creates three arrays of ten pointers, each forty bytes. For each of ten physical player slots it allocates:

| Block | Size | Meaning |
| --- | ---: | --- |
| Player report | 24 bytes | Borrowed name/side pointers, ID, flags, ally count and ally-array pointer. |
| Allies | 40 bytes | Up to ten pointers to report-player records. |
| Scoreboard | 12 bytes | Aggregate score, score count and score-pointer-array pointer. |
| Score pointers | 36 bytes | Nine pointers to individual score records. |
| Scores | 72 bytes | Nine eight-byte name-pointer/integer-value records. |

The top arrays are zeroed only after all three have allocated. Score-pointer and score-storage blocks are zeroed, and the nine pointers are bound at byte offsets zero, eight, through sixty-four in score storage. The loop increments its byte offset by eight until seventy-two, producing **nine**, not eighteen, pointers. The adjacent source comment claiming a 36-byte overflow is inconsistent with that body; the observed allocation and loop agree at nine pointers.

Success returns zero. Any detected null allocation calls ShutdownScoreTables and returns one. Allocation tags identify each slot/block. This is not a robust arbitrary-partial-construction cleanup: failure before top-array initialization can leave uninitialized elements, and failure during slot construction leaves null child records that ShutdownScoreTables dereferences before freeing. Normal game allocation policy may abort rather than return null, but the explicit null-return paths do not make the public lifecycle safe for every partial state.

AllocScoreTables does not first release existing allocations. Its caller must shut down prior tables. Names, side strings and score-name strings later stored in these records are borrowed, while the blocks in this table are owned by this module.

## Populating player, ally and score records

FillScoreTables (`0x46c2a0`) scans all ten physical Player slots. Its inclusion expression contains a detailed active/type/index test followed by OR unitsCreated and OR active. As a result, every active slot and every inactive slot with unitsCreated is included, even when the earlier detailed test would fail. It returns an included count but stores results at their original slot indices, rather than compacting entries into the first count positions. Records excluded on this call are not explicitly cleared, so prior values can remain.

For an included slot it borrows Player.name, copies network ID and starts flags at one. It adds bit two for active local computers or active remote type-three players whose PlayerInfo.kind is two; bit four for an active observer/player-info bit-six slot; bit eight for active PlayerInfo.flags_97 bit zero. The side pointer comes from a two-element Arm/Core name array indexed directly by PlayerInfo.side. Additional configured sides or missing PlayerInfo are not checked here, including inactive historical slots admitted by unitsCreated.

Allies are every other slot whose Player.allied byte is nonzero. The helper does not additionally require the ally to be active or included in the participant count. It stores report-player pointers, counts them and clears unused ally entries to null through slot nine. Self is excluded.

Aggregate score first truncates gameTick divided by sixty to an integer quotient, multiplies that quotient by the mission time multiplier and converts to int. It separately converts kills times the mission kill multiplier to int and adds it. Truncation happens before the time multiplication and separately for both terms; it is not a continuous floating-point elapsed-time score. Overflow, negative multipliers and out-of-range float conversions have no special handling.

Every included scoreboard advertises nine named integer fields:

| Position | Value |
| --- | --- |
| 0 | Kills. |
| 1 | Losses. |
| 2 | Total energy produced, converted to int. |
| 3 | Total metal produced, converted to int. |
| 4 | Energy wasted, converted to int. |
| 5 | Metal wasted, converted to int. |
| 6 | Commander kills. |
| 7 | Commander losses. |
| 8 | The game winner bit at offset `0x3923b`, bit four, for the local index or an active type-two computer; zero for other slots. |

The last field's configured label is I am Winner. It does not independently compute whether each player survived or won. Notably local and active computer records share the same global winner bit through this expression. Live counters and alliances are owned by [players](game-players.md), [economy](game-economy.md), unit/combat code and victory/endgame, rather than calculated from unit enumeration here.

## Loading reporter.dll and enabling HAPINET reporting

LoadReporterDll (`0x46bf30`) begins with ShutdownScoreTables, so it tears down prior reporting and may uninitialize HAPINET multiplayer if a previous reporter DLL was loaded. If a lobbied connection exists it copies the provider GUID from connection offset sixteen into game storage; otherwise the existing GUID remains.

When DAT_00512c98 begins with zero, IPX, serial and modem GUIDs return four before allocation. A nonempty value bypasses that restriction. It then calls AllocScoreTables, but compares its result with **two**, although the allocator returns zero or one. Its intended return-two allocation-failure branch is therefore not reached for the explicitly returned one.

It sets g_hapinetOnlineReport to one and registers ReportGameEventCallback/ShowGameMessage through RISetCallbacks in [hapinet.cpp](../../../src/network/hapinet.cpp). It then loads reporter.dll and requires all eight decorated exports:

| Export | Use |
| --- | --- |
| `_RIEnable@4` | Enable/disable request forwarding. |
| `_RIGetVersion@4` | Version/application compatibility query with Total Annihilation. |
| `_RIInitializeEx@8` | Initialize with caller choice-count/name outputs. |
| `_RIReport@40` | Report game events and score tables. |
| `_RIReportGameChat@8` | Report game chat. |
| `_RISetCallbacks@8` | Install event/notification callbacks. |
| `_RIIntervalTimer@0` | Periodic reporting work. |
| `_RITerminate@0` | Termination. |

Missing any export unloads the DLL, clears its handle and returns two. Function-pointer globals resolved before the failure are not all reset. A nonzero version response terminates/unloads; initialization response exactly four also terminates/unloads. Other initialization responses are accepted and DLL callbacks installed. Those version/init rejection paths fall through to the final availability test, so the HAPINET flag already set to one can still make the overall result zero despite DLL rejection.

If LoadLibrary itself fails, it sets the caller's count to zero but preserves the HAPINET reporting flag, normally yielding zero. It does not clear the caller's names output. Final return is zero when either the DLL handle or HAPINET reporting flag is nonzero, otherwise four. Thus zero means an available reporting route under these flags, not proof that reporter.dll was loaded or that it returned selectable consoles. The external DLL's internal behavior, threading and choice-string ownership are not present in this source and remain outside the verified contract.

## Enable query and shutdown hazards

EnableReporter (`0x46bf00`) calls the saved enable function when its pointer is nonnull and returns its result; otherwise it returns one. IsReporterDllLoaded (`0x46bf20`) tests only the handle. These two tests are not interchangeable because shutdown clears the handle without clearing every saved function pointer.

ShutdownScoreTables (`0x46c190`) clears HAPINET reporting and both HAPINET callback addresses. With a DLL handle it uninitializes multiplayer on the game session, calls the terminate export if present, frees the DLL and clears the handle. It then releases every slot's ally block/player record, score-pointer block/scoreboard and score storage, followed by the three arrays, nulling only those top-level array globals.

The per-slot paths check whether the top-level array exists but do not check each slot pointer before reading its child pointer. That is safe for the fully populated allocation assumed by the normal path, not for all partially constructed arrays. Stale DLL function pointers remain after unloading, so a later direct EnableReporter call can attempt a released target if callers rely on pointer nonnull alone. Repeated shutdown after a completed shutdown sees null arrays/handle, but that does not repair the partial-allocation hazards or stale export pointers.

## Persistence and coverage

These external-facing structures are rebuilt snapshots of player counters, not savegame data and not an independent network replication store. Their pointer fields rely on 32-bit process layout and borrowed live strings. All six owned bodies, allocation counts and direct reporting/frontend consumers have been traced. No DLL was loaded for this documentation, and no network, game, build or installation operation was performed.
