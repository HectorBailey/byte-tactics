# Bit streams, traffic counters and sampled network rates

Status: finished. The fifteen entries of `network/net_stats`, `0x415b20..0x416240`, are in [net_stats.cpp](../../../src/network/net_stats.cpp), with shared [BitWriter](../../../src/network/bit_writer.h) and [BitReader](../../../src/network/bit_reader.h) declarations. Two initial entries also register VTOL orders and advance GAF sequences; they are included according to ownership rather than omitted because the module name suggests counters alone.

## Bit-writer representation and lifetime

BitWriter constructor (`0x415b60`) selects an inline array of 256 unsigned 32-bit words, capacity 256 words, word index zero and bit offset zero. It zeroes only the first word initially. WriteBits initializes subsequent words when advancing into them. The inline capacity is 1024 bytes, not 256 bytes. There is no automatic destructor in this declaration; consumers explicitly call FreeBuffer.

FreeBuffer (`0x415b90`) deletes data only when it differs from the inline buffer. It does not null/reset data, capacity or position, so repeated freeing of a grown buffer and continued use after freeing are unsafe. Copying the object by value also does not establish independent ownership of a grown buffer or repair a pointer to another object's inline storage.

GrowBuffer (`0x415bb0`) doubles word capacity but allocates **one unsigned integer initialized to that new capacity**, rather than an array. It then copies every old-capacity word into that four-byte allocation, deletes the previous heap block if applicable and installs the new pointer/capacity. Thus the first growth overruns its allocation immediately. This source defect is relevant even though ordinary compact packets may stay inside the inline capacity. No allocation-size or multiplication-overflow guard repairs it.

## Bit ordering and field boundaries

WriteBits (`0x415c10`) appends low bits first within each word. Fields whose width plus current offset is strictly less than 32 are masked, shifted and ORed into the current word. A full 32-bit field at offset zero assigns the word, advances index, grows if necessary and clears the next word. A crossing field fills the remaining low word, advances, saves the residual offset and writes the remaining high bits into the next word. Exact-end fields from a nonzero offset leave residual offset zero and initialize the next word to zero. Reaching capacity triggers the defective GrowBuffer path before that next-word initialization, even when a consumer intended to stop at the just-filled boundary.

The supported calling convention relies on sensible field widths, normally one through 32, and at most one boundary crossing. There is no parameter validation for negative or larger widths; shift expressions and whole-word special cases are tied to the original 32-bit target. SetByteAt (`0x415da0`) writes one byte at a caller offset with no bounds or current-length check. It does not change the bit position and can patch already written packet headers or corrupt unwritten/outside storage if misused.

BitReader has a data pointer, word index and bit offset, with no buffer-length field or constructor here. ReadBits (`0x415dc0`) mirrors low-first extraction: mask within the current word, return the full word directly at aligned full-width reads, or combine the tail of one word with the head of the next. Exact boundaries advance correctly, but there is no end-of-packet test or failure return. Callers must initialize position and ensure enough readable word storage, including any padded access at a boundary.

ReadSignedBits (`0x415e60`) reads an unsigned field then extends its top field bit through the remaining integer bits. A zero width has no meaningful sign-bit contract; oversized widths also invoke unchecked shifts. The inline ReadBit in the shared header reads one bit, increments offset, and resets to zero/advances index at 32. It likewise has no length check. These utilities do not impose a packet schema, endian conversion or authentication. [Movement goals](game-victory.md), order targets and unit-state serializers choose their field widths and object discriminators.

## Message counters and direction indices

CountMessage (`0x415ef0`) adds amount to one of two adjacent aggregate counters by indexing from g_messageBytesReceived. Existing [net_game.cpp](../../../src/network/net_game.cpp) and [packets.cpp](../../../src/network/packets.cpp) calls use index zero for received and one for sent; the parameter named player is a direction index here, not a ten-player slot. Other values would index beyond those two counters. Negative amounts and overflow are not rejected.

Kinds two through forty-four also increment the corresponding per-kind/direction message count and byte total. Kinds zero/one and forty-five or above contribute to aggregate bytes but not those per-kind arrays. These are logical message counts, distinct from transport packet counts when messages are bundled or retransmitted.

CountPacket (`0x415f40`) adds any positive overhead argument to g_compressedBytesSent regardless of the sent flag. Nonzero sent increments sent packet count/bytes; zero increments received count/bytes. In [net_condenser.cpp](../../../src/network/net_condenser.cpp), the send call supplies uncompressed sendSize plus three as packet size and the actual encoded packet total as that second argument. The provisional overhead name therefore does not establish saved bytes: g_compressedBytesSent accumulates encoded packet sizes on that path, while g_packetBytesSent accumulates uncompressed sizes plus header. Simulated successful drops are counted too. Receive calls normally supply zero there.

ResetNetStats (`0x415e90`) records the current simulation tick in DAT_00511c20, zeroes sent/received logical bytes, per-kind counters/bytes for rows one through forty-four, and sent/received packet totals/bytes. It does not reset row zero, g_compressedBytesSent, sampled-rate timestamps, prior samples or retained output rates. Immediately sampling after a reset can therefore combine fresh zero totals with old baselines rather than producing a wholly clean interval.

## Formatted rates and compression display

FormatNetStats (`0x415fa0`) samples GetTicks. Only elapsed unsigned ticks strictly greater than thirty recalculate rates; otherwise the previous values are formatted. Each rate is the relevant byte/count delta multiplied by thirty and divided by elapsed, using integer arithmetic and the source's signed/unsigned conversions. The thirty-tick scale is fixed, not dynamically obtained from the simulation speed or display refresh rate.

It computes logical received/sent byte rates, packet sent/received byte-counter rates and sent/received packet-count rates. Compression percentage is zero when the logical sent-byte delta is nonpositive. Otherwise it is `(logical sent delta minus compressed-counter delta) * 100 / logical sent delta`. It then advances all corresponding saved samples. There is no clamp to zero through one hundred, so resets, counter semantics, overhead and overflow can produce values outside that range.

The text fields are pS and pR for logical message-byte rates, followed by S packet-count/packet-byte rates, R packet-count/packet-byte rates and C percentage, ending in newline. The function uses sprintf into caller storage with no size argument. Those pS/pR labels should not be mistaken for packets per second, which are the separate values inside the parenthesized fields.

GetByteRates (`0x416150`) has an independent timestamp and prior packet-byte samples. After more than thirty ticks it calculates sent/received packet byte-counter rates, caches them, and always writes both retained rates to the output pointers. Calling FormatNetStats does not advance this second sampler's baselines. No per-player measurement, packet-loss estimate, ping or reliability accounting is inferred from these counters.

FUN_004161f0 (`0x4161f0`) sums per-kind counts/bytes over one through forty-four into locals, discards all four totals and calls PacketManager.NopRet_D. It does not display an additional statistics report or reset anything.

## Adjacent registration and animation entries

RegisterVtolOrders (`0x415b20`) registers g_vtolOrders under numeric order-table ID `0x16`; [VTOL orders](orders-vtol-orders.md) define those handlers. StepAllGafSequences (`0x415b30`) walks animplayCursors from count minus one down to zero and calls StepGafSequence. These are registered animation cursors, not every GAF asset in memory. There is no null-element or mutation guard in that loop; [GAF](graphics-gaf.md) defines sequence stepping.

## Coverage and validation limits

All fifteen bodies and both bitstream headers have been traced, including direction call sites and condenser counter arguments. These helpers keep transient transport/diagnostic state; they do not serialize their counters to savegames or add network safety checks around caller payloads. Source links were checked; no build, dependencies or network/game execution were used.
