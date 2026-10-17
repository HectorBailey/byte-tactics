# Map plot-map persistence

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/plot_map`. Implementation: [plot_map.cpp](../../../src/map/plot_map.cpp). This writeup covers all six annotated functions in the module. It describes persistence of selected mutable cell and visibility data, not terrain rendering.

## Responsibility and ownership

The runtime map contains cells with terrain, metal, feature-related state, occupancy and flags, plus a separate visibility-mask buffer. This module writes selected mutable values to HapiBank accounts and reads them back into an already allocated map. It neither constructs terrain dimensions from a save nor serializes the complete cell structure.

The map width and height in the current game context determine every transfer size. Restoring the correct base map and dimensions is therefore a prerequisite. These routines do not decide which mission should be loaded or reconstruct units and placed feature objects.

There are three independent persisted datasets: metal bytes, a packed player-feature field, and the visibility-mask buffer. Each has a save and a load entry point. HapiBank supplies account selection, box lookup, size reporting and raw byte transfers.

## Dataset contracts

| Dataset | Account | Box | Stored size | Functions |
| --- | --- | --- | --- | --- |
| Per-cell metal | `Metal` | `Plotmap` | One byte per terrain cell | Save `0x484ce0`, load `0x484d60` |
| Player-feature field | `PlayerFeatures` | `Plotmap` | One byte per pair of terrain cells | Save `0x484df0`, load `0x484e80` |
| Visibility mask | `Mapping` | `Data` | Width times height times two, divided by four | Save `0x484f50`, load `0x484fa0` |

The account and box names are format identifiers. Their spelling and capitalization should be preserved when inspecting existing saves. The two `Plotmap` boxes are in different accounts and do not overwrite one another merely because their box names match.

## Metal transfer

Saving metal allocates a temporary buffer with width times height bytes, walks the flat terrain-cell array in its existing order, and extracts each cell's metal byte. It writes the buffer into `Metal/Plotmap` and releases it.

Loading requires both the account and box to exist and the box size to equal the current cell count exactly. It then reads a temporary buffer. Only a read reporting at least the requested size causes the cell-metal bytes to be replaced. Other cell fields are preserved.

Thus the operation changes the resource distribution independently of tile artwork, terrain height and unit occupancy. The economy's extractor scan later consumes these metal values. A save can restore a depleted or modified metal plot without replacing the base map geometry.

The shown short-read path does not release the temporary metal buffer: release is inside the successful-read branch. This is a concrete resource-lifetime asymmetry in the existing implementation. It is not a reason for a documentation change to alter game code.

## Packed player-feature transfer

The module's local cell view contains a feature-related bitfield in the cell flags byte. This is different from the separate 16-bit terrain feature identifier at cell offset `0x08`. Confusing the two would produce the wrong save schema.

Saving combines two cells into a byte. The first cell occupies the high nibble and the second the low nibble. The first expression shifts the local feature field by four; conversion to the stored byte leaves its low four bits in the high nibble. The second expression explicitly masks to four bits. Loading extracts those nibbles into a four-bit field beginning after the three lower flag bits, preserving other bits outside that assignment.

The local save view names a five-bit field while the load view assigns a four-bit field. Consequently, this is not an eight-bit round trip of the whole flags byte or even a guaranteed five-bit round trip of that field. Documenting it as a generic cell-feature ID array would miss both distinctions.

The allocated size is integer cell count divided by two. The loop processes exactly that many pairs. A hypothetical odd cell count has no extra byte for its last cell in this routine. The supported map dimensions should be established by map loading before assuming that edge case occurs in ordinary content.

Loading checks account/box existence, exact expected byte count, allocation success in the shown branch, and a read count at least as large as requested before changing the cells. Like the metal loader, its temporary release is inside the successful-read branch. The save path uses a scalar delete for its array allocation, whereas the corresponding load path uses array delete; that source-level asymmetry is preserved as observed rather than explained away as intended design.

## Visibility-mask transfer

Saving mapping writes the current visibility-mask buffer directly to `Mapping/Data`; it does not pack it through a temporary cell walk. The transfer size is half the terrain-cell count in bytes, derived as width times height times a two-byte element size, divided by four. This corresponds to the distinct coarser visibility grid used elsewhere, rather than a two-byte value for every terrain cell.

Loading checks account and box existence and requires exactly that size. It then reads directly into the live visibility-mask buffer. Unlike the two plot-map loaders, it does not inspect the returned read count before returning from this function. A short read could therefore change a prefix of the buffer without a local rollback or success report here.

This module does not serialize every player's explored-map allocation or rebuild line-of-sight caches. Those responsibilities belong to their own serializers and recalculation paths. A box named `Mapping` should not be interpreted as the entire information state of a match.

## Failure behavior and interaction boundaries

The load entry points return no success value. Missing accounts, missing boxes and size mismatches leave the targeted dataset untouched by their guarded copy paths. The caller cannot distinguish those cases through a returned status from these functions.

The save routines write their requested bytes but do not perform an explicit independent write-count audit in the shown bodies. HapiBank's allocation and I/O failure behavior must be assessed at that layer; these wrappers do not add transactional guarantees.

No packet is sent, simulation tick advanced, unit moved or graphical surface redrawn by these functions. Their state changes become visible through later consumers: extraction uses metal, feature/information handling uses the restored field, and visibility queries use the mask. Multiplayer snapshots and disk saves are separate protocols even if they preserve related state.

## Investigation and completeness

When a plot-map restore fails, verify the base map dimensions, selected account, selected box, exact size and raw read result in that order. For feature-state differences, inspect the flags subfield rather than the 16-bit feature ID. For sight differences, inspect subsequent recalculation and per-player data as well as `Mapping/Data`.

All six module entry points, their datasets, sizes, ordering, guards and relevant asymmetries are covered here. HapiBank's complete on-disk format and the meanings assigned to the player-feature nibble by every consumer are separate module responsibilities.
