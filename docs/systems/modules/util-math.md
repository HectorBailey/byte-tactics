# Checksums, random state, vector algebra and angle arithmetic

[Systems manual](../index.md) | [Roadmap](../../doc-writeup-roadmap.md)

The util/math group contains 25 callable entries between `0x4b6b80` and `0x4b73b0`: fourteen functions in [math.cpp](../../../src/util/math.cpp), RandomInt in [math_4b6c30.cpp](../../../src/util/math_4b6c30.cpp), and ten assembly entries in [math_4b70a0.cpp](../../../src/util/math_4b70a0.cpp). ENTRY markers are required to inventory the assembly routines. Source overview comments have two significant orientation/resolution inaccuracies, corrected from the actual instructions below.

## 1. Four-byte checksum

ComputeChecksum (`0x4b6ba0`) starts four byte accumulators at zero. For each data byte at zero-based index i, it adds the byte into A, XORs it into B, adds i XOR byte into C, and XORs i+byte into D. Each accumulator retains only eight bits. The returned integer packs A as its low byte, then B, C and D in increasing byte significance.

An empty or negative signed length performs no loop and returns zero. Positive input requires enough readable storage; no independent buffer-size argument exists. This is a lightweight compatibility/change checksum with collisions, not a cryptographic digest or corruption-proof validation.

[COB image loading](util-int-map.md) computes it before relocating file offsets and keeps it for interpreter-state compatibility. [Map metadata](map-map-load.md) and other consumers decide which bytes to supply. The helper neither loads a file nor inserts the checksum into any supplied header itself.

## 2. Random generator and advancement rules

SeedRandom (`0x4b6ca0`) stores (suppliedSeed XOR 0x66e29572) with its low bit forced to one. It does not restrict the value to a clean 1..2147483646 domain. RandomInt (`0x4b6c30`) returns zero immediately for range below two, without advancing the stored seed. Otherwise it computes a quotient by 127773, applies the source's 16807 multiplication and quotient correction in 32-bit unsigned arithmetic, adds 2147483647 when the resulting signed interpretation is nonpositive, stores the new seed and returns seed modulo range.

For ordinary positive Park-Miller state this implements the intended multiplicative modulus recurrence. The exact wrap/correction sequence matters for unusual high-bit seeds supplied through SeedRandom; documentation does not replace it with an arbitrary host-language random engine. Modulo range is not an independent rejection sampler, and requesting a trivial range consumes no random state. Changing call order or adding a nontrivial draw can change later simulation decisions.

This state is separate from CRT rand(), which [speech](sound-sound.md), particles and other source paths also use. Calling SeedRandom does not establish that every random-looking game system has been seeded through this one generator. COB's random instruction, unit initial headings/bobbing and scripted debris include concrete RandomInt consumers. There is no networking or seed-save routine inside this module itself.

## 3. Approximate and floating vector operations

ApproxDistance (`0x4b6e40`) returns max(abs(dx),abs(dy)) plus one quarter of the smaller magnitude, using an integer shift. It is not Euclidean square root and can under/overestimate that length depending on the ratio. It does not safely normalize the most-negative signed integer through abs or guard final addition overflow.

| Entry | Operation and storage behavior |
| --- | --- |
| DotProduct, `0x4b6e80` | Ordered sum bx*ax, then by*ay, then bz*az; float inputs, double result |
| VectorFromTo, `0x4b6eb0` | Componentwise to-from on float triples, returned by value |
| VectorFromToInt, `0x4b6f00` | Subtract integer components first, then convert each difference to float |
| CrossProduct, `0x4b6f70` | a cross b from six float products; return the resulting triple |
| NormalizeVector, `0x4b6ff0` | Float squared components, square-root length, divide each component by that length |
| VectorLengthFloat, `0x4b7070` | sqrtf(x*x+y*y+z*z) |

The integer difference form can overflow before conversion; it is not widened subtraction. Normalization has no zero-length guard or preferred fallback direction, so a zero vector reaches division by zero and inherits floating-point exceptional results. Very large/invalid inputs similarly have no saturation/finite-value checks. These helpers do not own their input objects or consult map geometry.

## 4. Angle units and lookup-table resolution

A full turn is 65536 angle units. A signed 16-bit angle used by the x87 path represents the equivalent centered range. Constants convert between angle units and radians using pi/32768 and its inverse. Sine/cosine table amplitudes use scale 8192, or 13 fractional bits.

FixedSine (`0x4b70a0`) zero-extends the angle's low word, shifts right six, clears offset bit zero and sign-extends the table word at that byte offset. Thus the element index is effectively angle>>7: 512 angular samples per turn. The comment describing 1024 entries/top ten bits is not the actual addressed resolution. The cosine entry (`0x4b70c0`) uses the identical lookup with g_cosTable.

The global labels in [globals.csv](../../../data/globals.csv) place g_cosTable 256 bytes after g_sinTable, 128 words or one quarter of this 512-sample cycle. They are overlapping windows of shared trigonometric storage rather than unrelated independently allocated tables. The first cosine value's stored 0x2000 also establishes the amplitude scale.

There is no interpolation between table samples and no full-precision floating sine call in these two entries. Angle wrap follows the low 16-bit representation and masked offset. Cardinal and nearby angles can map to the same table word.

## 5. Scaled sine/cosine and wide integer arithmetic

The scaled-sine entry (`0x4b70ef`) sign-extends the angle word, adds 0x20, shifts right six and masks the byte offset with 0x3fe before looking up g_sinTable. The scaled-cosine entry (`0x4b7123`) uses zero extension and adds 0x4020 before the same mask. Their final lookup is still 512-sample resolution; the 0x20 angular bias must not be described as a generic full-resolution trigonometric function.

Each multiplies the signed table value by a 32-bit scale to obtain a 64-bit product, adds 0x1000 with carry, then takes the shifted result at bit 13. This is a biased integer rounding step, including its asymmetric behavior for negative half cases. It does not clamp an output that exceeds 32-bit range. The caller supplies whether the scale represents pixels, velocity or another fixed-point quantity.

The entry at `0x4b70e0` returns the signed product's high 32 bits. It is not ordinary low-word multiplication or division by an arbitrary supplied scale. FUN_004b7381 (`0x4b7381`) instead forms a signed 64-bit a*b product and performs signed division by c. A zero c returns zero without division. A nonzero divisor whose quotient cannot fit 32 bits can still trap; there is no saturating overflow return. Signed division truncates toward zero rather than using the sine multiplier's bias.

## 6. Heading from a planar vector

FUN_004b715a (`0x4b715a`) loads its first integer, then its second integer onto the x87 stack, performs FPATAN, multiplies by radians-to-angle and converts back to a signed integer with FISTP. Because FPATAN uses the older stack element as the numerator, the result is atan2(first,second) in angle units. The source overview's atan2(second,first) description reverses that actual stack order.

This establishes the game's planar heading convention: first component zero and a positive second component gives zero angle; a positive first and zero second gives a quarter turn. Callers such as [unit positioning](units-unit-position.md) and script aim adapters supply/sign their deltas to obtain the desired facing. The routine itself does not mask the returned integer to an unsigned word, check a zero vector or install a private floating rounding mode. FISTP follows ambient x87 conversion state.

## 7. In-place two-coordinate rotation

FUN_004b7173 (`0x4b7173`) accepts an angle word and a pointer to two integer coordinates. A zero low-word angle returns without touching them. Otherwise it converts that signed angle to radians and uses x87 FSINCOS, not the coarse lookup tables. For original pair (u,v), the result is u*cos-v*sin and u*sin+v*cos, each converted back through FISTP and written into the same pair.

The two source values are loaded before either output is stored, so this specific pair interface is intentionally in-place. It has no null/bounds check or overflow fallback. Finite precision and the ambient conversion mode affect integer results, even though it uses floating trigonometry internally.

RotateAboutZ (`0x4b6d50`) applies that helper to X/Y and preserves Z. RotateAboutY (`0x4b6da0`) applies it to X/Z and preserves Y. RotateAboutX (`0x4b6df0`) applies it to Y/Z and preserves X. These names describe the chosen coordinate pair; handedness follows the helper's signs rather than an imported engine convention.

RotateByAngles (`0x4b6cc0`) applies angles[0] to X/Y, angles[2] to the resulting Y and original Z, then angles[1] to the resulting X/Z. It stores the final Y and then final X/Z. This specific sequence and its intermediate integer conversions matter: it is not an arbitrary interchangeable Euler order or one unrounded floating matrix product.

## 8. Shared 13-bit rotation matrix

The builder entry at `0x4b71a7` receives a structure pointer and reads angle words at offsets 0x0c, 0x0e and 0x10. It does not take a pointer directly to a three-word angle array at offset zero. It fetches coarse sine/cosine values and writes nine signed 13-fractional-bit coefficients into global g_rotationMatrix. Every intermediate product is shifted by 13 before later combinations, preserving the implementation's truncation order.

Let a, b and c denote those three angles, with s/c prefixes denoting their table sine/cosine values interpreted at unit scale. The matrix's conceptual output rows are:

| Output | X coefficient | Y coefficient | Z coefficient |
| --- | --- | --- | --- |
| X | cos(b)*cos(c) | -cos(b)*sin(c) | sin(b) |
| Y | cos(a)*sin(c)+sin(a)*sin(b)*cos(c) | cos(a)*cos(c)-sin(a)*sin(b)*sin(c) | -sin(a)*cos(b) |
| Z | sin(a)*sin(c)-cos(a)*sin(b)*cos(c) | sin(a)*cos(c)+cos(a)*sin(b)*sin(c) | cos(a)*cos(b) |

This table explains coefficient roles; it does not remove the actual fixed-point truncation between multiplications. Storage is by the columns used by the apply routine, so the first output row consumes stored elements zero, three and six.

The apply entry (`0x4b72e2`) takes input/output integer triples. For each output component it multiplies three inputs by the corresponding matrix entries, shifts each product separately by 13 and adds the shifted results. It writes X before rereading input X for Y/Z, so identical input/output pointers are not generally safe. It does not compute all three results into independent temporaries before writing.

Matrix construction overwrites shared global state, and application uses whichever matrix was built most recently. There is no matrix object parameter, nesting save/restore or module-level lock in these bodies. They do not automatically apply translation from the structure's first three integers. Callers manage matrix lifetime and coordinate offsets.

## 9. Error display, consumers and completion

ShowErrorBox (`0x4b6b80`) shows a parentless topmost message box with the global Error caption and supplied text. Its second integer argument is unused. It does not terminate the process, change the random seed or report a match failure to callers.

All 25 owned entries were reconciled globally and read, including the ten embedded assembly interfaces and separate random implementation. These arithmetic services support [model posing/rendering](graphics-model-render.md), [unit geometry and alignment](units-unit-targets.md), orders and scripts; they do not themselves choose an enemy, create a particle or draw a construction band. Persistent/network determinism depends on their consumers' state/order, not an implicit protocol inside the math module.

Documentation records the actual table indexing, x87 operand order, shared matrix and invalid-input limits rather than silently correcting source. It uses existing files only, without builds, installation, runtime numeric comparisons or copied source listings.
