# Network game lifecycle, protocol dispatch and shared match state

Status: finished. `network/net_game` owns 91 annotations in `0x44fd40..0x458160`: 89 in [net_game.cpp](../../../src/network/net_game.cpp), [SendLobbySyncRequests](../../../src/network/net_game_450530.cpp) and [HandleNetPackets](../../../src/network/net_game_453d40.cpp). These implement player/session coordination and application messages above [HAPINET](network-hapinet.md), [packet queues](network-packets.md) and the [condenser](network-net-condenser.md). Naval units use the same protocol as other units, with specialized motion/weapons applied downstream.

## Identity, ownership and shared storage

The game has ten ordinary player slots and a spare physical eleventh slot at sentinel ten. DPID is a DirectPlay identity, player.index is a slot index, alliance is a numbered team or sentinel five, and lobbyDataSynced is a separate machine/group assignment in the range one through ten. These are not interchangeable. Local human is type one, local CPU two, remote three, blocked four, empty zero; remote PlayerInfo.kind differentiates human one and CPU two. Most “local” operations accept both one and two.

PlayerInfo is the packed 185-byte [shared record](../../../src/network/player_info.h): map/password, resolution, DPID, kind/side/logo, host/sharing bits, memory, lobby rule/readiness/observer flags, password/shared-victory/CD flags, ping/resources/unit limit, version and map checksum. It is copied directly into packets, not field-by-field converted to a portable format. Native little-endian integers, IEEE float bit patterns, packed offsets and the original 32-bit ABI matter. Padding/unknown bytes can be transmitted with the record.

Connection flag bit zero gates most network activity; bit two marks battle context. Wider frontend mode five allows loading-phase packets, six allows battle packets and other values use the lobby mask. The one shared receive/scratch buffer is also overwritten to build outgoing messages. Calls that build/send while dispatching must therefore preserve any incoming fields they need afterward. No per-message immutable ownership abstraction protects these pointers.

## Lookups and counts

GetLocalDpid (`0x44fdb0`) finds the first active local human/CPU, or minus one. GetLocalHumanDpid (`0x44fe00`) finds the first type-one record without separately checking active. FindSlotByDpid (`0x44fe40`) and FindPlayerByDpid (`0x44fed0`) ignore empty types and reject ID minus one; failure is ten/null respectively. GetPlayerSlot (`0x44feb0`) returns pointer.index or ten. GetSlotDpid (`0x44ffd0`) rejects sentinel ten/empty type, while GetPlayerDpid (`0x450010`) rejects null/empty. Neither is a complete arbitrary-index validator.

GetHostDpid (`0x450030`) returns the first info.host slot's ID through the type filter; FindHostSlot (`0x456850`) also requires nonzero type. IsHostLocal (`0x457a50`) tests that slot with Player::IsPlayableSlot (`0x456030`), which means active type one/two, not every live combatant. No host reaches the spare slot. PickNewHost (`0x450240`) selects the highest unsigned ID among active local humans/remotes, sets its host bit and always returns ten; no named caller is shown. Removal contains its own election path.

FindFreeSlot (`0x4504f0`) chooses inactive and not blocked, or ten. FindFreePlayerId (`0x450910`) actually finds the first unused lobbyDataSynced value one through ten among active valid-index players, returning zero if exhausted. CheckDuplicatePlayerIds (`0x450980`) counts those group values and sets a global duplicate flag. Duplicate machine/group assignments can be intentional for a remote human and its CPU; this flag changes broadcast routing, not DPID allocation.

CountHumanPlayers (`0x457af0`) counts active local humans and remote kind-one; CountComputerPlayers (`0x457b40`) active local CPUs and remote kind-two; CountLocalComputerPlayers (`0x457b90`) only local type two; CountRemotePlayers (`0x457c80`) all active type three. CountActiveAIPlayers (`0x457bc0`) adds valid index and surviving-or-never-created for local CPUs. CountActiveHumanOrAiPlayers (`0x457c10`) uses those same lifetime tests but only local types one/two despite the redundant wider type check. CountCombatPlayers (`0x457cb0`) requires active, valid index, surviving-or-never-created, human local/remote kind-one and nonobserver. Names such as combat/all do not imply the same population.

## Names, slot creation and advertised descriptions

SetPlayerName (`0x450090`) copies longName into the thirty-byte display name and shortName into fullName, without guaranteed termination, then calls DirectPlay with original pointers and returns one only for exact HRESULT zero. GetPlayerName (`0x450140`) retrieves into a 1024-byte temporary and unbounded-copies both returned names; ID minus one supplies COMPUTER. Caller capacities and nonnull returned strings are trusted. AnnouncePlayerLeft (`0x450380`) skips when local rejectReason equals one, otherwise formats player plus one of eight random translated departure phrases and posts category four.

AddNetPlayer (`0x450a10`) reuses a matching slot only when not already active/remote; otherwise it allocates the first inactive nonblocked slot and marks type three. No slot, existing player, name retrieval failure and actual success all return one, so its return is not an insertion count. On success it sets up the slot, clears rejection, assigns DPID/lastHeard and increments numPlayers. Connected games broadcast every local PlayerInfo and team, send group-sync requests and flush. It can overwrite the global scratch packet while doing so. Multiplayer with more than one participant reports event two.

CreateLocalPlayer (`0x451220`) sets requested type, builds nickname or AI:name truncated at sixteen, chooses host according to the currently flagged host slot, resets color to `0xff`, keepalive bit one, ready/closed-related fields and initializes join time, maxUnits one hundred, resolution and version. It invokes HAPINET_addplayer with a 21-byte create-player payload containing password and fixed protocol shorts zero/`0x50`. Failure clears type and shows a recreate-versus-rejoin message based on host locality, but does not restore every field it changed. CPU name formatting uses the local display name and a fixed temporary.

BuildGameInfo (`0x451090`) updates local major/minor and the low four occupancy bits, exports four native dwords from the info memory/rule/resource region and builds a 32-byte advertised name: sixteen game-name characters plus fifteen map-name characters, spaces replacing NULs and final termination at byte31. This is not an arbitrary-length session description. UpdateNetGameInfo (`0x451180`) sets session flag `0x20` once started and calls HAPINET_updategameinfo; it does not clear that flag when started becomes false. CreateNetGame (`0x451540`) marks local host, builds the advertised fields before resetting numPlayers zero and calls session creation without returning its result.

## Initialization, join worker and teardown

InitNetConnection (`0x450d80`) initializes HAPINET defaults, netMode ten and packet-manager parameters two/one hundred, then initializes the selected connection only if manager setup succeeds. InitLobbiedConnection (`0x457710`) returns success immediately for an existing lobby connection. Otherwise it initializes lobby/defaults/manager, sets mission type three and derives a session name from the lobby, command-line/online override or username, optionally appending HHMMSS. The literal source writes the result to the game.net address; that provisional field view must not be silently replaced with gameName. Some sixteen-byte temporary/name copies can omit space for termination.

JoinLobbyGame (`0x451640`) borrows the player's password when required, defaults capacity ten or clamps an online configured value above one to ten, allocates a 28-byte argument/result block and starts a worker with 32 KiB stack. JoinLobbyGameThread (`0x451770`) only calls the HAPINET create-or-join adapter and stores the result. The caller waits forty seconds; timeout restores cursor, may toggle display/sleep and raises a fatal error. Non-timeout wait outcomes read the result without separately validating WAIT_OBJECT_0. The thread handle is not closed in this body. Argument storage is freed afterward; safety on a returning fatal path would depend on the worker having stopped, which the timeout branch does not establish.

JoinNetGame (`0x4517b0`) acts only for the local slot. Lobby name preference is short then long name over online default; online password can update PlayerInfo/global password according to host flags. It resets slots for lobby connection, clears readiness/color, copies resolution and resets count. It either calls the worker, treating failure as fatal, or joins the selected GUID. It then calls CreateLocalPlayer without checking its return, enumerates peers, broadcasts local info/team/group requests and requests color zero. Its separately formatted Windows/nickname name temporary is never used for player creation. EnumPlayersCallback (`0x4515d0`) performs an unused lookup and calls AddNetPlayer.

RemoveLocalPlayers (`0x451b60`) removes each active local human/CPU and shuts score tables when network-connected. CloseNetSession (`0x450dd0`) flushes queued data, uninitializes transport only when the reporter DLL is absent and clears connection bit zero. ReleasePacketData (`0x452370`) frees/nulls receive storage and applies the same connection cleanup policy. Neither explicitly frees all packet-manager retained pools here.

LeaveNetGame (`0x450e20`) flushes, removes local players/shuts scores when connected, quits DirectPlay, clears the close handler, sets endgame bit two and calls QuitApp with translated-by-caller rejection policy or null when reason zero. LeaveNetGameCallback (`0x4578f0`) follows the same latter flow without the initial forced queue flush. GetRejectReasonText (`0x452c40`) maps three closed, four password, five full, six disconnected, seven missing unit, eight version, nine watching disallowed, ten creator left; other values use generic rejected. UI and protocol can store additional reasons without distinct text.

## Routing, queues and return values

SendPacketToPlayer (`0x451bc0`) requires connected mode, a valid active local type-one/two sender with no rejection, and an active remote type-three destination with no rejection. Packet-manager mode returns QueuePacket's result, meaning queued work, not delivered acknowledgment. Direct mode requires HAPINET_sendpacket HRESULT zero, counts message/packet bytes and returns one. Invalid/local destinations return zero; this is not a general loopback API. It does not validate payload length against type before sending.

BroadcastPacket (`0x451df0`) requires a valid local sender. Without duplicate group assignments it queues on the broadcast channel or sends directly to DPID zero. With duplicates it sends to one remote per lobbyDataSynced group using an eleven-entry seen array. The read is indexed before validating group range, and marking occurs only for zero through nine, leaving group ten unmarked and potentially sent repeatedly. Per-peer send failures are ignored; disconnected mode can also return one after the initial sender validation. These return semantics cannot be called reliable delivery confirmation.

SetPacketRate (`0x4578d0`) delegates to packet-manager pacing, whose disabled/negative behavior is documented with that module. ReportPacketGap (`0x4568b0`) is empty. DefaultPacketHandler (`0x44fd40`) and RejectPacketType2/5/6/7/8 (`0x44fd50..0x44fd90`) return minus one. NetPacket::DispatchPacket (`0x44fda0`) directly indexes a 45-entry handler array using the first byte without a bound. The main HandleNetPackets path uses its own switch rather than relying on these placeholder callbacks for gameplay behavior.

## Protocol framing table and mode gates

InitPacketTables (`0x451fd0`) clears sizes one through44, sets explicit sizes/handlers/mode masks, stores sync tick and allocates 8192 receive bytes. It does not reset every mode/handler entry wholesale. Mode bits mean lobby one, loading two, battle four. The table below records only values assigned in this initializer; unassigned means prior/static content is retained. Zero/unassigned size is significant to the [frame parser](network-packets.md), not a valid empty command.

| Decimal / hex type | Bytes | Mode mask | Main meaning |
| --- | ---: | ---: | --- |
| 2 / 02 | 13 | 7 | Ping/echo |
| 3 / 03 | 3 | 7 | Reserved/default callback; also raw condenser envelope outside application parsing |
| 5 / 05 | 65 | 7 | Chat |
| 6,7,8 / 06..08 | 1 each | 7 | Probe, acknowledgment, enter battle |
| 9 / 09 | 23 | 4 | Unit creation |
| 10 / 0a | 7 | 4 | Attachment |
| 11 / 0b | 9 | 4 | Unit damage |
| 12 / 0c | 11 | 4 | Unit death |
| 13 / 0d | 36 | 4 | Weapon fire |
| 14 / 0e | 14 | 4 | Projectile impact |
| 15 / 0f | 6 | 4 | Feature damage/burn/death |
| 16 / 10 | 22 | 4 | COB script invocation |
| 17 / 11 | 4 | 4 | Unit state bits |
| 18 / 12 | 5 | 4 | Builder/completed-unit link |
| 19 / 13 | 18 | 7 | Sound |
| 20 / 14 | 24 | 4 | Unit ownership transfer |
| 21 / 15 | 1 | 6 | Startup ready/vision handshake |
| 22 / 16 | 17 | 4 | Resource or map sharing |
| 23,24 / 17,18 | 2 each | 7 | Color request/assignment |
| 25 / 19 | 3 | 7 | Speed/pause |
| 26 / 1a | 14 | 1 | Pregame catalogue sync |
| 27 / 1b | 6 | 7 | Rejection |
| 28 / 1c | 5 | 7 | Player drop |
| 29 / 1d | 9 | Unassigned | Integrity placeholder |
| 30 / 1e | 2 | 6 | Start position |
| 31 / 1f | 5 | 6 | Start-position acknowledgment |
| 32 / 20 | 186 | 7 | PlayerInfo |
| 33 / 21 | 10 | 1 | Group assignment request |
| 34 / 22 | 6 | 1 | Group assignment result |
| 35 / 23 | 14 | 7 | Directional alliance |
| 36 / 24 | 6 | 7 | Numbered team |
| 37 / 25 | 5 | 1 | No explicit main-switch case |
| 38 / 26 | 41 | 7 | Host slot-DPID table |
| 39 / 27 | 17 | 7 | Executable integrity warning |
| 40 / 28 | 58 | 7 | Economy/statistics/view initialization |
| 41 / 29 | 3 | 7 | View acknowledgment |
| 42 / 2a | 2 | 7 | Load progress |
| 44 / 2c | 3 minimum | 4 | Variable-length unit state block |

Types one, four and43 have zero sizes after this initializer and no new handler/mode assignment here; zero itself is outside the cleared range. Packet type44 has an embedded length consumed by the frame parser, so three is the header size, not a complete state snapshot. DirectPlay system messages use a native dword type and sender zero, independent of this byte table.

## Receive loop, validation and DirectPlay events

ReceiveNetPacket (`0x4534e0`) returns zero when not connected, otherwise calls packet-manager ReceiveFrame or raw HAPINET. Success counts application bytes and returns one. No-message `0x887700be` and other errors return zero; buffer-too-small `0x8877001e` updates capacity and reallocates tagged storage, retrying without an allocation-null check. Actual successful length is local, while recvPacketSize remains capacity. Dispatch does not receive a separate trusted length for each field access.

HandleNetPackets (`0x453d40`) resets all ten message counters, drains until receive returns false or type eight stops the pass, then checks duplicate groups/timeouts and returns number of received messages including ignored/system ones. It caches the receive-buffer pointer before the loop. If ReceiveNetPacket reallocates and moves it, this cached pointer remains stale, even though the global pointer has changed. Outgoing scratch writes during a switch can also alter the input bytes retained in that buffer.

Sender zero means DirectPlay system traffic, processed only for active local-human recipient. Destroy type five handles player destruction, rejecting host/local appropriately before battle and updating session info for a local host. Create type three calls AddNetPlayer, then a local host rejects closed admission, payload size other than21, version shorts other than zero/`0x50`, or a mismatching case-insensitive password when required. Payload fields are inspected before a later null check. Set-data `0x102` copies185 bytes into found info, checks forbidden observers and creates/destroys an otherwise temporary Player. Set-name `0x103` copies thirty-byte names without guaranteed termination. Set-session `0x104` copies settings for a nonhost. Native pointers inside these system messages are trusted to DirectPlay's process-local representation.

For application traffic CommandAllowed indexes g_packetModes before checking the command byte. The later range rejection uses byte <=one AND byte >=45, which is impossible. It therefore does not prevent table over-indexing. It rejects local senders, unknown/ineligible players, invalid recipient and updates accepted remote lastHeard/messages. It does not universally authenticate payload DPIDs/unit ownership against the sender or check every payload field's bounds before dispatch. Each gameplay consumer has its own additional guards; absence here must not be described as comprehensive packet validation.

## Lobby/control receive actions

PlayerInfo type32 uses the embedded DPID and copies into an active remote record, then checks duplicate groups. Type38 copies ten DPIDs into the host-slot table without an explicit host-sender test in this branch. Type23 lets local host choose/approve requested color; type24 installs recipient color and republishes local info/teams/group requests. Type35 resolves embedded endpoints, updates local directional relations through SetAlliance where applicable and marks/refreshes lobby/allies UI; it also writes the sender endpoint's outgoing ally byte. Type36 assigns the embedded player's numbered team. Type27 calls RejectPlayer with embedded target/reason.

Type28 announces disconnection, runs drop/remove/rebroadcast logic and sets failure/endgame flags when the dropped ID matches the first local human. The drop helper overwrites the shared buffer used by its caller. Type30 stores recipient startPos and sends type31 with recipient DPID; type31 marks the embedded target's assignment acknowledgment. Chat type5 posts incoming category eight only for a local-human recipient. Type39 formats an integrity warning and posts it twelve times; it does not itself reject or repair an executable. Type29 calls a packet-manager integrity no-op when enabled.

Type6 replies with seven from the first active local sender; seven sets keepalive flag zero; eight sets battle bit and ends this receive pass. Type25 uses byte one to choose SetGameSpeed(byte two, no rebroadcast) or direct pause bit assignment. Type26 calls UnitSync only for an active local-human recipient; that object interprets its fixed catalogue payload. Type42 stores sender progress. Unhandled accepted types still contribute to lastHeard/messages and receive totals.

Group requests type33 are handled only with a local host. An unflagged request broadcasts an existing nonzero group; a flagged request inherits the referenced controller's group or allocates the first free one, installs it and broadcasts type34. Type34 installs that value on the embedded player's record. SendLobbySyncRequests (`0x450530`) skips wider mode six and visits valid players whose group is zero: local humans request an independent group, CPUs request the local controller's group, remotes ask about their existing assignment. When host is local, local participants are simply assigned one. Some host checks inspect the spare slot before testing for sentinel ten. This protocol groups machine-owned players; numbered alliances remain separate.

## Live units, construction, weapons and effects

SendNewUnit (`0x456050`) broadcasts type9 with definition index, numeric unit ID, three fixed coordinates and three angles. There is no unit-name string or construction mesh in this message. Receive type9 calls CreateUnitFromPacket with sender slot, while44 calls ReceiveUnitStates with sender Player. Definition agreement is established separately through UnitSync. [Unit position replication](units-unit-position.md) explains variable state records, quantization and attachment; [unit lifecycle](units-units.md) explains create/live ownership.

Types10/11/12 delegate attachment, damage and death. Types13/14 delegate weapon-fire and impact to [weapons](weapons-weapons.md), where projectile type, target and ownership checks control replay and local side effects. Type15 maps subtype `0xfd` to nonviolent feature removal, `0xfe` to burning, `0xff` to violent feature removal, otherwise uses subtype as weapon-definition index for DamageFeature at unsigned-short cell x/z. [Features](map-features.md) owns the resulting fire/debris/economy effects.

Four script senders (`0x456110`, `0x456190`, `0x456200`, `0x456290`) broadcast a fixed22-byte type16: unit ID, script index, byte arg count and four integers. Name variants perform script lookup even before the network-connected guard. No-argument variants zero all four arguments. None validates count against four or missing script index here. Receive looks up numeric ID and, if live flag `0x10000000`, starts the script by index with those values. A zero ID yields null followed by a dereference in this branch; arbitrary large IDs are not independently bounded. [COB interpreter](units-cob.md) defines script execution and effects such as emit-SFX.

Type17 applies all requested unit bits through SetStateBits(mask,true) followed by SetStateBits(complement,false) for a live record. It does not compare a generation handle. BroadcastBuilderLink (`0x4560c0`) sends type18 with completed-unit ID then builder ID. Receiver calls FinishConstruction(builder, constructed). [Unit lifecycle](units-units.md), [scripts](units-unit-scripts.md) and [construction placement](ingame-build-placement.md) own completion bookkeeping, script calls, visibility/wireframe transitions and shipyard behavior. The packet carries references, not pre-rendered wireframe vertices or a model construction percentage.

Type19 plays a sound globally when flag nonzero, otherwise positionally using the vector at byte six. Type20 resolves live unit and embedded new-owner DPID, transferring only when new owner is active local human/CPU. A missing owner yields null and is passed to IsConnected, which dereferences it, a retained failure path. Numeric unit IDs, script indices and sound IDs rely on synchronized resources and consumer guards; they are not unrestricted safe object handles. [Ownership commands](units-unit-commands.md) describes transfer mutation and capacity consequences.

## Colors and directional alliances

IsColorFree (`0x452570`) rejects requested color outside zero/nine or255, builds a ten-byte owner table from nonzero types except the requesting DPID and returns vacancy. Existing non255 colors are used as indices without checking below ten. AssignPlayerColor (`0x4523e0`) cycles at most ten candidates, wrapping candidates >=ten to zero and excluding nonempty/nonblocked other players' colors. Negative starting candidates are not lower-clamped. Exhaustion can leave the outgoing color byte uninitialized. Local target writes directly; a remote send updates the host's cached target color only when packet-manager mode and send result nonzero.

RequestPlayerColor (`0x4526c0`) lets local host accept a free color or run assignment, otherwise writes type23 to the scratch buffer and sends host/broadcast when no host, flushing queues. A missing host does not synchronously produce a valid color. [Multiplayer UI](frontend-multi.md) has a different logo-choice candidate rule; the assignment routine determines final acceptance.

SetAlliance (`0x452960`) resolves embedded source/destination and changes outgoing/incoming relation bytes according to which endpoint is local. CPUs, remote CPU destinations and extra/force mode can make the relation reciprocal. A remote destination gets type35 with endpoint DPIDs, value and force. It reports multiplayer event four. SendAlliance (`0x452b70`) always builds/sends that message without the local mutation logic. BroadcastAllyTeam (`0x452bd0`) requires active local ownership and sends type36 with DPID/team, flushing when queued. A directional alliance, reciprocal alliance, numbered team and shared-victory bit remain separate states.

## Removal, rejection and timeout policy

BroadcastPlayerLeft (`0x452800`) builds type28 in scratch storage, removes immediately for a remote or when loading not begun/already finished, then broadcasts from local. RemovePlayer (`0x452cc0`) requires active type one/two/three and valid index, clears local players' relation bytes for its slot and kills its units. Before battle it removes local DirectPlay player and clears slot/type/activity/DPID/group; in battle it retains local records but clears remotes. It decrements numPlayers, clears CD-present and outgoing alliance bytes and reports event three. In battle a removed host causes highest unsigned active human/remote DPID to gain host, without a universal prior-host-bit clearing pass. Repeated removal of a retained local record can still encounter it active; this body is not generally idempotent.

RejectPlayer (`0x453010`) builds type27 in shared storage. Local-human rejection loops all local players, broadcasts each ID, removes the original target during that loop and stamps reasons. Local CPU removal handles only that slot. A remote human rejection removes every record sharing its narrowed group value, without an additional active/type guard on the inner group comparison; remote CPU rejects only itself. It sets the original target's reason afterward. Send results do not roll back removals or reasons. Host admission, GUI authority and message routing supply context; this helper does not itself test “caller is host.”

CheckPlayerTimeouts (`0x453c20`) skips when UI-option bit zero is set. Pause bit zero instead refreshes a global baseline. A remote is timed out when now minus max(lastHeard,baseline) exceeds timeoutSecs times thirty. It opens a prompt only if all timed-out remotes belong to one group; mixed groups suppress/close the prompt, avoiding immediately blaming one peer when the local machine may have lost connectivity. OpenTimeoutDialog (`0x453a50`) cannot retarget an already-open prompt, except ID minus one closes it; otherwise it resolves a player, allocates2560 chatter (initially clears only1920), binds visible lines and callbacks.

UpdateTimeoutDialog (`0x453640`) refreshes every strictly passed two-tick deadline, clears/rebuilds packed chat and displays timeoutSecs plus120 minus elapsed/30. At that extended deadline it rejects reason six/closes. The display calculation uses lastHeard directly, unlike the pause-adjusted trigger. Missing/inactive target has no explicit automatic-close branch here. HandleTimeoutDialog (`0x4538f0`) sends/refocuses chat, supports immediate REJECT and frees/nulls chatter on cleanup. SendProbe (`0x453320`) merely sends type6 to one or all targets; heartbeat and timeout checks are separate policies.

## Chat, heartbeat and ping clocks

SendChatPacket (`0x453360`) stores65-byte type5 with at most64 text bytes and no explicit terminator. Leading plus or chatMode zero broadcasts; mode three iterates selected recipients; other modes target remote outgoing allies for one or nonallies for two. It uses first type-one DPID, not an explicitly passed speaker. When no sends occur its local return value is uninitialized; callers generally ignore it. [Message HUD](network-net-chat.md) handles local formatting, sound, filtering, ring storage and unit jumps.

SendNetHeartbeat (`0x456310`) runs only after now minus stored tick is strictly greater than60, advancing the stored deadline by60 rather than resetting to now. For each local player it flushes, sends type2 with GetTickCount milliseconds and echo zero under temporarily disabled guaranteed delivery, flushes/restores policy, sends probe6, requests a missing color and, for host, broadcasts ten-slot table38. It then republishes local info/team/group requests and marks lobby dirty. Sixty display ticks are normally two seconds; delayed repeated calls can catch up one interval each.

HandlePing (`0x4565a0`) stamps first echo with GetTickCount, sends it from recipient transport DPID to the original target and temporarily disables guaranteed packets similarly. An echoed packet whose target is a local playable slot updates the actual sender's ping to current milliseconds minus sendTick. Lookup failures reach the spare slot rather than being fully rejected. It does not subtract remote processing time using echoTick. Queue flushing reduces local delay but does not establish synchronized clocks or guaranteed latency bounds.

## Loading, start positions and startup handshakes

SendLoadProgress (`0x456de0`) averages six byte stage percentages with integer division, writes each local player's progress and broadcasts type42. It does not transmit per-stage detail. AssignStartPositions (`0x4568c0`) checks host locality through the spare-aware slot helper. A local host builds a once-only ten-slot array: fixed starts assign consecutive positions to nonobserver valid participants, random starts shuffle candidate positions, with the two-player shuffle conditional on an extra random bit. Others get minus one, transmitted as byte255.

The host waits for each active remote's shareVisionReady and startPosAssignAck; a client waits only shareVisionReady. Host sends type30 repeatedly until each slot acknowledges, assigning local fields/acks directly. Type31 identifies the recipient to set its acknowledgment. Nonhost always broadcasts type21 from its local participants; host broadcasts21 only when its computed all-peer condition is satisfied. Receiver21 records shareVisionReady only after loading flag bit two is set. The return can remain false while messages make progress, and the load coordinator can call again; it is not a one-shot synchronous setup. [Game loading](game-game-load.md) documents the thread/loading bits and eventual start gate.

## Resource messages and actual channels

SendShareMetal (`0x456ee0`) emits type22 subtype one; SendShareEnergy (`0x457050`) subtype two. Both require active valid-index surviving-or-never-created endpoints and carry DPIDs plus the amount's raw dword, normally float bits supplied by player services. SendShareMapInfo (`0x4571c0`) subtype three rejects only sentinel-ten arguments, carries zero amount and delegates send validity. Receive resolves both IDs: subtype one calls provisionally named TransferMetal, which actually credits energy; subtype two calls TransferEnergy, which credits metal. These names agree across send/receive but are reversed relative to resource fields, as documented in [player transfers](game-players.md). Map subtype calls ShareMapInfo, not an economy credit.

UpdateResourceSharing (`0x457d30`) acts only connected. At exact gameTick multiples60 it chooses a destination separately for metal/energy, requiring remote human, outgoing alliance, active valid index and surviving-or-never-created. Each qualifying candidate with less stock than the donor replaces the previous choice; it selects the last qualifying slot, not the globally neediest peer despite the source comment. Metal sends one third of surplus above shareMetal; energy sends one half. Each is capped by recipient capacity minus stock, without lower-clamping that capacity gap. The correctly matched provisional helper is called with spend/broadcast flag one. At exact multiples450, enabled shareMapping sends subtype three to qualifying allies. Repeated calls on the same tick can repeat transfers; scheduling the updater is a caller responsibility.

## Economy snapshots and view acknowledgment

SendPlayerEconomy (`0x4573d0`) builds58-byte type40 from local nonrejected players: four widened short kill/loss counters, four raw resource/storage float dwords and six cumulative doubles narrowed to floats, plus caller share flag. A specified nonrejected target is attempted directly; otherwise only nonrejected remote human-kind peers receive it. This is an observation/initialization message, not a resource transfer or shared deterministic economy simulation.

HandlePlayerEconomy (`0x457540`) rejects null/rejected sender. It copies fields only when no active local player already has shareVision for that slot. Counter dwords are read as low shorts and float totals are widened back to doubles, so the wire round trip loses double precision. A nonzero share flag sets local shareVision, sends type41 ackLos one and ackMapping based on existing shareLos, and may reply with its own flagged snapshot unless victory/LOS conditions stop it. Type41 records recipient shareLos and optionally shareMapping; acknowledgments do not necessarily mean the names' everyday visual interpretation. They are per-slot handshake tables with separate map/LOS work elsewhere.

BroadcastPendingViewState (`0x4572a0`) loops local nonrejected participants that have created units and remote targets missing relevant share tables, sends flagged snapshots and returns zero whenever it attempts another send. It sleeps250 ms even when no work remains. A return one means no pending send found in this pass, not transport-confirmed delivery. Remote CPUs can participate in the shareVision branch even though the ordinary broadcast snapshot chooses remote humans.

## Coverage and validation boundaries

All91 annotations, both split companions, protocol layouts, mode/length initialization and direct subsystem handoffs have been traced. Shared-buffer aliasing, native ABI assumptions, lookup sentinels, group-versus-team distinctions, no-op integrity/gap helpers, literal resource channels and failure behavior are explicitly documented. The linked unit, weapon, feature, script, sound, loading and UI references explain the delegated effects. No network sessions, game commands, builds, installations or source listings were executed or embedded for this writeup.
