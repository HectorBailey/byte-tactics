# Bitmap font metrics, string fitting and outlined text

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/font`, all four entries `0x4c1480`, `0x4c14f0`, `0x4c1760` and `0x4c1830` in [font.cpp](../../../src/graphics/font.cpp). These functions measure and draw one line of the game's byte-oriented bitmap font. Font asset loading/freeing, global style selection and glyph bit rasterization belong to separate address groups.

## 1. Font data, characters and borrowed state

The current font and foreground/background/transparent colors live in the display object. [Drawing-style accessors](graphics-draw.md) select those values without acquiring ownership. [Data-file services](../../../src/game/data_files.cpp) load fonts by name from fonts/*.FNT, including the COMIX and smlfont assets, and free them during resource teardown. This module neither loads an asset nor frees the current pointer.

The binary font starts with a height byte, additional metadata including a signed vertical offset used by the glyph blitter, and a first-character byte at +3. An unsigned-short offset array begins at +4. For a character at or above first-character, the offset at character-minus-first selects a glyph in the font block. A zero offset means no glyph. Each selected glyph begins with its width byte followed by packed bitmap bits. The offset table's valid extent and each bitmap's byte length are trusted asset contracts, not checked here.

Characters are unsigned bytes. These methods do not decode UTF-8, Unicode code points, shaping, combining marks or proportional kerning pairs. A tab or carriage return receives only whatever ordinary glyph lookup its byte value permits. NUL terminates text, and newline terminates the measured/drawn line without advancing to a second line.

## 2. Measuring one line

GetTextWidth (`0x4c1480`) returns zero for null font or text. Otherwise it walks up to NUL or newline, looks up each eligible glyph and sums its width byte. Bytes below first-character or with zero glyph offsets contribute zero and do not add a missing-character replacement space. Width therefore represents actual encoded glyph advances rather than character count times a fixed cell size.

Equivalent inline width calculations appear inside the three drawing functions. They use the current display font. Those null-safe width helpers do not make the later drawing routines safe with null font/text: DrawString subsequently reads font height and can pass a null text pointer to the lower-level blitter.

## 3. DrawString and its width limit

DrawString (`0x4c14f0`) accepts destination, text, X/Y and maxWidth. It measures the first line. A maxWidth of -1 disables trimming; otherwise a measured width exceeding the supplied limit enters a byte-removal loop. It copies at most 299 bytes to a 300-byte local buffer, finds its length and removes characters from the end until the recalculated line fits or the buffer becomes empty. It does not append an ellipsis, wrap words or split on glyph boundaries wider than one byte.

The copy uses strncpy without explicitly terminating the buffer. Input at least 299 bytes long can therefore make the following strlen scan uninitialized/out-of-buffer bytes. The width is not recalculated on the branch that empties the buffer and immediately exits, leaving a stale nonzero bounding width for an empty draw. A negative limit other than -1 is not normalized. Newline remains a first-line terminator during each width recalculation; trimming operates on the copied whole string's tail, potentially removing later-line bytes before changing the first-line width.

The acceptance rectangle is [X,Y,X+width,Y+fontHeight]. [RectInsideRect](../../../src/util/registry.cpp) requires both corners entirely inside the Surface's inclusive clip rectangle. The routine then draws the complete string or draws nothing. It does not clip partially visible glyphs. The rectangle uses width/height as the right/bottom addition rather than width-1/height-1, making edge acceptance conservative by one pixel relative to an ordinary filled rectangle. The glyph blitter also applies its font-specific vertical offset separately; that offset is not subtracted from the wrapper's tested Y bounds.

A null destination requests one screen lock. Failure returns without drawing; success obtains its clip rectangle, applies the same all-or-nothing test, draws and unlocks. A supplied destination is used directly. The void interface does not return whether a string fit, was rejected, or failed locking, and the caller's original text is not modified by trimming.

## 4. Glyph pixel behavior

The downstream [BlitText implementation](../../../src/graphics/blit.cpp), `0x4ccf60`, reads the font height and signed vertical offset, iterates byte characters and unpacks each glyph's bits most-significant-bit first. Set bits select the foreground color; clear bits select the background color. A selected color equal to the transparent-key color leaves the destination pixel unchanged. These are indexed palette bytes, not alpha coverage or antialiased RGB glyphs.

The blitter receives raw pixels/pitch and no clip rectangle. The wrapper's containment test and valid font metadata are therefore essential. Characters with missing glyphs are skipped with no X advance; valid glyphs advance by their width. Drawing stops at NUL or newline, matching metrics. This module supplies the wrapper contract; the completed [assembly blitter reference](graphics-blit.md) explains packed glyph traversal and lower-level prerequisites.

## 5. Centered and outlined strings

DrawStringCentered (`0x4c1760`) measures the line and chooses X=(destinationWidth-textWidth)/2 using a signed shift. Its third parameter is Y, despite the provisional name flag. It centers within the full Surface width, not the current clip rectangle. A null destination locks once and calls DrawString with that borrowed surface and unlimited width, then unlocks. Overwide text can receive a negative X and be rejected by containment; it is not automatically shrunk or wrapped.

DrawOutlinedString (`0x4c1830`) also centers horizontally. It sets text colors using the first supplied color and current transparent-key value, draws at X-1, X+1, Y-1 and Y+1 around the center, then changes the text foreground to the second supplied color and draws the central string. Thus its argument named fore supplies the outline pass and the argument named back supplies the central glyph color in the literal call order.

The color helper treats -1 as leave-unchanged independently for each field. Using a transparent key of -1 therefore does not necessarily install that value as the background; it can preserve the prior background field. The function does not restore previous text colors after drawing. Each of the five DrawString calls independently passes or fails full containment, so an edge can lose part or all of its outline. Null-destination locking failure performs none of those style/draw changes.

## 6. Coverage and limits

All four address-assigned functions and their metrics, containment, style and glyph-blitter dependencies were traced. Text content, font bytes and selected palette determine the visible names, HUD labels and messages. This module has no unit visibility decision, gameplay action, font save record or network protocol; higher interface code supplies text and layout. It does not count a source link or current-font pointer as a completed font load. Documentation is based on static source inspection without source listings, builds, installs or runtime drawing tests.
