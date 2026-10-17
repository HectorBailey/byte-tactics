# Sharing preferences, thresholds and diagnostic command toggles

## Scope and command routing

The `game/share_commands` group owns twelve entries in `0x418cd0..0x419560`, all in [share_commands.cpp](../../../src/game/share_commands.cpp). It changes local sharing preferences and thresholds and exposes four diagnostic/transport toggles. Actual resource distribution, unit gifting, visibility merging and outgoing packet encoding are implemented by consumers, not by these small handlers.

The main command table in [console_commands.cpp](../../../src/game/console_commands.cpp) registers ShareMetal, ShareEnergy, ShareMapping, ShareRadar, ShareAll, ShowRanges, SetShareMetal, SetShareEnergy, Compression, BPS and SFX with permission mask one. ToggleShareLos is a separate owned entry but is not registered under a ShareLOS name in that table slice. Its existence alone does not establish an available chat command with that spelling.

[Command parsing/dispatch](util-commands.md) and [chat submission](ingame-dialogs.md) determine how text reaches handlers and which permission mask applies. Cross-file callback declarations sometimes use CommandArgs pointers where the handler declares an unused integer; these no-argument toggles do not inspect that argument. Threshold handlers do inspect CommandArgs.

## PlayerInfo sharing flags

Five handlers test network-game flag bit 0 before changing anything. When it is clear they return without a message or broadcast. When set, each flips one local PlayerInfo flag, adds a confirmation message with arguments 2, 0, 10, and broadcasts the updated player-info block.

| Entry | Address | PlayerInfo flag |
| --- | --- | --- |
| CmdShareMetal | `0x418cd0` | shareMetal, mask 2 in flags at `+0x97` |
| CmdShareEnergy | `0x418d90` | shareEnergy, mask 4 |
| CmdShareMapping | `0x418e50` | shareMapping, mask `0x20` |
| ToggleShareLos | `0x418f10` | shareLOS, mask 8 |
| CmdShareRadar | `0x418fd0` | shareRadar, mask `0x40` |

The flag layout is declared in [player_info.h](../../../src/network/player_info.h). Some bodies use bitfield negation and others explicitly replace the masked bit. Both leave unrelated flags unchanged. Confirmation text is constructed from fixed English text plus ON/OFF; these handlers do not pass the whole message through Translate themselves.

The broadcast communicates preferences, not a resource amount or a precomputed sight bitmap. Turning a flag on does not immediately transfer every excess resource, reveal every enemy or bypass alliance/observer checks. [Players](game-players.md), [line of sight](map-line-of-sight.md), [radar](map-radar.md) and network game state determine the later effects.

CmdShareAll (`0x419090`) has the same network gate and invokes four inlined toggle bodies in order: metal, energy, mapping, radar. It does not include LOS. It toggles the existing values rather than setting all flags true; mixed states remain mixed with each bit inverted. Each component adds its own message and broadcasts player info, so one ShareAll call can produce four successive preference updates rather than a single combined publication.

## Resource thresholds

CmdSetShareMetal (`0x419340`) and CmdSetShareEnergy (`0x419400`) act only with network-game bit 0 set. Each obtains integer argument one with fallback zero, converts it to float and stores the smaller of that value and the corresponding current storage capacity in the local player's share threshold.

There is no lower clamp. Negative input produces a negative threshold; an absent/nonnumeric argument follows CommandArgs' integer conversion/default rules. Capacity limiting uses the current capacity, not a maximum remembered for a future completed storage building. Later capacity changes do not automatically rerun this handler.

The confirmation prints the requested integer rather than the clamped stored float. A request above capacity can therefore display a number larger than the actual threshold. The min expression can evaluate the argument getter more than once, and the message obtains it again; the getter is expected to be stable. These handlers do not broadcast player info or send an immediate transfer. The threshold fields belong to the local player runtime record, not the five sharing-enable bits in PlayerInfo.

The direct consumer is UpdateResourceSharing in [net_game.cpp](../../../src/network/net_game.cpp). It checks the enable flag and stored balance versus threshold on its resource schedule, applies recipient/alliance/role tests and computes a bounded transfer. It uses TransferEnergy for actual metal fields and TransferMetal for actual energy fields, matching the source naming mismatch established in [player transfers](game-players.md). A threshold change is thus input to later sharing work rather than proof a transfer already occurred.

The [share dialog](ingame-dialogs.md) is a separate explicit-amount path. Its sliders request one transfer and optional unit/map gifts; these chat commands configure ongoing preference/threshold state instead.

## Outgoing compression control

CmdCompression (`0x4194d0`) is also gated by network-game bit 0. It assigns compressionOff from whether its previous value equals zero, normalizing it to zero or one, and prints outgoing compression as OFF when compressionOff is nonzero or ON otherwise. It does not publish this preference to every peer or change the receive decoder.

The consumer in [net_condenser.cpp](../../../src/network/net_condenser.cpp) selects packed outgoing data only when its compressed length plus three bytes is less than the uncompressed send size and compressionOff is zero. Enabling compression therefore permits a size-beneficial representation; it does not force every packet through an expanded compressed encoding. Transport grouping, compression format and packet authority remain separate network responsibilities.

## Local display and effect toggles

The remaining three handlers do not test the network-game flag:

| Entry | Address | Mutation |
| --- | --- | --- |
| CmdShowRanges | `0x4194c0` | XOR the low bit of game showRanges |
| CmdBPS | `0x419540` | XOR the low bit of game showBps |
| CmdSFX | `0x419550` | XOR the low bit of the global effect-event-pool blocked byte |

They add no confirmation message, broadcast or persistence record here. XOR one toggles the low bit while preserving any higher bits; it is not the same normalization used by compressionOff.

The SFX consumer is the small effect-event allocation path in [particles.cpp](../../../src/graphics/particles.cpp), which returns null while the blocked byte is set. This blocks that pool's new allocations, not all rendering, all audio or every already-existing particle. [Particles](graphics-particles.md) documents the event pool and the other particle families. A similarly broad assumption about BPS or ShowRanges would also be unwarranted: these handlers only change the flags that their display consumers inspect.

## State lifetime and completion

These entries allocate no retained buffers, own no GUI layer and define no save-bank or packet binary layout. Player runtime/preferences and global diagnostic state are held by their respective owners; this module supplies state mutations and calls to the message/player-info services. Threshold changes and resource transfers remain distinct from the economy's request/backlog mechanism.

All twelve owned entries and the complete relevant registration table slice were inspected, with direct resource/compression/effect consumers traced. Exact recipient behavior, network distribution and information sharing require the corresponding consumer modules and current session state. No command was executed, no preference was changed and no packets were sent for this writeup.
