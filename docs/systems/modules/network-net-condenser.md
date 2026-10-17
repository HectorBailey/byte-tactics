# Network payload compression, envelope checksum and receive caching

Status: finished. `network/net_condenser` owns nine annotations in `0x44f720..0x44fd40`: two global initializers, their exit destructors and five explicit methods. All are in [net_condenser.cpp](../../../src/network/net_condenser.cpp). The separate SendPacketTo method is owned by `network/packets` and defined in [packets.cpp](../../../src/network/packets.cpp), despite a stale filename in a nearby declaration comment.

## Place in the networking stack

The condenser wraps already prepared outgoing payloads in a three-byte envelope and optionally compresses them. Receive processing unwraps that envelope, verifies its limited checksum and returns the original logical payload. It calls HAPINET_sendpacket/receivepacket for transport and [LZSS](util-sqsh.md) for the codec. Reliable queues, message splitting, resends, routing/player ownership and individual packet schemas belong to neighboring network modules.

The name Accumulate does not mean this method appends multiple messages: it replaces the current send buffer. Actual queue bundling happens upstream. The packet-module SendPacketTo helper assigns destination, calls Accumulate once and SendPacket once, ignoring the latter's result through its void interface. The condenser has no autonomous timer, worker thread or retry queue.

## Storage and global lifetime

Constructor (`0x44f8a0`) allocates a 28000-byte receive/expanded buffer (`0x6d60`) and three 2800-byte (`0xaf0`) arrays: sendBuffer, encoded packet, and buffer_14. It clears an auxiliary field, cached-expanded state/length, byte-sized send count, send size and destination. buffer_14 is allocated/freed but not otherwise used by the methods in this group.

The two process globals g_receiveCondenser and g_sendCondenser have initializers at `0x44f720/0x44f7e0` and exit destructors at `0x44f7a0/0x44f860`. Each owns its own buffers, despite descriptive send/receive names; both use the same class. Destructor (`0x44f900`) deletes all four arrays and does not null pointers. Partial construction, repeated destruction and sharing buffers by object copy are not supported by explicit safeguards here.

Accumulate (`0x44f940`) checks only whether requested size exceeds the original constant 2800. For larger sizes it deletes sendBuffer using scalar delete, allocates an array of exactly that size and formats a diagnostic into a local buffer. It does not record a capacity. Every later above-2800 request reallocates again even when it would fit the previous allocation. It copies at offset zero, replaces sendSize and increments the byte-sized sendCount. Prior unsent contents are lost rather than concatenated. Negative lengths and null pointers are not checked.

Only sendBuffer grows. The encoded packet and receive expansion arrays retain their original capacities, so accepting an oversized send payload does not make SendPacket safe. Even a raw payload of exactly 2800 bytes requires 2803 envelope bytes, beyond the allocated encoded packet buffer. Upstream packet-size limits are important assumptions, not enforced by this class.

## Outgoing representation and compression selection

SendPacket (`0x44fc10`) tries LZSS compression when sendSize is greater than twelve, with the codec's global preset enabled around the call and disabled afterward. At twelve or less it uses compressed length sentinel `0xffff`, selecting raw storage for ordinary sizes. It still computes compression when compressionOff is nonzero, then declines to use the compressed result; the setting does not bypass codec work.

It chooses compressed data only when compressed length plus three is strictly less than the original payload size and compressionOff equals zero. This is a stricter condition than merely compressed payload smaller than raw payload, since both eventual wire forms carry a header. Otherwise it copies raw payload to encoded buffer offset three. The envelope is:

| Bytes | Meaning |
| --- | --- |
| 0 | Kind three for stored/raw data, four for LZSS data. |
| 1 through 2 | Unsigned sixteen-bit checksum in the process's little-endian packed layout. |
| 3 through total minus one | Encoded or raw payload bytes, partially XOR-transformed as below. |

For indices three through total minus four inclusive, it XORs each payload byte with that index narrowed to a byte and adds the transformed byte to the sixteen-bit sum. Header fields are written afterward. The final three payload bytes are neither transformed nor included in the checksum. For very short payloads the processed range may be empty. This is a symmetric lightweight transform, not encryption/authentication, and it does not protect all payload bytes or the kind field.

There is no output-capacity argument passed to LzssCompress and no special check for a codec error sentinel beyond the size comparison. The preset is shared global codec state and is always disabled after the ordinary return, rather than restoring an arbitrary previous setting. Concurrent/reentrant use would need coordination outside this body.

## Transport result, simulated loss and counters

When netSendDropPct is nonzero, SendPacket draws floor(rand times 101 divided by 32768), a value in `0..100` for the existing rand range, and drops when it is less than or equal to the configured setting. This inclusive comparison means the setting is not an exact conventional percentage. A simulated drop reports result zero without calling HAPINET. Otherwise HAPINET receives session, sender, retained destination, envelope pointer and total length.

Result zero, including a simulated drop, invokes CountPacket with uncompressed sendSize plus three as its first argument and actual encoded total as the second. [Network statistics](network-net-stats.md) consequently count nominal uncompressed outgoing packet bytes separately from encoded bytes, rather than measuring confirmed delivered traffic. A transport failure is not counted by this success branch.

Regardless of result, it resets sendSize and sendCount to zero and returns the transport/simulated result. It does not retain failed data for retry. Destination persists. Reliability and retransmission, where used, must retain their own upstream payload copies.

## Receive flow and sender gating

ReceivePacket (`0x44f9c0`) saves the caller's initial capacity and first checks whether a previously expanded result is cached. With no cached result it calls HAPINET_receivepacket directly into the caller's buffer, letting that API update size. Extra condenser processing occurs only when the network object's from field at offset `0x4b5` is nonzero. A zero-from/system path returns the transport result unchanged without this decoding/counting branch.

For a successful nonzero-from receive it counts the transport-reported packet bytes before validating the envelope. A first byte other than three/four is returned successfully untouched; noncondenser packets can pass through. Kinds three/four require total size at least four, so a bare three-byte zero-payload condenser envelope is rejected with `0x887700be`. The function reads kind before this minimum-size check and has no separate zero-length-success defense.

It sets payload length n to total size minus three, sums transformed bytes at indices three through n minus one, and XORs them in place with the index byte. Because n equals total minus three, this excludes the same final three payload bytes as the sender. Checksum mismatch returns `0x887700be`; caller storage has already been partly decoded, with no rollback. The checksum is over pre-XOR received bytes, matching sender's post-XOR sum.

For kind four it enables the global LZSS preset, expands data from offset three into the internal receive buffer and disables preset. The decompressor receives no explicit input length/output capacity here. For kind three it copies n bytes to that buffer. Expanded result length is retained as unpackedSize. Raw n and codec output are not checked against the internal 28000-byte allocation before writing, so later caller-capacity checks do not protect that internal buffer.

## Buffer-too-small caching and retry

After decoding, if unpackedSize exceeds the caller's original capacity it sets unpackedCached, returns required size and result `0x8877001e`. The transport packet has already been consumed; the decoded result stays in the condenser for a larger-buffer retry. A later ReceivePacket checks that cache before requesting a new transport packet. Too-small retries leave it cached and again return required size. A sufficient retry copies expanded bytes, updates size, clears cached and returns zero.

An adequate initial caller buffer receives the decoded bytes immediately and returns zero with the logical length. The cache stores payload and size, but not an independent sender/session snapshot; callers rely on the network object's retained from/to metadata remaining appropriate until the cached result is delivered. It is a single pending result, not a multi-packet queue.

Underlying HAPINET errors generally pass through. `0x8877001e` from transport formats a local diagnostic but does not create an expanded-result cache; the transport still owns that retry situation. `0x887700be` also returns as-is. The local sprintf diagnostics are not sent to a logger or displayed by this body, so they should not be described as visible runtime messages. No exception cleanup restores preset/cache state after an abnormal codec failure.

## Coverage and persistence

All nine owned annotations, the upstream SendPacketTo caller and traffic-counter argument ordering have been checked. The condenser's buffers/counters/cache are transient; it has no savegame format, lobby rules or unit-target semantics. This source-only writeup identifies size/checksum/failure assumptions without executing networking, allocating game buffers, building or installing dependencies, and contains no source listings.
