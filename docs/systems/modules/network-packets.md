# Packet channels, retained pools, frame sequencing and receive queues

Status: finished. `network/packets` owns 68 annotations in `0x460e20..0x463ca0`. Sources are [packets.cpp](../../../src/network/packets.cpp), [global initialization](../../../src/network/packets_460e20.cpp), [global destruction](../../../src/network/packets_460f60.cpp) and [receiver declarations](../../../src/network/packet_receiver.h). Generated deleting destructors, player helpers and chat-index reset at the end of this range are included.

## Responsibilities and representation

The global PacketManager contains eleven destination channels, a shared growable outgoing buffer and one receiver with ten per-sender records. A channel owns byte-storage blocks, a separate packet-descriptor pool, a linked list of descriptors still referencing blocks and a 1024-entry pending ring. The receiver owns current/spare packet storage, per-player out-of-order storage and per-player command-frame queues. These are transient process pointers and sequence state, not a savegame schema.

This layer groups command bytes, schedules sends and orders incoming bundles. [NetCondenser](network-net-condenser.md) supplies compression/envelope/transport wrapping; [bit utilities](network-net-stats.md) provide variable command-length extraction. Higher game-protocol registration supplies command lengths and handlers. A retained packet pool and a gap callback do not establish reliable delivery by themselves: the current ReportPacketGap implementation in [net_game.cpp](../../../src/network/net_game.cpp) is empty, and several send paths discard transport results.

Key storage sizes are:

| Object | State and capacity |
| --- | --- |
| PacketBuffer | Channel pointer, start index/count/used length/first descriptor plus 1066 inline payload bytes (`0x42a`), allocation `0x43e` bytes. |
| NetPacketPendingEntry | 32 bytes: sender/frame argument, byte offset/length, owner block, queued marker, sent timestamp and previous/next links. |
| PacketRing | Count/read/write indices plus 1024 descriptor pointers. Count disambiguates full and empty, so all 1024 slots are usable. |
| FrameRing | 512 records of due tick, borrowed command pointer and size, with count/head/tail. |
| PlayerFrameInfo | Sender ID, saved recipient, last bundle sequence, pending length/capacity/pointer and an embedded FrameQueue. |

Descriptor frame is used as the transport sender argument when a group is sent. It differs from the negative bundle sequence called frameNumber. Command due ticks are another separate value.

## Initialization, pacing and channel discovery

Global initialization (`0x460e20`) constructs the manager and registers exit destruction (`0x460f60`). PacketManager constructor (`0x4611e0`) starts default pacing at 200 milliseconds and clears outgoing pointer/size/capacity while constructing channels and receiver. Its ordinary/generated destructors (`0x461420/0x461340`) have empty explicit bodies but still destroy receiver/channel members. The outgoing raw buffer is not explicitly freed by that body. PacketChannel constructor (`0x461a70`) starts IDs/indices at minus one, sequence at minus two, empty pools/list/ring, retain time four seconds and send spacing 200 milliseconds.

InitPacketManager (`0x461020`) applies a nonzero configured frame rate through SetDefaultSendPacing, then, when manager use is enabled, ensures receiver storage exists, resets receive length/pending attachment, initializes channel zero for destination zero using caller pool sizes, initializes ten others unused with two blocks/100 descriptors, and lowers the current thread to priority minus two. It ignores individual InitPools return values. Disabled manager use returns one without those allocations. The member InitChannels (`0x461750`) performs equivalent initialization but returns zero when disabled or receiver allocation fails.

SetDefaultSendPacing (`0x4619e0`) disables g_usePacketManager for a negative input. Zero means 200 milliseconds. Positive inputs clamp to `2..30` sends per second and store integer 1000 divided by rate milliseconds. It updates every channel's pacing ticks with ceiling milliseconds times thirty divided by one thousand. It does not explicitly re-enable the global switch after a prior negative request.

SetSendPacingMs (`0x4628a0`) performs that tick conversion without its own range clamp; negative/overflowed arithmetic can become a large unsigned interval. SetMinRetainMs (`0x462860`) clamps unsigned milliseconds to `4000..60000` and rounds up to thirty-Hz ticks. GetMinRetainMs (`0x461d80`) divides retained ticks by thirty before multiplying by one thousand, losing fractional seconds.

FindChannel (`0x461630`) searches all eleven IDs, then, if creation is allowed, initializes the first unused ID-minus-one channel. When full it searches channels one through ten for an ID absent from all ten game Player.id fields, irrespective of player active state, and reuses it. Channel zero is excluded from that reclamation scan. Initialization results are not checked before returning the selected pointer. ReleaseChannel (`0x461820`), ReleaseAllChannels (`0x461860`) and ResetChannel (`0x462470`) reinitialize pools with destination minus one, 200-millisecond pacing and default sizes, then reset selected indices/next-send time; existing allocations are retained, not necessarily shrunk to requested defaults.

## Pool initialization, reuse and growth

InitPools (`0x461db0`) sets destination/pacing. It allocates descriptors only when absent, defaulting zero requested count to 100, and blocks only when absent, defaulting zero block count to two. Fresh descriptors initialize every field except frame. Blocks receive their channel pointer and empty metadata. Reset copies a template whose frame is likewise uninitialized, clears owner/queue/list fields and drains pending ring entries by advancing read position. It sets block count to zero before calling FreePackets, so that reset invocation skips removing old descriptors individually; the later descriptor/template/list/ring reset supplies the overall clearing.

There is no full transactional allocation rollback on failure. Already allocated pools/blocks and destination changes can remain. Invalid negative counts, multiplication overflow or a corrupted negative ring count are not handled. Queue indices are advanced/drained rather than reconstructed from a new ring, relying on their existing consistency.

PacketBuffer.IsReusable (`0x462a60`) and an inline equivalent return true for an empty block. Otherwise they walk linked descriptors while owner matches the block, rejecting any queued marker at least zero or any sent timestamp inside the retention interval. A sent marker is negative; retained timestamp comparisons include the original signed/unsigned arithmetic and are not a separately wrap-safe clock abstraction. Descriptors no longer contiguous by ownership terminate this scan.

AllocBuffer (`0x461b10`) considers the next cyclic block. If reusable, it frees its descriptors and selects it. If not reusable and block count is at least 34, it forcibly frees/reuses it anyway. Below that threshold it grows by sixteen blocks and 800 descriptors and repeats. It does not scan every existing block for a better candidate first. AllocPacket (`0x461c20`) similarly considers the next cyclic descriptor, reuses an unowned/reusable owner block, or force-frees the owner when descriptor count is at least 1700. Otherwise it calls GrowPools with 800 more descriptors and retries without checking failure, so an allocation failure can prevent progress rather than cleanly return zero. It assigns the new descriptor's frame only when selected.

GrowPools (`0x461fd0`) returns one without doing anything if either existing pool count is nonpositive. Otherwise it allocates a new block-pointer array, rotates existing blocks relative to the previous selected block and appends requested new blocks. It then allocates a replacement descriptor array, initializes unused fields, copies live descriptors grouped by block, fixes block first/start/count and channel linked-list references, and dequeues/requeues pending descriptors so rings point to new addresses. Finally it releases old descriptor storage and selects the last copied index. Pointer stability is not guaranteed across growth.

Failures can leave the enlarged block array/count installed even when descriptor allocation fails; there is no rollback to the old complete pool arrangement. Caller-retained descriptor pointers can become invalid on successful growth. Pool thresholds bound when forced reuse begins, not an assurance that unacknowledged or recently sent payloads survive indefinitely.

## Adding, linking and removing packets

PacketBuffer.AppendPacket (`0x4628d0`) accepts data only when current used length plus unsigned size fits 1066 bytes, then copies payload and fills descriptor offset/owner/size/links. The first appended record captures the descriptor index and pointer; all appends increase block length/count. It does not validate source format or reject zero-sized commands.

PacketChannel.AddPacket (`0x462710`) force-flushes when queuedBytes is already at least 1066 or the ring has 1024 entries, returning zero if pending entries remain. It chooses/allocates a block and descriptor, attempts append, and on insufficient block space backs up the descriptor index and retries with a new block/descriptor. Success links it to channel first/last, pushes it, sets queued zero and adds bytes. Ring-push success is not checked at that final point. On failure the cleanup condition can dereference a null descriptor when allocation failed, depending on which block path was taken; oversized single payloads cannot fit either attempt.

EnqueuePacket (`0x461f90`) pushes an existing descriptor, sets queued zero and increments bytes even if ring push failed. DequeuePacket (`0x4623e0`) marks it minus one, records time, rotates/populates the ring until finding it and subtracts its byte count. It subtracts even when the item was absent; stopping early after rotating preceding elements changes the remaining queue order. Public PushPacket/PopPacket (`0x462370/0x4623b0`) return zero/null for full/empty respectively and wrap at 1024. PacketRing constructor (`0x460f40`) sets count/read zero, write minus one.

PacketBuffer.RemovePacket (`0x462ae0`) removes a pending item from the ring when its marker is nonnegative, adjusts block first and channel list endpoints, and unlinks prev/next. It does not by itself clear owner, decrement block count or reclaim used bytes. NetPacketPendingEntry.RemoveFromBuffer (`0x462bd0`) calls it then clears owner. GetData (`0x462bf0`) returns owner address plus twenty plus offset, without validating owner. GetPacketEntry (`0x461da0`) similarly computes a descriptor address without checking index; GetCurrentBuffer (`0x461d70`) returns the selected block pointer when the selected index is nonnegative, without checking the upper bound.

FreePackets (`0x4629b0`) walks the block's recorded count starting at its recorded pool index with wrap, invokes each descriptor's actual owner removal and clears its owner, then clears block length/count/start/first. That assumes index/count correspond to live descriptors; stale/null owners are not defended. FindOwnerChainTail (`0x462a40`) merely walks to the end of an ownership run and discards the result, changing no state.

## Sending bundles and failure propagation

AppendToSendBuffer (`0x4614e0`) grows the manager buffer by at least 512 bytes, or by incoming length when larger. First allocation adds four header bytes, sets used size four and writes sequence minus two. It preserves old content, appends bytes and counts the command as sent logically. Unsigned length arithmetic and zero-length data's first-byte access are unchecked. ClearSendBuffer (`0x4615f0`) resets used size to four when allocated, otherwise zero; it does not rewrite the header.

SendQueued (`0x4624a0`) returns one without sending before the channel deadline unless forced. When due it schedules the next interval and repeatedly groups all currently pending descriptors whose frame matches the ring head. Other descriptors are rotated back. Selected entries leave the ring, receive the channel's negative frameNumber and sent timestamp, and append to the shared outgoing buffer. An append-allocation failure returns zero after those destructive queue/marker changes, without restoring removed items.

The outgoing first dword is minus one for a directed nonzero destination, otherwise the channel's negative sequence for destination zero. The condenser is called with head descriptor frame as sender and channel dpid as recipient. Its SendPacketTo wrapper (`0x4626e0`) discards the actual transport result. The shared used size resets to four and frameNumber decrements, with minus one or greater normalized to minus two. queuedBytes becomes zero after each grouped scan, even when different-frame entries remain for later groups. Normal completion can send multiple bundles in one call and returns one without delivery acknowledgment.

SendAllQueued (`0x4618a0`) visits every non-unused destination and returns zero at the first channel failure. QueueOnChannel (`0x461990`) forwards to a supplied channel's AddPacket. QueuePacket (`0x4619b0`) creates/finds a destination and returns AddPacket's integer success cast as void pointer, not a descriptor pointer.

SendToDPID (`0x461180`) wraps direct condenser send and converts transport zero to Boolean one. SendFrameState (`0x461900`) writes minus one when its message field_14 is nonzero, otherwise field_10, sends using field_14 as destination and resets used size. It assumes buffer exists, ignores SendTo's result and always returns one. Consequently different send adapters expose different result conventions; no caller should infer confirmed transmission from every one-valued result in this module.

## Receiver records, resets and borrowed buffers

PacketReceiver constructor (`0x462c00`) clears owner/buffer/state fields and constructs ten PlayerFrameInfo objects. Its ordinary/generated destructors (`0x462d30/0x462cc0`) free spare if nonnull, otherwise current buffer, then destroy per-player entries. This choice avoids double-free when current aliases a saved per-player frame during a swap, assuming the expected swap invariant.

PlayerFrameInfo constructor (`0x4635b0`) starts IDs/sequence at minus one, pending length zero, pending capacity minus one and frame null. Initialize (`0x463610`) changes ID, resets pending recipient/sequence/length and calls FrameQueue.ResetFrames while retaining the separately allocated saved frame. Destructor (`0x463680`) frees that frame; the embedded queue destructs afterward.

FindPlayerFrameInfo (`0x462d90`) searches existing IDs until the first unused record, then takes an unused record or reclaims one whose old ID is absent from all ten game Player.id fields. It does not require those Player slots to be active. If an unused hole precedes an existing later matching ID, the early break can lead to another record for that ID rather than a complete initial match scan. Full/still-referenced tables return null.

ResetReceiveBuffer (`0x462ed0`) ensures a current 1066-byte buffer, reusing spare where possible, resets current length and clears an attached saved entry's pending length. It does not reset every sender queue/sequence. Queue/packet payload pointers are borrowed slices valid only while their owning copied byte buffer remains intact.

## ReceiveFrame transport and sequence ordering

ReceiveFrame (`0x462f30`) first scans sender queues in array order until an unused ID. A command is due when requested tick is zero, due tick is no later than current gameTick, or it is more than thirty ticks ahead. That last condition releases implausibly distant future entries rather than waiting indefinitely. Due entries copy directly to caller data without checking caller *size capacity, then update network from/to and return zero. Lower-index senders can take precedence over others.

When no queued command is due, the receiver resumes any saved/spare bundle swap or asks the receive condenser for another packet into its internal buffer. Buffer-too-small (`0x8877001e`) causes deletion/reallocation to the required size and retry; allocation failure returns `0x8007000e`. No-messages (`0x887700be`) clears current length and returns. Other errors format a trace through an empty stub, clear length and pass through.

Sender-zero system messages bypass bundle parsing on the fresh transport path and honor caller capacity, retaining length on buffer-too-small. Other senders require at least four bytes and reject exactly four with `0x80004005`; ordinary bundles therefore need command bytes after their header. A later retry with retained nonzero length bypasses some fresh-transport classification, so this is not a universally uniform system-message retry path.

Header minus one is unsequenced. Other values use a per-sender last sequence, normally decreasing from minus two. The expected next value is previous minus one, normalized away from minus one/nonnegative values. A newly observed gap with no pending saved bundle copies the whole bundle into that sender's saved frame and returns no-messages. With an existing saved bundle, comparisons choose which bundle to process first, call ReportPacketGap for missing ranges and retain the other using current/spare/saved pointers. Sequence arithmetic and direct signed comparisons are original integer logic rather than a fully general wrap-safe protocol abstraction.

The gap callback is empty in this revision, so detecting the gap does not automatically request resends. Unknown sender allocation failure returns general failure or no-messages depending on path. After selecting a bundle it updates sequence, submits command bytes to the sender's FrameQueue and returns one due command if available. Success copies are unbounded by caller capacity on those command paths. Return zero means a command/system message was produced; no-messages can also mean a future/gap bundle was retained, not that transport had no bytes.

## Splitting commands and smoothing delivery ticks

FrameQueue constructor (`0x4636b0`) allocates a 512-record FrameRing and clears byte-buffer/tick/source state. Destructor (`0x463710`) frees ring and copied bytes. ResetFrames (`0x463730`) clears metadata and sets recvBuffer null without freeing the previous allocation; when a ring already exists it does not reset that ring's count/head/tail. Reinitializing a used queue can therefore leak bytes and retain stale ring records. It returns one for an existing ring and zero when it allocated a ring, rather than ordinary allocation-success Boolean semantics.

QueueFrames (`0x463790`) returns one for zero size. If old commands remain, it increments skipCount, pops/pushes every record with the supplied current tick and returns zero without copying the new bundle. That accelerates existing commands; the receiver's retained length/next call determines later processing rather than merging byte buffers.

For an empty ring it clears skipCount, allocates size plus 256 bytes when necessary, copies the whole bundle and records sender/recipient. It skips the first four sequence bytes and scans command kinds two through forty-four. Most lengths come from g_packetSizes; kind forty-four reads a sixteen-bit length after its type byte using BitReader. It stops on invalid kind or length exceeding remaining bytes. A zero registered/embedded length can prevent scanning progress, and there is no comprehensive minimum-width check before variable-length reads. Allocation failure returns one even though no commands were queued.

It counts commands first. When more than 512, it skips early kind-forty-four commands while excess remains, rather than discarding arbitrary commands or growing the ring. If excess consists of other kinds, pushes can still hit capacity and truncate delivery. With smoothing enabled, spacing uses tick minus baseTick clamped to `1..30`, sixteen-unit fixed progress and command count. It starts due ticks at current tick and increments according to that progress threshold. The body never updates baseTick afterward, so it is not a complete interval-between-packets estimator. Without smoothing, accepted commands all use the supplied tick. Saved-bundle processing selects the unsmoothed path through its caller flag.

Ring pushes point into recvBuffer rather than allocating command copies. A later accepted bundle can overwrite that storage only once the ring is empty under the intended path. Return values mostly indicate the bundle was processed/consumed, not proof every command fit, validated or reached a gameplay handler. Packet schemas and their registered lengths are owned by the game protocol.

## Adjacent player helpers, stubs and coverage

Player constructor (`0x463be0`) sets inactive/type zero, index ten, allocates and zeroes a 185-byte PlayerInfo and constructs its embedded grid. FreeSideDataAndFogSightCounts (`0x463c40`) deletes PlayerInfo and grid cells without nulling them. SetType (`0x463c60`) narrows the type to a byte and copies kind to PlayerInfo except for type three, whose remote kind is retained. ResetChatHudIndices (`0x463c80`) zeros the game chat read/write indices without clearing message text.

PacketTrace (`0x461170`), PacketManager.NopRet_D (`0x461610`), HandleIntegrityNop (`0x461620`) and NopRetC_B (`0x461d60`) are empty. Formatted traces in this group are therefore not visible log output. RegisterEmptyAtexit (`0x463ba0`) sets a latch bit when absent but calls atexit on every invocation; EmptyAtexitHandler (`0x463bd0`) does nothing.

All 66 main-file annotations and both global-lifetime companions have been traced, including receiver branches and frame splitting. No game, build, dependency installation or network operation was performed. Limits above describe existing source behavior rather than repaired or experimentally verified transport guarantees.
