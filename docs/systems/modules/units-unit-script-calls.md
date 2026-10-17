# COB unit-value interface, piece changes and scripted effects

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_script_calls`, twenty-eight annotated entry points from `0x4804e0` through `0x4814b0`, in [unit_script_calls.cpp](../../../src/units/unit_script_calls.cpp) and [unit_script.cpp](../../../src/units/unit_script.cpp). This module implements UnitScript's game-facing virtual callbacks, aim completion and small grid/math helpers. The COB interpreter schedules bytecode and invokes these services; this module connects those requests to live units, model state, effects and attachment.

## 1. UnitScript and model-state binding

UnitScript derives from CobScript and retains the unit's ObjectState pointer. That model state contains owner, dirty/cached-picture fields and 0x36-byte piece records with translation, rotations, derived position, SFX offsets and visible/cached/shaded bits.

`SetObjectState`, `0x480d40`, assigns that pointer. [Unit initialization](units-units.md) creates the script/object state, binds them and runs Create. The deleting destructor is part of the unit-lifecycle module. Methods here generally assume valid binding, piece indices and axis indices rather than performing complete validation for every virtual call.

## 2. Reading unit values

`GetUnitValue`, `0x480770`, implements property IDs 1..20. Unknown IDs and failed optional unit lookups return zero. Optional unit IDs are narrowed to unsigned short and zero means no unit; nonzero array indices are not independently bounded here.

| Property ID | Confirmed returned value |
| ---: | --- |
| 1 | Activation byte bit 0 |
| 2 | Standing movement mode, two bits |
| 3 | Standing fire mode, two bits |
| 4 | Health times 100 divided by definition maximum |
| 5 | COB state bit 0, used as build stance |
| 6 | COB state bit 1, used as busy |
| 7 | Queried piece's world X/Z packed into one integer |
| 8 | Queried piece's fixed-point world Y |
| 9 | Live queried unit's packed world X/Z |
| 10 | Live queried unit's fixed-point world Y |
| 11 | Live queried unit definition's model maximum Y |
| 12 | Angle of a packed X/Z vector relative to this unit's heading |
| 13 | Magnitude of a packed X/Z vector |
| 14 | Angle helper for the two supplied components, masked to one turn |
| 15 | Hypotenuse of the two supplied components |
| 16 | Fixed-point ground height at a packed X/Z point |
| 17 | Quantized percentage of construction remaining |
| 18 | COB state bit 2, yard-open state |
| 19 | COB state bit 3 |
| 20 | Activation byte bit 1, armored state |

Health percentage is not clamped in this getter. A pending lethal unit can therefore differ from a HUD/script event path that explicitly clamps percentages. Model maximum Y is definition geometry, not automatically the target's ground-relative top after adding its current position.

Construction remaining is zero for exactly completed buildLeft. Otherwise it returns 1 minus the truncated product of buildLeft and minus 99, giving a discrete script-facing scale. This differs from the network eight-bit construction representation and the raw float used by economic work.

Packed X/Z combines the upper X word with the signed whole Z contribution. Vector/ground queries compensate the high component when the reconstructed low component is negative. These legacy packed values must be decoded with that signed convention, not treated as a pointer or two arbitrary unsigned screen coordinates.

## 3. Writing unit values and the yard gate

`SetUnitValue`, `0x480b20`, supports a subset of the getter IDs:

- 1 sets/clears activation bit 0 through SetStateBits, including activation callbacks and replication.
- 5 and 6 set the build-stance and busy COB bits.
- 18 invokes SetYardOpen rather than blindly writing its stored bit.
- 19 writes COB bit 3.
- 20 sets/clears activation bit 1 through SetStateBits.

Every call marks the unit's dirty bit at +0xba, including an unsupported property that performs no switch action. Script-set stance/busy bits unblock the [order wait gates](orders-order-queue.md).

The yard setter checks occupancy before changing open/closed claims and refreshing passability, as explained in [terrain](map-terrain.md). A script request to open/close a shipyard is therefore not merely a model animation toggle, and a blocked yard transition need not complete just because COB issued the set operation.

## 4. Piece translation and rotation

GetPieceTranslation (`0x480c30`) and GetPieceRotation (`0x480cb0`) read selected axis state directly. SetPieceTranslation (`0x480c50`) and SetPieceRotation (`0x480ce0`) update only when different, clear the piece's cached value, mark object state dirty and invalidate an additional cache field when the piece is cached. Rotations compare/store as unsigned-short angles, retaining one-turn wrapping.

These callbacks mutate model-instance state, not the unit's world position or terrain footprint. A turret moving or a construction scaffold extending need not move the unit record. Later object-state calculation propagates piece hierarchy changes into geometry and effect positions.

## 5. Visibility, caching and shading flags

SetPieceVisible (`0x480d50`) updates the visible bit when changed, clears the piece cached value and invalidates the additional cache field for a cached piece. Its shown body does not set the object dirty field in exactly the same way as translation/rotation setters.

SetPieceCached (`0x480db0`) and SetPieceShaded (`0x480df0`) assign their respective bits and reset the cached object-picture/frame field. IsPieceVisible (`0x480e30`), IsPieceCached (`0x480e50`) and IsPieceShaded (`0x480e70`) return those bits.

Visible controls a piece's presentation, cached controls participation in picture caching, and shaded controls a rendering property. These are distinct from unit visibility/sensors and buildLeft. Their invalidation effects connect script animation to [cached picture storage](units-unit-motion.md); they do not independently select construction wireframe colors or world draw ordering.

ExplodeLegacy (`0x480e90`) and PlaySoundNoop (`0x480ea0`) are empty overrides in this revision. Calls through these slots do not automatically create the modern piece explosion or play a sample. Other supported bytecode/callback paths provide effects/audio.

## 6. Script SFX coordinate preparation

`EmitSfx`, `0x480eb0`, first tests the unit against the viewing player's visibility helper. An invisible unit returns before effect preparation. Otherwise it updates object state so animated geometry is current.

For IDs with bit 0x100, it derives one world point from the unit position plus the chosen piece's computed position, reversing model Z. Other IDs derive two world points from the piece's six-integer SFX offset array. Those two points establish an emission segment/direction and can be reversed by the selected effect ID.

This visibility gate affects script-emitted presentation, not the underlying simulation's movement, damage or sonar contact. An observer can receive authoritative unit state without every hidden wake/smoke particle being instantiated locally.

## 7. Supported thrust, wake, smoke and bubble IDs

The switch recognizes exact IDs, not every integer sharing one bit:

| SFX ID | Dispatch |
| --- | --- |
| 0 | Thrust between point 1/2 with parameters 1, 6, 7 |
| 1 | Thrust between point 1/2 with parameters 1, 7, 7 |
| 2 | Wake point 1 to 2, amount 16, type 2 |
| 3 | Wake point 1 to 2, amount 8, type 2 |
| 4 | Wake point 2 to 1, amount 16, type 2 |
| 5 | Wake point 2 to 1, amount 8, type 2 |
| 0x101 | White smoke at point 1, index 9 |
| 0x102 | Black smoke at point 1, index 9 |
| 0x103 | Bubbles from point 1 toward a copy of that point at sea level, amount 8, index 7 |

Unknown IDs produce no matching emission after preparation. Named helper parameters are preserved where the downstream particle module supplies their detailed meaning. The effect table alone does not establish stock texture/frame names, colors or lifetimes.

Wake direction follows the piece endpoints and ID, not an unconditional assumption that every wake trails opposite unit heading. COB/model assets choose which piece emits and when. Bubble ID 0x103 explicitly uses sea-level Y for its second endpoint, explaining a vertical reference distinct from a submerged model-piece origin.

## 8. Exploding pieces and additional visual sources

`ExplodePiece`, `0x481140`, has two independently enabled branches. When input bit 0x20 is absent, it builds a 0x30-byte debris request containing owner/piece, three random rotation-like values below 3000, X/Z velocities in quarter-unit increments around zero, nonnegative random Y velocity below ten fixed-point world units, parameter 900 and selected remapped flag bits. It sends that request to StartExplodePiece.

The remapping depends on the low input bits, including a mode distinction for bit 0 and separate shifts for the other flags. The local header flag word is merged before it is initialized. The source identifies surviving high bits from prior stack contents as a suspected original bug. A deterministic explanation that every debris flag starts at zero would contradict the body.

Independently, any of input bits 0x100 through 0x2000 requests an explosion effect at the piece world position from its corresponding entry in a six-source table. Multiple bits can append multiple effects. Bit 0x20 can skip the debris branch while still permitting these source effects.

Unlike EmitSfx, this method does not perform the same initial unit-visibility return in its own body. Downstream explosion/debris rendering supplies additional visibility/lifetime behavior. The source-table entries, animation art and collision/damage rules are subjects of the explosion/particle modules, rather than inferred from the word explode alone.

## 9. Script transport operations

`AttachUnit`, `0x481340`, resolves an ID and requires a live target that is uncarried or already carried by this script's unit. It then requests attachment to this unit with the supplied piece/mode. The shared attachment service repeats structural guards and broadcasts/applies the change; this callback does not itself enforce all capacity or unit-type policies of pickup orders.

`DropUnit`, `0x4813b0`, requires the live target to be carried by this unit. It tests placement at that target's stored cell in mode 1, then requests detachment with piece minus one and mode 1. A failed footprint test leaves the unit attached. Calling drop is consequently not a guarantee of successful unloading over blocked shore or incompatible terrain.

`IsCarryingUnit`, `0x481430`, searches the child chain for the requested ID. `GetTransporterId`, `0x481470`, returns the script unit's carrier ID or zero. These are runtime relationship queries, not independent searches for a nearby transport/pad.

## 10. Aim completion callback

`WeaponAimCobCb::OnAimCobReturn`, `0x481490`, sets nAimCobDone to 1 when the script returns a nonzero value. Zero does not clear an already set completion flag in this body. Firing code must reset/setup its callback state for each aim request and then test this result as one of its gates.

`AimCobStub`, `0x4814b0`, returns zero without other work. A COB aim thread returning nonzero communicates readiness; it does not itself spawn a projectile or debit weapon resources. [Target resolution](units-unit-targets.md) and weapon firing supply the surrounding steps.

## 11. Grid/math helpers

`StepTowards`, `0x4804e0`, moves a scalar by the supplied step and clamps at the target to avoid overshoot for ordinary positive steps. `StepPointTowards`, `0x480510`, instead adds/subtracts the step independently on each differing coordinate without an equivalent overshoot clamp. The point helper must not inherit the scalar helper's stronger guarantee.

`ChebyshevDistance`, `0x480570`, returns the larger absolute coordinate difference. `ClampPointToSegment`, `0x4805b0`, chooses an endpoint using those distances, limits a step by endpoint separation and calls StepPointTowards. It is this grid-step construction, not an exact Euclidean projection onto an arbitrary sloping segment. Applying the same step to both differing axes can place a point outside the ordinary straight segment.

`OctantBetween`, `0x480720`, encodes only the signs of coordinate differences: negative Y alone gives 0, negative X/negative Y 1, negative X alone 2, negative X/positive Y 3, positive Y alone 4, positive X/positive Y 5, positive X alone 6 and positive X/negative Y 7. Coincident points also return zero. It is not a magnitude-weighted angular sector calculation.

## 12. Completeness and investigation sequence

All twenty-eight annotated entry points are covered. This interface owns neither COB bytecode serialization nor its scheduling loop, and defines no new independent save account or transport packet format. It invokes shared activation/attachment events and effects, whose owners define those contracts.

For construction animation, trace COB piece changes, cache invalidation, build stance/busy values and yard permission separately from the renderer's build-progress mode. For naval wakes/smoke, trace SFX ID, visibility, piece endpoints and particle dispatch. For a gun that aims but does not fire, inspect per-request callback reset and nonzero return before later reload/resource gates. For an unload that stays attached, inspect the actual footprint-placement result rather than just the script's drop instruction.
