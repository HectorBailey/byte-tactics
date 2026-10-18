# Explosion effects, model fragments and debris

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Weapon impacts](weapons-weapons.md)

Module group: `weapons/explosions`, 18 functions from `0x420620` through `0x421da0` in [explosions.cpp](../../../src/weapons/explosions.cpp) and [procedural frame generation](../../../src/weapons/explosions_420d20.cpp). This group owns visual explosion sequences, detached model pieces and face shards. It also contains the feature-at-position lookup consumed by reclaim/resurrection. Applying weapon blast damage is a separate module.

## 1. Three distinct storage pools

The game has 300 effect/debris entries, 300 reusable eight-vertex/six-face shard objects, and a separate 100-slot pointer array of detached model-piece records. These are not the 300 live weapon projectiles. Effects can use primary/secondary animation references, a model pointer, world position, inherited velocity, ballistic velocity, spin and impact-effect flag. A detached piece retains its owner unit pointer, model-piece index, motion/spin, lifetime, flags and copied model-state record.

InitExplosions (`0x420620`) clears the effect count, creates a 22 by 22 lens frame with parameter 8 and constructs three procedural sequences. Nominal duration/start/end settings are 24/64/8, 30/128/16 and 30/200/32. Each uses duration/2 frames lasting two ticks. Shade is assigned 20, 50 and 100 after the respective sequence construction, not passed as a per-frame argument.

It marks all 300 shard objects free with signed state -1, supplies eight vertices, six quad faces, firstFace -1, name explodepiece and fixed backing vertex/face arrays. Faces initially use flags bit 0, palette color 200 and predefined quad indices. It initializes the debris memory cache to 100000 bytes and zeros the 100 detached-piece pointers.

AllocExplodePieceObject (`0x420920`) chooses the first state -1 shard, changes state to zero and returns it, or null when full. Detached pieces instead use FindFreeExplodedPieceSlot (`0x421150`), then cache allocation. Effect/debris capacity and available shard geometry can fail independently.

## 2. Procedural explosion images

BuildCalcedExplosion (`0x420cb0`) allocates a 0x28-byte header plus eight bytes per frame, stores frame count and zero flags and creates each frame with duration two. Size starts at from and increments by integer `(to - from) / count`. Final size is generally not exactly to: the last generated index precedes count and the step truncates. Count zero is an unguarded divisor; initialization supplies valid counts.

BuildExplosionFrame (`0x420d20`) allocates a square frame, centers its offsets using integer size/2 and computes an elliptical noisy radial shade. Vertical squared distance is multiplied by 1.33. Each pixel adds integer random noise 0..9 to radial distance, converts through a byte-valued 32-step intensity, then selects transparent 0xff, edge color 0x6e or palette range beginning at 0x4f. Frame transparency is 0xff. It generates palette-indexed sprites, not runtime physical fire or a damage field.

Size and allocation success are assumed usable; size zero makes the radial normalization invalid. The startup sizes avoid that case. Model explosions, impact GAFs and these generated images remain separate asset/effect sources.

## 3. Adding and drawing an explosion effect

AddExplosionEffect (`0x420a30`) appends only below 300 entries. It copies position, initializes a primary sequence when supplied, initializes a secondary built-in sequence for nonnegative index or disables it for negative index, and clears model pointer. A null primary sequence does not prevent allocation or secondary animation.

When flag is zero and the position's whole Y is strictly above sea level, it additionally requests EmitSmoke with parameters 7,15,9. Nonzero flag suppresses that extra smoke, as used for water/lava splashes. Full capacity silently drops the effect, including its associated smoke request in this body. The index into built-in sequence pointers is not independently bounded here.

DrawExplosions (`0x420b00`) first draws detached pieces, then makes two passes over ordinary effects: secondary animation through DrawFrameShadow, followed by models and primary animation. Viewport inclusion uses projected center X and Z minus half Y, plus X128/Y32 offsets. It does not perform a separate per-player line-of-sight test in this function. A caller's use of effects/visibility should not be inferred from the viewport rectangle alone.

The shadow/secondary animation here projects from the effect position; it is distinct from the ground-height shadow calculations in live projectile rendering. Animation reference helpers choose actual frames. The explosion function does not manufacture missing textures or choose weapon damage from the drawn frame.

## 4. Combined effect/debris update

UpdateExplosions (`0x420f30`) first updates each of the 100 detached-piece pointers, clearing finished entries. It then updates all effect/debris records. A model shard adds both inherited size-vector motion and its own velocity, subtracts gravity from vertical velocity and adds spin to three angles.

It queries mean terrain height. Above sea, or over terrain at/above sea, falling to terrain restores the previous position and reverses/halves vertical velocity. If its resulting whole vertical velocity is nonpositive, optional impact explosion is spawned, the shard object's state becomes 0xff (free -1) and model pointer clears. Horizontal shard velocity is not halved in this branch.

Below/equal sea over lower terrain, optional impact effects select lava or water splash according to mission +0xd44, unless noSeaLevelTrigger at +0xd48 suppresses it. The shard is freed regardless. This differs from a detached whole piece's bounce policy below.

Both animation references then advance if nonnull. After updating, the routine repeatedly removes one fully empty record, shifts later records down and restarts until no empty record remains. An entry survives while any model or animation source remains. New impact effects can append during the walk; its loop tests the live count, unlike the snapshotted projectile-update count.

UpdateExplodedPieces (`0x421170`) separately exposes the same detached-piece update loop. DrawExplodedPieces (`0x4211a0`) exposes its draw loop. The combined and separate wrappers do not add a once-per-tick deduplication flag; calling both would perform their work twice. Main scheduling chooses which entry points run.

## 5. Starting a detached model piece

StartExplodePiece (`0x421620`) locates the unit's model-state record at state+0x22+index*0x36 and clears its visible flag bit 0 **before** allocating any effect. Flag b2 redirects to face shattering. Otherwise it obtains a free detached-piece slot and asks the cache for vertexCount*12+0x66 bytes. Failure leaves the original piece hidden without a replacement effect in this function.

The allocation contains a copied 0x30-byte request header, copied 0x36-byte piece record and transformed-vertex storage. It reconnects pointers to the copied record/storage and adds unit world position to local piece position. Original model geometry remains referenced through its descriptor rather than deeply cloning every texture/model face. The stored unit pointer later supplies owner-colored texture selection, so the larger unit/effect lifecycle must keep that reference usable.

Flag meanings established by consumers are b0 thrust-like particles, b1 white smoke, b2 shatter, b3 gravity and b4 terminal explosion/splash. Shatter uses b5 as its optional impact-effect switch. These flags control different paths; enabling shatter does not run the detached-piece movement code as well.

## 6. Whole-piece movement and expiry

UpdateExplodedPiece (`0x4213b0`) decrements lifetime and finishes when the old value was zero. A lifetime of N therefore permits N positive old-value updates before the following zero test ends it. Nonpositive unusual values are not normalized at creation by this routine.

Position already at/below sea immediately finishes. With b4 and enabled surface triggers, it adds lava/water splash first. This early sea test precedes querying terrain below the piece; it is not the same condition as ordinary face-shard water collision.

Above sea, it predicts next Y against current mean terrain height. Collision reverses/halves vertical velocity and halves horizontal velocities. Rebound below two world units per tick ends the piece, optionally creating ordinary explosion. Otherwise the piece moves using the modified velocity, accumulates all three angular velocities and applies gravity only with b3. It does not restore old position and use the ordinary shard's nonpositive-whole-velocity stop rule.

No unit health, feature damage, pathfinding occupancy or reclaimable wreck is created by this movement. Visual debris can disappear while a separately created terrain wreck remains, or exist without a reclaimable feature.

## 7. Drawing a detached piece

DrawExplodedPiece (`0x421550`) emits white smoke for b1 and jittered thrust particles for b0 during the **draw call**. Their frequency follows how this draw service is invoked, not an independent smoke timer here. It reorders the stored angles, rotates original model vertices into the copied transformed array and draws faces, returning one. The wrapper's zero-result cleanup path is thus not normally selected by this implementation.

DrawExplodedPieceFaces (`0x4211d0`) rejects centers outside the viewport, projects transformed vertices with the model-coordinate Z sign convention and walks faces. When firstFace is nonnegative it starts at face 1, rather than skipping an arbitrary firstFace index in this particular draw routine. This literal rule differs from shattering's per-index exclusion.

Solid-color faces use FillPolygon. Nonsolid faces draw only when they have four vertices, using an animated texture reference, owner-color frame or direct picture according to flags. Local arrays hold 2000 projected vertices and 25 polygon points without independent descriptor-count checks in the loop. Ordinary asset geometry must fit those limits. There is no new unit selection outline, construction wireframe or sensor contact generated here.

## 8. Breaking one piece into face shards

BreakPieceIntoDebris (`0x421700`) works from the piece's transformed vertices. Zero thickness scale becomes one. It scans faces, accepts textured/nonsolid quads and excludes the descriptor's firstFace index. Triangles and solid-color faces do not become these quad-box shards.

For each accepted face it first increments ordinary effect count, establishes piece-plus-unit world position and allocates a reusable shard object. If that allocation fails it returns with the newly counted record only partly initialized. Capacity therefore can suppress later faces and leave an incomplete last record rather than rolling back its count.

A unit with motion supplies half its current velocity as inherited drift. Without motion, that inherited field is not explicitly zeroed here, so reused entry state matters. Shard velocity starts from random components approximately centered on zero, with vertical gravity*30 added; spin independently ranges from -799 through 800. Primary/secondary animation references clear and b5 becomes the impact-effect flag.

The face's four vertices copy in forward/reversed order to make eight corners. A normalized face normal derives from float conversion using divisor **65535**, rather than the usual fixed-point 65536. Random normal-directed X/Z impulses modify velocity. The back four corners extrude along that normal by scale, then all eight vertices are centered by their average. The world position is not additionally translated by that computed centroid in this body.

Every face of the resulting six-face box receives the source texture/flags/color. Owner-color animated textures resolve to the owner's frame and clear animation-reference bit 1. This preserves a shard's team artwork after separation. Degenerate face normals and allocation/content validity are not independently repaired here.

## 9. Exploding an entire unit

ExplodeUnitPieces (`0x420e50`) iterates every model piece and constructs a request with lifetime 900, scale one, b4/b5 zero, random smoke bit, shatter bit b2 and gravity b3. It also chooses angular/linear velocities. Since b2 redirects immediately to BreakPieceIntoDebris, the whole-piece request's lifetime and chosen velocities do not govern the emitted face shards; shattering supplies its own velocity/spin. The random smoke flag likewise belongs to the bypassed whole-piece path.

This helper does not promise one detached object per original piece or every polygon flying away. Eligible quad faces, the 300 ordinary entries and reusable shard availability determine what appears. Unit death explosion weapons, terrain corpse creation and scripts can request additional effects through their own modules.

## 10. Feature lookup adjacent to effects

FindFeatureAtPos (`0x421da0`) converts fixed-point X/Z to 16-unit cells and queries the map. Outside returns 0xffff. Continuation ID 0xfffe subtracts origin offsets and queries the origin. IDs at/above 0xfffb return 0xffff. A valid result optionally writes origin cell and feature-definition footprint size, then returns definition index.

It does not test reclaimability, visibility, resurrection name mapping or resource amounts. The [service action](orders-unit-orders.md) adds those conditions. After resolving continuation it assumes a valid origin cell and does not independently compare the final index against a feature-count field in this function.

## 11. Teardown and technical boundaries

FreeExplosions (`0x420960`) performs its whole cleanup only when the lens pointer is nonnull. Inside it frees the memory cache, each procedural frame and sequence, and the lens, clearing the corresponding pointers. It does not independently clear every count/object/100-slot pointer after freeing. A null lens with other allocations present skips those frees, so partial initialization is not handled transactionally here.

There is no dedicated save/network serializer or health-damage routine in these bodies. Impact effects come from authoritative/replicated weapon events, while visual allocation can fail at finite capacities. Sound and camera shake are requested by weapon impacts separately. Construction nano particles, ship wakes and other scripted effects are documented through their own emitters and [COB unit interface](units-unit-script-calls.md).

All 18 annotated functions, procedural image generation, both fragment paths, capacity/failure behavior, drawing/update wrappers and feature lookup are covered through static source inspection. No builds or game execution were used, and exact named-unit fragment shapes remain model/script-dependent.
