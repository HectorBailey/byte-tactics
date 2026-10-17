# Pregame unit catalogue, compatibility, restrictions and report dispatch

Status: finished. `network/unit_sync` owns 74 annotations in `0x46c620..0x470a40`. Sources are [unit_sync.cpp](../../../src/network/unit_sync.cpp), [finish/apply](../../../src/network/unit_sync_46ca60.cpp), [sequenced send](../../../src/network/unit_sync_46cc10.cpp), [destruction](../../../src/network/unit_sync_46d1a0.cpp), [incremental processing](../../../src/network/unit_sync_46dad0.cpp), [player-record lifetime](../../../src/network/unit_sync_player.cpp) and three container insertion companions linked below. The [shared header](../../../src/network/unit_sync.h) exposes the caller contract.

## Scope and distinction from live state synchronization

UnitSync reconciles **unit definitions available to a multiplayer setup**, including definition identity, associated script/resource checksum, permission and per-type limit. It does not replicate the position, health or order of each live unit. Those use unit-position/order/net_game systems. This address group also owns online report dispatch and a reusable sequenced-packet helper, which are explained separately rather than silently counted as catalogue behavior.

The 104-byte manager holds an unsigned-checksum-keyed map, a vector of per-remote-player records, an insertion-ordered list of changed keys, sequencer state and queues, direct/authority mode, received-packet count, incremental checksum cursor and disabled flag. Local names x/y/w/h describe the map entry's storage layout, not geometry:

| Entry field | Meaning |
| --- | --- |
| x, 32 bits | Unit FBI/definition checksum, also the map key. |
| y, 32 bits | Cached script/resource checksum used for compatibility. |
| w, 16 bits | User permission/allowed state. |
| h, 16 bits | Availability/compatibility across checked participants. |
| limit, 32 bits | Per-type maximum, with ordinary initial values zero or minus one. |

One 92-byte per-player record contains network ID, parallel vectors of reported keys/checksums, expected definition count, sent-update count, acknowledged count and its own sequencer state/packet vectors. These arrays/maps/lists are owned allocations; external UI pointers and iterators can become invalid when vectors grow or records are removed.

## Construction, reset and completion

CreateUnitSync (`0x46c8e0`) constructs a manager and assigns the game sync pointer without first freeing an old one. UnitSync constructor (`0x46d040`) starts sequencer/count/cursor/disabled state at zero, stores the caller's direct mode and inserts every definition index one through count minus one into the map. Permission starts one, compatibility starts direct narrowed to a short, script checksum starts zero, and limit is zero when UnitDef.flags2 bit sixteen is set, otherwise minus one. Equal FBI checksums share a map key and later definitions overwrite its value; this map is not indexed by unique local unit IDs.

ResetEntries (`0x46d2e0`) rewrites the same defaults for current definitions. It does not clear absent old map keys, player records, acknowledgments, changed-key list or incremental cursor, so it is not a full handshake restart. Zero definition index is intentionally omitted. The unsigned-short loop relies on a valid bounded definition count.

DeleteUnitSync (`0x46c920`) safely tests the top pointer, destroys owned containers and nulls it. FinishUnitSync (`0x46ca60`) first applies negotiated state to definitions, then deletes/nulls; its primary body does not check null before ApplyToUnitTypes. Repeated caller invocations therefore need external guarding. The out-of-line destructor (`0x46d1a0`) and generated player deleting destructor (`0x470300`) implement the same member ownership: queued-packet vectors, changed-key list, player vector with its four vector allocations, and map nodes/sentinels. They do not own the global UnitDef array.

## Fourteen-byte catalogue packet

Every catalogue packet has outer type `0x1a` and this packed layout:

| Offset | Field |
| --- | --- |
| 0 | Outer type byte. |
| 1 | Subtype byte. |
| 2 | Four-byte sequence ID, ordinarily overwritten with zero by the live catalogue adapter. |
| 6 | Four-byte unit key or subtype-specific integer. |
| 10 | Four-byte value, or enabled byte, compatibility byte and signed sixteen-bit limit. |

The ordinary subtypes are zero, initial/control notice; one, expected unit count; two, unit key/checksum report; three, permission/compatibility/limit update; four, received-update acknowledgment count. Subtype `0x65` belongs to the sequencer's resend-request interface, not the live ReceiveSyncPacket dispatch described below. Limits therefore narrow from manager integers to signed sixteen-bit values on the wire and widen on receipt. Arbitrary large limits are not preserved losslessly.

SendSyncPacket (`0x46d4c0`) routes to the supplied target ID in direct mode, otherwise the host, sets packet ID zero and calls SendPacketToPlayer with fourteen bytes. It does not test disabled itself. SendSyncMessage (`0x46d530`), SendSyncMessageTo (`0x46d5b0`) and SendEntryTo (`0x46d630`) skip sending when disabled; the target-specific variants increment the player's sent count after attempting a send, without checking transport success. The no-target SendSyncMessage calls the common Send with null target, which is safe only in nondirect mode; direct mode would dereference it. Parameters called unused do not alter delivery policy.

ReceiveSyncPacket (`0x46d500`) overwrites incoming ID with zero and dispatches only subtypes below one hundred. It does not use PacketSequencer.ReceiveSequenced or validate a packet length/type field itself. Its gameplay caller in [net_game_453d40.cpp](../../../src/network/net_game_453d40.cpp) checks sync exists and recipient is an active local player before invoking it; source-player identity is passed as a slot byte. Further sender/map/list invariants remain the caller's responsibility.

## Authority/direct receive and compatibility calculation

HandleSyncPacket (`0x46d6c0`) returns immediately when disabled, otherwise increments pendingPlayerCount for every handled packet, including duplicates or unrecognized subtypes. In direct mode it searches the player vector for the ID in the supplied game slot. It does not reject an end iterator before cases one/two/four dereference it, so a packet arriving before player-record discovery or from an unsupported slot can violate that invariant.

Subtype one stores expected count. Subtype two scans already reported IDs; a duplicate key returns without adding another checksum, but the received counter was already incremented. A new key/checksum is appended to parallel vectors and passed to CheckUnitAvailable. Subtype four raises ackd to the maximum unsigned value observed, rather than requiring exact sequence-by-sequence acknowledgments. Zero and three do nothing further on this authority path.

CheckUnitAvailable (`0x46d970`) ignores disabled/missing local map keys. With a nonzero supplied checksum, it lazily computes the local definition's script checksum if the map cache is zero and compares it to the supplied value. It then requires every tracked player to have reported the key. When a nonzero checksum is involved, remote versions at least 2, or version 1 with minor at least 2, also require the stored parallel checksum to equal that value; older versions require key presence only. The local checksum is produced by [data-file checksumming](game-data-files.md), which includes associated resources rather than only a live COB interpreter state.

If FindPlayerByDpid returns null during this scan, the loop breaks without explicitly changing a still-true compatibility result. Thus a stale/missing player record can terminate checking early. The final Boolean goes to h and NotifyEntryChanged broadcasts/queues the result even when h did not change. Definition-key collisions and duplicate reports are not resolved by a filename list or a stronger identity object here.

NotifyEntryChanged (`0x46d860`) returns when disabled. Direct mode looks up the map entry, sends subtype-three state to every tracked player and increments each sent count, without a missing-map check at this call site. It then appends the key to the changed list only if absent, in either mode. The queue holds unit keys, not player IDs despite a provisional source comment. Repeated changes while a key is pending coalesce to one eventual latest-state read.

## Client receive and incremental reports

Nondirect HandleSyncPacket processes subtype three only: it builds/replaces the map entry from wire key/permission/compatibility/signed limit, clears local checksum cache to zero and queues the changed key. A previously unknown key can be inserted. Other subtypes still increment received count and thereby drive the response handshake even though no map mutation occurs.

ProcessSync (`0x46dad0`) returns when disabled. On the authority/direct side it reconciles its player vector with active game type-three records whose remote PlayerInfo.kind equals one, removing others and adding newly discovered remote humans. A new record starts empty with zero expected/sent/ackd, sends subtype zero and increments sent. If the participant set changed, every map entry is rechecked using checksum argument zero, which tests membership without the newer-version checksum comparison. The erase loop decrements an iterator after erase before incrementing again, relying on old-library pointer behavior at begin boundaries rather than a modern iterator-safe pattern.

On the nondirect side no report work occurs until at least one handled packet has raised pendingPlayerCount above zero. At checksumProgress zero, it waits for a host ID other than minus one, sends subtype one with unitDefCount minus one, sets cursor one and returns. Later calls compute/report up to four definitions per invocation using subtype two. Once the cursor reaches count, it repeatedly sends subtype four with the cumulative pendingPlayerCount. That count is not reset after acknowledgment, so it represents cumulative received notices, not pending remote participants despite the field name.

The checksum/handshake progression has no local wall-clock timeout, retransmission scheduling or transport-result test. [Battle-room update](../../../src/frontend/multi_44a680.cpp) calls ProcessSync each frame, and the [frontend coordinator](frontend-frontend.md) decides when to finish/apply it. Single-call progress is deliberately bounded to four checksum reports after the initial count message; local checksum caching can make repeated definition work cheap but does not change the count protocol.

## Completion tests and status text

GetSyncStatusText (`0x46df40`) returns null outside direct mode. In direct mode it returns the first issue in player-vector order: expected count zero, reported ID-vector size differing from expected, or sent count differing from ackd. Count mismatches format into one shared global text buffer; otherwise it returns literal OK. It does not apply all the skip rules used by the actual completion tests, and it does not gate on disabled.

AllPlayersSynced (`0x46e000`) returns one immediately for disabled or nondirect mode and for an empty player list. Otherwise it skips missing live Player lookups and active local/remote computers, and requires nonzero expected count, exact reported-key count and exact sent/ackd equality for the rest. Acknowledgment greater than sent therefore still fails the equality test even though subtype-four reception only raises the stored maximum.

IsPlayerSynced (`0x46e0b0`) returns zero outside direct mode before considering disabled. Within direct mode, disabled, absent game player or an active computer returns one. An ordinary matching sync record must meet the same count/equality tests; no matching record returns zero. These two methods intentionally differ for nondirect clients and absence. An empty catalogue count can never pass the nonzero expected test for an ordinary remote human.

## Restriction editing, change events and final UnitDef state

PopChangedEntry (`0x46e280`) returns zero for an empty change list, otherwise finds the front key, copies its current map value and pops the key. It assumes that key still exists. GetUnitEntry (`0x46e330`) uses the definition's FBI checksum, copies the entry and returns w and h both nonzero; it does not check end iterator. The provisional Unit pointer view actually addresses the key field at definition offset `0x13e`.

ToggleUnitAllowed (`0x46e3c0`) flips w between zero/one; DisallowUnit (`0x46e450`) clears it; AllowUnit (`0x46e4d0`) sets it. Each notifies and returns w-and-h eligibility, with no missing-key guard. SetUnitLimit (`0x46e550`) does guard a missing key, stores the integer limit and notifies. These editing helpers can mutate map fields while disabled even though notification then returns, unlike several fully gated processing methods.

ApplyToUnitTypes (`0x46e160`) skips when disabled. It scans local definitions one onward, temporarily enables write protection changes for each definition, and sets flags1 bit twenty-three to w-and-h and maxThisUnit to negotiated limit. A missing map entry clears availability and assigns limit zero. It restores read-only protection after every entry. It does not set permission from the limit itself, change construction menus directly or copy remote definition assets; existing type availability/build consumers observe those fields afterward. Names, FBI contents and scripts remain local resources.

## Independent sequenced-packet helper

PacketSequencer constructor (`0x46cbe0`) starts lastSent/current/max zero and empty sent/held vectors. SendUnsequenced (`0x46cec0`) ignores its own object state, sets packet ID zero and sends fourteen bytes. SendSequenced (`0x46cc10`) increments lastSent, writes that ID to caller packet, appends a retained value copy and sends it to the numeric target passed through its pointer-shaped parameter.

ReceiveSequenced (`0x46cef0`) answers subtype `0x65` by finding the requested ID in sentQueue and resending its stored copy. For ordinary packets it raises observed max. Exactly current plus one is delivered to UnitSync.HandleSyncPacket and then contiguous held IDs are delivered in order. IDs at or below current are ignored. A later ID is appended to heldQueue and every missing ID from current plus one through max generates a `0x1a/0x65` request. Those requests initialize type/subtype/ID but leave the remaining eight packet bytes uninitialized before transmitting fourteen bytes.

Held duplicates are not deduplicated before append, consumed held entries are not removed, and sent history is not pruned by these bodies. There is no timeout, maximum-gap bound or wrap handling, so extreme sequence gaps can trigger large searches/request loops and growing storage. The in-order branch returns after draining its contiguous run without separately issuing new requests at that point. This helper is available in the source, but the currently traced catalogue ReceiveSyncPacket path resets IDs and does not invoke it; do not attribute its resend mechanism automatically to every catalogue message.

PacketSequencer assignment (`0x470560`) copies counters and deep-assigns sent/held vectors. UnitSyncPlayer copy construction (`0x470390`), destruction (`0x46ded0`) and SyncPlayerRecord assignment (`0x470040`) similarly own four independent vector allocations and copy counters/sequencer state. They do not share mutable packet-vector storage after successful deep copy.

## Reporting functions sharing the module

ReportGameEvent (`0x46c620`) returns four when neither HAPINET reporting nor reporter.dll is enabled, one when required top arrays are missing. Otherwise it copies session GUID, reads the borrowed created-lobby interface, and for event one copies the session's seventeen-byte name into retained report identity, removing trailing spaces from indices fifteen down toward one. It populates [score tables](game-statistics.md).

HAPINET reporting refreshes g_reportFlags for events one/six/seven using query arguments one/two/three; only low two flag bits permit an RIReport call. Nonzero RIReport result disables that route. The current [HAPINET](network-hapinet.md) query returns zero, so ordinary refresh suppresses that route. An active reporter.dll separately receives the event, GUID/provider/lobby/name, mission-name pointer, local index, participant count and table arrays, regardless of HAPINET success; its return is ignored. The wrapper then returns zero.

ReportGameChat (`0x46c810`) proceeds for HAPINET report flag eight or a loaded reporter DLL, refills tables, calls enabled routes, disables HAPINET on its nonzero result and returns the DLL result when present, otherwise zero. It does not repeat ReportGameEvent's top-array null guards before filling. ReportIntervalTimer (`0x46c8b0`) calls the saved DLL timer only when the handle exists. FUN_0046c8c0 and FUN_0046c8d0 (`0x46c8c0/0x46c8d0`) are empty. None of these owns DLL initialization or table allocation, which remain in game/statistics.

## Container support included in the ownership audit

The remaining annotations implement the map/vector/list machinery required by these owners. Existing MSVC 5 XTREE, VECTOR and LIST headers were read locally for generated methods, without compilation. They do not introduce extra gameplay messages:

- Map Begin/FindExact/LowerBound (`0x46e880`, `0x46e9b0`, `0x46fe60`) use unsigned key order and end/head sentinel for a miss. Init (`0x46f720`) creates a red head and shared black nil node with reference counting under the library lock. Insert, node allocation, left/right rotations, iterator increment/decrement, single/range erase and recursive subtree destruction (`0x46ea10`, `0x46ef50`, `0x46f1e0`, `0x46f6d0`, `0x46fb80`, `0x46feb0..0x46ff90`, `0x46e890`) maintain the red-black tree, node count and extreme links. Unique insertion returns existing nodes for equal keys. Map subscript can insert a default temporary before a caller overwrites it; that temporary is not a validated catalogue entry.
- Changed-key list erase/postincrement (`0x46eb60/0x46fac0`) unlink/free one twelve-byte list node and return/advance iterators; they do not notify peers. InsertResult construction (`0x46fad0`) stores the resulting node and new/existing Boolean.
- [Integer-vector insert](../../../src/network/unit_sync_46e640.cpp), [packet-vector insert](../../../src/network/unit_sync_46eba0.cpp) and [player-vector insert](../../../src/network/unit_sync_46f7a0.cpp), at `0x46e640/0x46eba0/0x46f7a0`, grow by old size plus max(old size, inserted count) when needed. They construct/copy values, shift existing tails backward in place when capacity allows, destroy old elements and release old storage after relocation. Player elements deep-copy their member vectors.
- InitTaggedVector (`0x46e5c0`) copies allocator byte and clears begin/end/capacity pointers. Temp/checksum vector destructors (`0x46e5e0/0x46e610`), player-vector destruction (`0x46eaa0`), packet/integer uninitialized copies/fill/destruction (`0x46e870`, `0x46faf0`, `0x46fb40`, `0x470290`, `0x4702a0`) perform element lifetime/storage work. Trivial destruction and NopChecksumEntryDtor (`0x470030`) add no message handling.
- Capacity/Size (`0x470250/0x470270`) return element counts from pointer differences, zero for null storage. CopyDwordRangeOverwrite (`0x4702d0`) performs forward assignment and returns destination end. Packet-vector size/assignment (`0x470770/0x4707a0`) count fourteen-byte values and reuse capacity or allocate/copy as needed. They are not packet counts delivered or acknowledged by the network.

## Coverage and limits

All 74 owned annotations across nine implementation files, direct receive/room consumers and relevant existing library templates have been accounted for. Progress/status/eligibility are different measures, and disabled/direct/absence cases are described separately. The unit catalogue is reconstructed pregame and applied to local definitions; no save-account format or live-unit replication is inferred from it. This work used existing source only, without network/game execution, builds, dependency installation or embedded source listings.
