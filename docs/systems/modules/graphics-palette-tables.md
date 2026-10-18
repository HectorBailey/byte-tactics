# Palette transform cache loading and generation

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/palette_tables`, all five entries `0x42e140` through `0x42e300` in [palette_tables.cpp](../../../src/graphics/palette_tables.cpp). These are asset-cache adapters for the display's indexed-color transforms. [Palette services](graphics-palette.md) implement allocation, matching, generation and copying; this module chooses between packaged/precomputed data and generation.

## 1. Initialization caller and paths

[Game-state initialization](../../../src/game/game_state.cpp) calls LoadAlphaTable, LoadShadeTable and LoadLightTable with g_game->paletteRgba, then calls MakeGrayTable and MakeBlueTable. These calls prepare rendering lookup buffers for the current palette. They do not run on each ship, construction stage or particle update.

BuildDataPath combines the palettes directory, fixed base name PALETTE and one of three extensions. The resulting paths are palettes/PALETTE.ALP, palettes/PALETTE.SHD and palettes/PALETTE.LHT using the path builder's native separator convention. There is no per-mission palette hash, version suffix or data checksum in this module's cache key. A preexisting file is trusted as the relevant transform for the palette supplied by the caller.

| Entry | Expected transform | Byte count copied/written |
| --- | --- | ---: |
| LoadAlphaTable, `0x42e140` | 256 by 256 source-index blend | 65536 |
| LoadShadeTable, `0x42e1d0` | 32 shade rows by 256 indices | 8192 |
| LoadLightTable, `0x42e260` | 32 light rows by 256 indices | 8192 |

The sizes above are fixed by the setters and write calls. File-length queries are used only as a truth test, not compared to those expected sizes.

## 2. Existing-file branch

Each loader first calls HAPI_FileLengthByName on the constructed path. Any nonzero result selects loading. HAPI_LoadFile(path,0) returns the temporary input buffer. A null result is passed to FatalError with the path. The loader contains no recovery generation branch after that error call; it relies on the fatal routine's nonreturning/error contract for safety.

The matching SetAlphaTable, SetShadeTable or SetLightTable copies the full expected byte count to display-owned memory when its availability flag is enabled. The temporary loaded buffer is then freed through GameFreeThunk. The file's contents are not parsed, endian-converted or checked against palette colors. A short nonempty file can therefore reach a fixed-size copy without a local size check, while an oversized file's extra bytes are irrelevant to the setter.

Each loader returns the same pointer variable after freeing it. That returned value is dangling in this branch and must not be treated as an owned or readable table. The known game-state caller declares/uses these calls for side effects and ignores the return. The usable data is the setter's display-owned copy, subject to the setter's availability flag. If that flag is disabled the setter does nothing, but this wrapper still frees and returns the temporary pointer.

## 3. Missing-file branch and ownership asymmetry

A zero file-length result selects BuildAlphaTable, BuildShadeTable or BuildLightTable using the supplied palette. Each generator writes into an existing display-owned buffer and returns that pointer, or null when its corresponding table flag is off. The loader then calls WriteBufferToFile with the fixed expected byte count and returns the generator result.

The wrapper neither frees that result nor invokes the corresponding Set method, because the generator already populated display storage. In this branch the returned pointer can remain valid for the display buffer's lifetime, unlike the existing-file branch. There is no common caller-owns-return contract across both branches.

The write result is ignored. Failed cache persistence does not locally discard a successfully generated transform or report failure through a separate status. Conversely there is no null guard before attempting the write when a generator returned null. The normal initialization contract is that display allocation/flags permit generation. The cache files are derived rendering resources, not gameplay save files or authoritative simulation state.

## 4. Gray and blue wrappers

MakeGrayTable (`0x42e2f0`) directly invokes BuildGrayTable. MakeBlueTable (`0x42e300`) directly invokes BuildBlueTable. Neither queries a GRAY/BLUE file, constructs a cache path, writes a file, returns a status nor frees a result. Palette services determine availability and fill the display-owned 256-byte remap buffer in place.

Their parameter is locally represented as an unsigned integer, but it is forwarded as the palette address. This is a decompilation/interface representation, not a numeric palette identifier with a registry lookup here.

## 5. Render implications and limits

The loaders do not modify source palette entries, set brightness or draw a pixel. ALP blends two palette indices, SHD/LHT map an index through a selected lighting row, and gray/blue tables remap individual indices. Downstream graphics code chooses when and how to use them, including the separate model, fog and effect paths. An already cached transform can produce different output from regeneration if its contents do not correspond to the supplied palette, since this module has no validation against that palette.

All five module entries and their caller/setter/generator contracts were traced. No networking, unit interaction or serialization is performed beyond writing derived cache files. Exact packaged bytes remain asset-dependent; ownership, branch order, fixed sizes, missing-file behavior and unchecked failure paths above follow the source. No source listings, build or runtime check was used for this reference.
