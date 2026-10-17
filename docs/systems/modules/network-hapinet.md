# HAPINET DirectPlay transport, sessions, providers and lobby interfaces

Status: finished. All 44 entries of `network/hapinet`, `0x4c9530..0x4caa40`, are implemented in [hapinet.cpp](../../../src/network/hapinet.cpp). This is the game's thin DirectPlay/lobby wrapper, not its gameplay message dispatcher, compression codec or online.dll adapter. This reference describes the existing COM calls and numeric flags from source without claiming externally executed provider behavior.

## Network object and result conventions

The packed network object holds short session/player name buffers, application/provider GUIDs, an 80-byte session description, window handle, retained received sender/recipient IDs, caller-supplied enumeration arrays, two DirectPlay references, two lobby references, a connection-settings allocation, compression setting, limits and enumeration counts. The session description retains pointer-valued names/passwords rather than deep copies of all strings. Frontend/game code embeds this object at game offset twenty; pointers into it remain borrowed.

There is no universal HAPINET success convention. Packet send/receive and several direct player/address wrappers return HRESULT-style zero success and numeric failure. Session/player setup helpers commonly convert success to one and failure to zero. Enumeration returns count or minus one in one path, Boolean in others. Interface creation/release expose HRESULT/reference-count values. Callers must follow each wrapper's contract rather than test every nonzero result as success.

HapinetTrace (`0x4c9740`) is empty. Trace-looking calls in these methods do not create log files or visible diagnostics. HAPINET_GetDPErrorString (`0x4c9530`) returns static English strings for success and 28 recognized error values, covering unsupported/generic, memory/parameter/interface errors, session/player problems, buffer sizing, busy/timeout/cancellation and lost sessions. Unknown codes use a fixed undocumented-return-value description. GetEnumSessionsErrorText (`0x4c9750`) selects the most recent stored enumeration error, takes the description after the first space-hyphen-space and translates that suffix; fallback is generic text. These are borrowed string pointers, not allocations for the caller to free.

## Packet transport and guaranteed mode

HAPINET_guaranteepackets (`0x4c9790`) replaces a process-global integer and returns its previous value. Ordinary HAPINET_sendpacket (`0x4c97b0`) passes Send flags one when that global is nonzero, otherwise zero. HAPINET_sendpacketguaranteed (`0x4c9800`) always passes one. Both forward sender, recipient, data and size unchanged and return `0x887700aa` when the queried DirectPlay interface is absent. They add no header, queue, retransmission logic or size validation of their own.

HAPINET_receivepacket (`0x4c9840`) calls Receive with flag one and addresses of the network object's from/to fields. The caller supplies data and in/out size; the transport owns buffer-too-small/no-message semantics. No interface returns the same no-connection error. Stored from/to values are not precleared before a failing receive. [Condenser](network-net-condenser.md) and [packet queues](network-packets.md) sit above these calls and make their own decisions about system sender zero, envelope decoding, caching and command splitting.

## DirectPlay initialization and teardown

HAPINET_initmultiplaydefaults (`0x4c9c20`) sets maximum players sixteen and enumeration timeout 1500. It does not zero the whole object or establish interfaces.

HAPINET_initmultiplay (`0x4ca5d0`) disables global guaranteed sends, gets the display window handle, stores application/provider GUIDs and clears existing interface/lobby/connection pointers and connection size. It calls DirectPlayCreate for the base reference, then QueryInterface for IID_IDirectPlay3A into the queried reference. It returns one only for exact zero query result. It does not first release overwritten live references and does not release a successfully created base reference when QueryInterface fails. Correct use relies on prior uninitialization and later cleanup for partial setup.

HAPINET_initconnection (`0x4ca590`) calls the queried interface's slot 38 with the first pointer-sized integer from the supplied connection record and flag zero, returning whether HRESULT is nonnegative. This source's provisional comment calls it a session open, but the call is distinct from the slot-24 Open used by create/join. It has no interface/null connection check and does not modify the retained selected-provider record itself.

HAPINET_uninitmultiplay (`0x4c9b70`) closes/releases the queried DirectPlay reference, then closes/releases the base reference, releases the queried lobby and created lobby references, and frees the connection-settings allocation. It nulls each released pointer and clears connection size when freeing that allocation. It ignores close/release failures and does not free caller enumeration lists or their per-connection data, clear GUID/name fields, reset guaranteed mode or zero last sender/recipient. With all its owned pointers cleared, repeated normal calls avoid those releases, but that does not address arbitrary overwritten/aliased references from incorrect setup.

## Session creation, updates and join

HAPINET_createnewgame (`0x4c9920`) requires a queried interface, copies sixteen session-name bytes to the network object and zeros the local description. It sets size eighty, configured maximum, application GUID, borrowed name/password pointers and four caller user values. Slot 24 opens with flag two. It then probes/retrieves an actual session description and copies back the instance GUID when available, even though that retrieval is attempted after both successful and failed Open. The function's result depends on Open alone: one for zero, zero otherwise. A successful Open with failed description retrieval can therefore retain incomplete local metadata.

HAPINET_updategameinfo (`0x4c9890`) copies the short name, sets description size/name/four user dwords and calls SetSessionDesc with flags zero. The parameter named data is unused. It preserves other description fields rather than zeroing the structure. Both helpers' sixteen-byte strncpy calls do not force termination, and description name points to the caller string rather than necessarily the retained short copy.

HAPINET_joingame (`0x4c9fd0`) zeros the description, fills size/application/requested instance GUID and calls Open with flag one. Success probes/allocates/retrieves the negotiated description, updates instance GUID and copies session name with lstrcpyn limit seventeen. The local name region has a sixteen-byte declared name plus the following byte in the packed view. Return one follows successful Open even if retrieval/allocation fails. No password is set by this normal join body; lobbied/password flows are separate.

HAPINET_quitgame (`0x4c9f90`) closes the current queried interface when present. A failing Close returns zero; absent interface or successful close returns one. It does not release COM references or clear all network fields, so quitgame and uninitmultiplay have different lifetime effects.

## Enumerating available games

HAPINET_getgames (`0x4c9e50`) resets collected count, returns minus one without an interface, and supplies an application/instance-filter description plus caller-owned result storage to EnumSessions. Flags are `0x81` and timeout is the retained value. While result equals `0x8877015e`, it repeats the enumeration call, pumps at most one Windows message and consumes one queued game key when available. Escape breaks the loop; other popped keys are discarded. There is no additional elapsed-time limit or sleep in this wrapper.

Every call stores the current HRESULT in g_enumSessionsResult. Final exact zero returns collected count; any other result, including Escape while still pending, returns minus one even if callbacks already collected records. A Windows quit message is not specially handled before TranslateMessage/DispatchMessage in this body. Callback reentrancy can therefore invoke unrelated UI work during enumeration.

HAPINET_getgamescallback (`0x4c9c50`) returns zero on flag bit one, otherwise appends a result at count: unbounded session-name copy into 32 bytes, four user values, instance GUID and maximum-player count, then increments count and returns one. It does not copy current-player count or validate output capacity. Result record is 84 bytes and some fields remain untouched. The caller must size/initialize its list and understand user-value meaning.

HAPINET_getcurrentplayers (`0x4c9dd0`) separately probes the negotiated session-description size, allocates a temporary, retrieves it and returns dwCurrentPlayers on zero result, otherwise zero. Zero therefore conflates empty session and failure. Temporary allocation failure is not checked before the second call. HAPINET_justone (`0x4c9d10`) is a callback that writes its first input at context integer index eighteen and stops enumeration by returning zero; it does not itself retrieve a complete player list.

## Providers, connection records and caller-owned buffers

HAPINET_getconnections (`0x4ca250`) creates an interface first if absent, using the default provider GUID and supplied application GUID. It installs caller pointers for GUIDs, connection records and packed names, resets provider count and calls EnumConnections with flag one. Return one means exact zero enumeration HRESULT, not a returned provider count; that count remains in the object.

HAPINET_enumconnections (`0x4ca100`) skips four configured provider GUIDs. Accepted entries copy a GUID, allocate/copy the provider's opaque connection bytes and record byte size. It formats the short provider name into a 200-byte local buffer and copies it into the next packed NUL-separated name slot. Allocation failure stops callback enumeration after the GUID slot was already assigned. No array-capacity, aggregate-name-size or provider-name-length check exists. Allocation ownership transfers to the caller's connection array, whose frontend cleanup must free each data pointer; HAPINET_uninitmultiplay does not walk that array.

HAPINET_getproviders (`0x4ca400`) is an alternate provider enumerator using DirectPlayEnumerateA. It resets provider count and installs GUID/name outputs, then always returns one without checking enumeration HRESULT. Its callback HAPINET_enumproviders (`0x4ca330`) appends GUID and formatted name plus major.minor version, again using a fixed 200-byte temporary and unbounded copies. It does not use the skipped-provider filter or allocate opaque connection records. These two provider APIs therefore do not produce identical output schemas or filtering.

## Lobbied connection settings and create/join

HAPINET_initlobbiedconnection (`0x4ca490`) returns zero immediately if a queried lobby pointer already exists. Otherwise it clears created/queried lobby and connection state, creates a base lobby, queries IID_IDirectPlayLobby2A, probes connection-settings size and allocates/loads the settings block. Nonnegative results count as success; a successful query/probe that does not take the buffer-too-small branch can return success without a populated connection pointer. Failed allocation stores out-of-memory. Successfully reaching the queried-lobby branch sets guaranteed packets one even when settings retrieval later fails. Partial references/allocations remain for uninitmultiplay; failure does not unwind them immediately.

HAPINET_createorjoinlobbygame (`0x4c9a70`) assumes valid connection, its description and queried lobby. When any optional argument is nonzero, it can replace password by clearing its existing pointed buffer and concatenating up to sixteen bytes. It fills maximum-player and user fields only when their current values are zero, then calls SetConnectionSettings. Zero-valued optional arguments cannot explicitly override existing nonzero values. Password storage capacity/nonnull is not verified.

If the initial update result is nonnegative, it shallow-copies the connection's description into network local state and calls Lobby.Connect, writing the queried DirectPlay pointer. It returns whether that HRESULT is nonnegative. It does not query/copy all transient strings into independent storage or initialize a separate base DirectPlay reference here. Connection settings must remain alive while their borrowed description pointers are used.

HAPINET_passwordrequired (`0x4ca450`) returns the raw `0x400` mask from connection description flags when connection exists, otherwise the local description. It does not return normalized one and does not validate a supplied password.

## Player data, names and address adapters

| Wrapper | Forwarded operation and result |
| --- | --- |
| setplayerdatabuffer, `0x4c9d30` | SetPlayerData with flags two; one for exact zero, otherwise zero. |
| getplayerdatabuffer, `0x4c9d80` | GetPlayerData with flags zero and in/out size; same Boolean conversion. Required size can still be modified on a failure. |
| addplayer, `0x4ca6a0` | CreatePlayer with caller short/long names and a zeroed 21-byte initial block: sixteen-byte bounded name within a seventeen-byte field, then two shorts for version. One for exact zero. |
| removeplayer, `0x4ca780` | DestroyPlayer, Boolean exact-zero result. |
| getplayername, `0x4ca7c0` | Raw GetPlayerName HRESULT; absent interface `0x88770140`. |
| setplayername, `0x4ca800` | Raw SetPlayerName HRESULT and caller flags; same absent-interface error. |
| enumplayers, `0x4ca840` | Raw EnumPlayers HRESULT with caller session/callback/context/flags. |
| createcompoundaddress, `0x4ca880` | Queried lobby CreateCompoundAddress with caller elements/count/output/size; raw HRESULT or uninitialized. |
| enumaddress, `0x4ca8c0` | Queried lobby EnumAddress; raw HRESULT or uninitialized. |

addplayer also copies sixteen bytes of short/long names into retained buffers without guaranteed NUL. It does not itself compare version shorts or reject incompatible clients; higher-level join handling interprets that initial data. These wrappers do not own caller output buffers, normalize all errors or automatically retry buffer-too-small results.

## Temporary interface pairs and borrowed identifiers

HAPINET_createdplayinterface (`0x4ca900`) operates on a separate two-pointer object, not the full packed network object. It zeroes both pointers, creates a base DirectPlay interface and queries the version-three interface on nonnegative creation result, returning the final HRESULT. Overwriting a live pair leaks its references; failed query can leave the base reference requiring cleanup. HAPINET_releasedplayinterface (`0x4ca940`) releases/nulls queried then base pointers, returning the queried Release reference count or zero, not a Boolean success indicator. HAPINET_getplayeraddress (`0x4ca990`) forwards through this pair's queried pointer and returns uninitialized if absent.

GetCreatedLobbyInterface (`0x4ca9d0`) returns the retained created/base lobby pointer as integer, not the separately queried lobby interface. GetSessionGuidInstance (`0x4ca9e0`) returns network address plus `0x465`, the embedded session instance GUID. Neither getter adds a COM reference, copies bytes or grants independent lifetime; callers must not release/free the borrowed pointer as a new owner.

## Report-interface stubs and coverage

QueryOnlineFlags (`0x4ca9f0`) always returns zero. RIReportGameChat (`0x4caa00`) and RIReport (`0x4caa10`) always return one without sending anything. RISetCallbacks (`0x4caa20`) stores two global callback addresses but does not invoke them. The [statistics adapter](game-statistics.md) can register these while also loading reporter.dll; these stubs explain why an enabled HAPINET route is not proof of actual report delivery in this revision.

All 44 functions and callback structures have been traced. Provider/session user values and gameplay admission rules remain owned by frontend/net_game; COM internals are external to this source. No library, network session, build, dependency installation or source listing was used to write this reference.
