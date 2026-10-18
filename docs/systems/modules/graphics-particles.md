# Particle pools, emission, wakes, nanolathes, smoke and visibility

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `graphics/particles`, 127 annotated entries from `0x470a40` to `0x476710`, before the `0x476740` boundary. Sources comprise [particles.cpp](../../../src/graphics/particles.cpp) and 18 retained companion files. The module owns six emitter families, their individual records, ten presentation lists and a shared emitter-object pool. Live weapon projectiles, model debris and explosion-effect pools are separate systems.

## 1. Emitters and particles have different lifetimes

The [ParticleSystem base](../../../src/graphics/particle_system.h) has a virtual destructor, Update, Render and IsFinished, plus an emission deadline. Derived classes add a next-emission tick, a vector of individual particles and family parameters. A pool slot holds an **emitter system**, while its individual particle vector uses ordinary allocated storage. One emitter can own several simultaneously moving particles.

SetLifetime (`0x471d70`) stores gameTick+ticks. This normally bounds future emission, not every particle's expiry. Individual expiry can use a separately calculated travel deadline, an animation count, terrain contact, or the shared emitter deadline. Init methods emit immediately before the system enters its list, so a zero emitter lifetime can still create an initial particle.

The six families are TeleportParticles, NanoParticles, ThrustParticles, WakeParticles, SmokeParticles and TimedSubParticles. The last name is provisional: its implementation uses smoke1 artwork and wind/gravity drift, without an automatic submarine-class or sea-height test. A named effect's actual source depends on orders, scripts and feature logic calling these entry points.

## 2. Fixed emitter pool and exhaustion

[ObjectPool](../../../src/graphics/object_pool.h) owns allocated blocks and a pointer table of free/used slots. Initialization uses 1000 slots of 0x4c bytes, enough for the largest declared emitter layout. Constructor/destructor wrappers (`0x470a90`, `0x470ae0`, `0x470b80`) establish/release that storage; InitParticlePool (`0x471c80`) constructs the global pool and registers ExitParticlePool (`0x471ca0`), whose destroyed bit prevents repeated normal exit destruction.

Grow (`0x470c10`, [retained implementation](../../../src/graphics/particles_470c10.cpp)) only adds capacity. It reallocates the pointer table and allocates one new backing block for the additional slots, populates their pointers and retains the block. A request no larger than current capacity returns success without changing size. Failed table allocation returns zero; failed backing allocation can leave the enlarged table pointer installed while capacity remains unchanged. Existing blocks are not relocated to a new slot size, and the routine does not reject an inconsistent size on a later growth request.

AllocSlot (`0x470e50`) returns the next pointer and increments used while used<capacity, otherwise zero. It ignores its size argument. FreeSlot (`0x470eb0`) decrements used and pushes the returned pointer into that free-stack position, without verifying membership, duplicate frees or underflow. ParticleSystem::operator new (`0x471d10`) returns null when g_fxEventPoolBlocked is set or no slot is available; otherwise it clears the requested object size in that slot. It neither grows the pool automatically nor verifies requested size against slotSize. operator delete (`0x471d50`) returns the slot.

Every creation wrapper checks allocation before initialization/insertion. Failed allocation silently omits the requested visual system. It does not roll back damage, build progress or a COB instruction. [CmdSFX](../../../src/game/share_commands.cpp) toggles the new-allocation block; existing emitters are still present and their update/render loops do not consult that switch.

FreeBlocks (`0x470ed0`) and destruction release the slot table and every backing block. FreeBlocks does not reset all scalar/pointer state afterwards, so it is not an idempotent initialized-empty reset. Container pointer destruction alone would not free the underlying blocks; explicit freeing in these methods is required.

## 3. Ten presentation lists and update ordering

[ParticleLists](../../../src/graphics/particle_lists.h) owns ten vectors of ParticleSystem pointers. Its constructor (`0x470f80`) creates those vectors; its destructor (`0x470fb0`) deletes every retained system. CreateParticleLists (`0x471d90`) allocates the game-owned collection; [DestroyParticleLists](../../../src/graphics/particles_471de0.cpp) (`0x471de0`) deletes and nulls it when present. Match lifecycle invokes those services; creation is not a guarded ensure-existing operation.

AddToList (`0x471160` in [its file](../../../src/graphics/particles_471160.cpp)) and the inline Add variants evict/delete the oldest system only when the existing size is **greater than 400**, then append the new system. Thus a normally growing list reaches 401 entries. It is not a strict 400-particle cap, and the global 1000-system pool can exhaust across lists before each list reaches that threshold. List indices are trusted signed-short arguments with no local 0..9 bounds check.

ParticleLists::UpdateAll (`0x471050`) and UpdateParticles (`0x471eb0`) visit lists in index order. They call IsFinished **before** Update. A finished system is deleted/erased; otherwise it advances once. They do not catch up with multiple simulated particle steps based on elapsed wall time. ParticleLists::DrawAll (`0x4710e0`), DrawList (`0x471120`), DrawParticles (`0x471f40`) and DrawParticleList (`0x471f90`) invoke Render without advancing time or deleting expired records.

For teleport, nano, thrust and wake systems, IsFinished is simply vector.empty. If all records disappear during Update, emission can still happen later in that same Update when due; if the system reaches the next outer update with an empty vector, it is deleted before another emission attempt. Their Init methods' initial emission is therefore significant. Smoke waits for both an empty vector and gameTick strictly beyond deadline. TimedSubParticles never reports finished.

Most expiry loops erase the current vector iterator without assigning erase's returned iterator; the shifted next record is then revisited at that storage address. Nano's update explicitly assigns the returned iterator. This follows the retained MSVC vector/pointer implementation, with ordinary iterator invalidation caveats; it is not a deliberate skip-every-other-particle rule. Reallocations/insertion and destruction remain meaningful ownership boundaries.

## 4. Visibility, projection and battle layers

Teleport, nano, thrust, wake and ordinary smoke drawing use the displayed player and projected information-grid coordinates: whole X/32 and (whole Z-whole Y/2)/32. Bounds failure hides the particle. With mapFlags bit 2, they require a nonzero byte in the player's grid, which [sight documentation](map-line-of-sight.md) establishes as **current sight count**, despite local names such as explored. Otherwise they test that player's bit in permanent visibilityMask.

These are point tests at the particle position, not tests of its entire sprite rectangle or the source unit's current radar/sonar flags. A visible shooter does not imply every later drifting particle remains visible. TimedSubParticle drawing omits this per-particle visibility gate, using blended smoke artwork directly. Upstream effect creation and final scene/fog composition can impose other restrictions, but those are separate from its Render implementation.

Screen placement uses whole X-scrollX+128 and whole Z-whole Y/2-scrollY+32, with several helpers narrowing to signed short. Sprite families use [GAF blended drawing](graphics-gaf.md). Nano and wake families use inclusive rectangles from left/top to left/top+1, yielding solid 2 by 2 indexed pixels through [FillRectangle](graphics-draw.md).

[Battle composition](../../../src/ingame/info_panel_468cf0.cpp) requests lists 0/1/2 early, 3/4 later, then 5/6/7, 8 and finally 9 around its other scene passes. Some calls have option gates. A list number is a presentation-layer choice by the caller, not a player ID or exclusive particle type. Existing call sites use layer 2 for wakes, 6 for construction/reclaim nanos, 7 for scripted thrust/bubbles and 9 for much weapon/smoke work. Teleport and feature callers use other layers. [Simulation scheduling](../../../src/ingame/game_dialogs.cpp) invokes UpdateParticles separately from those draw calls.

## 5. Teleport streams

ParticleLists::AddTeleportParticles (`0x471340`) and EmitTeleportParticles (`0x471fd0`) allocate, initialize and append a teleport system. Init (`0x4736e0`) stores source/target, computes their full 3D fixed displacement and a signed-short integer travel count derived from distance divided by five world units. It divides displacement by that count, then emits immediately. A distance below five world units can produce zero and divide by zero; extremely large distances can wrap the short conversion. The method does not clamp a minimum safe count.

Emit (`0x4737c0`) appends one record and schedules the next emission ten ticks later. It reserves approximate future capacity but does not emit all missing intervals at once. A record uses flameStreamSeq, a random initial animation frame, frameCount equal to asset frame count minus one, source position, constant travel vector and expiry now+travelCount.

TeleportParticle::Step (`0x473560`) adds that vector and advances frame modulo frameCount every update. The last asset frame is excluded by the count-minus-one/modulus representation. At most one-frame input gives an invalid zero divisor, with no local asset guard. IsExpired (`0x4736c0`) is currentTick>endTime, so equality survives that expiry test. DrawParticle (`0x473590`) uses the visibility/projection/blended-sprite policy above. The particles illustrate a stream between points; actual unit relocation belongs to the teleport order.

## 6. Nanolathe and reverse-nanolathe systems

AddNanoParticles (`0x471470`) takes two source/target bounds. EmitNanoParticles (`0x4720d0`) constructs a degenerate source bound with both endpoints equal to the supplied emitter point and uses a one-tick emission lifetime. EmitReverseNanoParticles (`0x472200`) swaps the broader source bounds and point destination, preserving the same system implementation. They do not debit or refund resources.

NanoParticles::Init (`0x473b50`) narrows each supplied segment/box axis to the interval from 4/11 through 7/11 of its original extent, then stores start plus extent. This central 3/11 sampling region controls where sparks originate and terminate; the full target model bounding rectangle is not uniformly filled.

[Emit](../../../src/graphics/particles_473d50.cpp) (`0x473d50`) makes five independent attempts per callback, choosing random XYZ positions in both narrowed boxes. Each velocity is target-source divided by a signed-short count derived from distance/four world units. Zero counts are skipped, so short/coincident paths can create fewer than five particles or none. Negative/wrapped counts are not independently rejected. A successful record expires at now+count, starts with flags/color 0xA1 through 0xA5 across these five attempts, and retains target and an otherwise unused 0x100 field. Next emission is now+1, still subject to the system deadline.

NanoParticle::Step (`0x4739b0`) adds velocity and cycles low color/frame bits through 1..7, preserving the relevant upper bits through its literal mask. IsExpired (`0x473b30`) uses strict now>endTime. [DrawParticle](../../../src/graphics/particles_473a00.cpp) (`0x473a00`) fills a 2 by 2 rectangle using the color/flags value after visibility acceptance. These particles do not select GAF nanolathe animation frames.

[Ground/common orders](orders-unit-orders.md) and [airborne services](orders-vtol-orders.md) decide when paid construction/repair or reclaim work requests forward/reverse particles; some action-specific branches differ in whether helpers' results gate emission. The renderer does not infer those economic outcomes. A hull can pulse through its separate construction shader without any new nano requests, and a failed/hidden particle request does not undo accepted progress.

## 7. Thrust streams and local jitter

AddThrustParticles (`0x4715a0`) and EmitThrustParticles (`0x472330`) retain two points, animation period and lifetime. Init (`0x4742c0`) sets deadline, computes the displacement and scales it by the reciprocal of lifetime before its initial emission. Lifetime zero causes a divisor failure; period zero later fails the record's modulus. Neither is locally normalized.

Emit (`0x4743a0`) creates one flameStreamSeq record per due update, with period, frame zero, tick zero and expiry equal to the emitter deadline. It schedules another emission at now+1. ThrustParticle::Step (`0x474130`) adds constant velocity every update; its animation changes only when its tick wraps modulo period, then advances modulo asset frameCount-1. IsExpired (`0x4742a0`) uses now>endTime. DrawParticle (`0x474170`) uses the usual visibility and blended projection.

EmitJitteredThrustParticles (`0x472ab0`) perturbs each whole coordinate by -1, 0 or 1, chooses lifetime 1..3 and uses the same perturbed point as both endpoints with period 1. Its displacement is therefore zero; animation and short expiry provide the effect. It is not a random velocity emitter. COB SFX callers supply other thrust point/period/lifetime combinations through the shared geometry interface.

## 8. Wakes and bubbles share one class

AddWakeParticles (`0x4716e0`) exposes period, lifetime and ascending mode. EmitWakeParticles (`0x472430`) uses lifetime 1 and ascending mode 1. EmitBubbles (`0x472530`) uses lifetime 1 and ascending mode 0. WakeParticles::Init (`0x474760`) retains source/target, derives their displacement, and scales it by a fixed reciprocal based on twice vector length. A zero-length segment is not locally guarded. The resulting direction is a small fixed movement vector, separate from the unit's continuing motion after emission.

Emit (`0x474880`) appends one record, randomly offsets each whole source coordinate by -3..3, and sets expiry to now+period*6. Its color range is 0x61..0x67. Ascending starts at 0x61 and increments; descending starts at 0x67 and decrements. It records smoke1 as data, but DrawParticle does not render that sprite: it draws a colored 2 by 2 rectangle. Next emission is now+1 until the emitter's short deadline ends.

WakeParticle::Step (`0x474580`) adds its constant velocity. Every period ticks it advances the color and wraps to the other end of the range when needed. Period zero is invalid. IsExpired (`0x474720`) keeps the record only when now<=endTime **and** GetGroundHeight at its current position is strictly below seaLevel. Ground exactly at sea level expires it. The test is terrain height under the particle, not a comparison of the particle's own Y to sea level. DrawParticle (`0x4745e0`) additionally applies point visibility.

The [script SFX interface](units-unit-script-calls.md) chooses source endpoints and normal/reversed direction, and selects periods such as 16 or 8. Ship class alone does not guarantee a wake request; script/movement SFX state determines whether a request is made. Likewise the bubble wrapper shares wake mechanics rather than introducing a separate buoyancy or water-volume solver.

## 9. Ordinary smoke: animation-count expiry and wind

[AddSmoke](../../../src/graphics/particles_471820.cpp) (`0x471820`) exposes frame limit, emission period, hold period, lifetime and alternate animation. [Smoke wrappers](../../../src/graphics/particles_472630.cpp) specialize it as follows:

| Wrapper | Family parameters |
| --- | --- |
| EmitSmoke, `0x472630` | Caller emission period/lifetime, ordinary smoke1, default hold period |
| EmitTimedBlackSmoke, `0x472720` | Caller emission period/lifetime, alternate smoke2 |
| EmitWhiteSmoke, `0x472810` | Period 1, lifetime 0, smoke1 |
| EmitBlackSmoke, `0x4728f0` | Period 1, lifetime 0, smoke2 |
| EmitWeaponSmoke, `0x4729d0` | Frame limit 3, period 1, hold 30, lifetime 0, smoke1 |

Construction (`0x474cd0`) initializes the next time to now. Init (`0x474d50`) sets deadline/position, selects smoke1 or smoke2 and obtains asset count minus one. A nonzero caller frame limit caps that value; hold period defaults to 7 when supplied zero. It emits immediately.

Emit (`0x474df0`) creates one record with timer=holdPeriod, animation count zero and a random limit beginning at 2 based on maxFrame-2. A valid sufficiently long animation is assumed. It reserves projected future capacity using emission period and schedules now+emitPeriod. An emission period of zero divides by zero here. Due testing (`0x475440`) requires nextTime<=deadline and nextTime<=now, so finite ordinary smoke stops producing new records, while existing puffs continue.

SmokeParticle::Step (`0x474b00`) adds windVelX*8 and windZ*8 to horizontal fixed coordinates and gravity*4 to Y each update. It does not integrate a separate ballistic velocity. When a decremented timer equals zero it advances animation count and resets timer to a random value from half the hold period through approximately the full period minus one. A period whose half is zero can reset timer to zero, after which decrementing to negative values does not trigger another equality-to-zero advance. Count reaching its random limit expires the record (`0x474cb0`), independent of a caller-supplied tick argument.

SmokeParticles::Update (`0x475340`) inlines the same drift/timer behavior, deletes expired puffs and emits at most once if due. [Render](../../../src/graphics/particles_475470.cpp) (`0x475470`) and the individual [DrawParticle](../../../src/graphics/particles_474b80.cpp) (`0x474b80`) apply point visibility then blended GAF frame count. IsFinished (`0x474f80`) requires no puffs and deadline strictly before now. Animation limits generally stop before the final asset frame; they are not full-sequence loop references.

## 10. TimedSubParticles is a persistent emitter

[AddTimedSubParticles](../../../src/graphics/particles_471a50.cpp) (`0x471a50`) exposes period/hold/lifetime. EmitTimedSubParticles (`0x472c50`) uses emission period 5, default hold and lifetime 150. Constructor (`0x4750b0`) initializes next emission time. Init (`0x475150`) records the deadline and smoke1 parameters, but the subsequent behavior does not use that deadline to stop the emitter.

[IsEmitDue](../../../src/graphics/particles_4750f0.cpp) (`0x4750f0`) checks only nextTime<=now. IsFinished (`0x475330`) always returns zero. Therefore an emitter created through the nominal 150-tick wrapper continues producing records after that time until external list eviction or destruction removes it. This is a substantive difference from ordinary smoke, not a generic finite-life interpretation of the class name.

Emit (`0x4751c0`) uses the same random smoke1 frame-limit/hold scheme and schedules the next period. Its reserve estimate can become nonpositive after the deadline, but that does not suppress appending a record. Step (`0x474fc0`) and Update (`0x475600`) use wind*8 and gravity*16, four times ordinary smoke's vertical drift. IsExpired (`0x475090`) tests animation count>=limit. DrawParticle (`0x475040`) and [Render](../../../src/graphics/particles_475700.cpp) (`0x475700`) blend smoke1 at the projected position without ordinary smoke's local sight test.

## 11. Container support and complete module boundary

The address inventory contains 100 entries in particles.cpp, ten in particles_472630.cpp and one in each of 17 other retained files, totaling 127. The main file includes lifecycle constructors/deleting destructors for the pool, base and emitter families. Static emission scaffolding in the decompilation accounts for some constructor/vtable/destructor entry generation; it should not be interpreted as an extra gameplay emission policy.

CopyStructRange (`0x470a40`) copies packed 14-byte records; CopyPointer (`0x470f60`) copies a pointer-sized value when destination is nonnull. The 0x470ef0..0x470f30 and 0x472d30/0x4732d0/0x473500/0x473530 helpers implement trivial-element vector size, destruction, uninitialized copy and fill. NanoParticleVector::GetCount (`0x475840`) returns pointer-byte difference divided by 48, or zero for a null first pointer, with reserve/copy/fill/destruction support at 0x475770, 0x475870, 0x475880 and 0x476710.

Retained vector insertion specializations cover [system pointers](../../../src/graphics/particles_4732e0.cpp), [52-byte teleport records](../../../src/graphics/particles_4758c0.cpp), [60-byte thrust records](../../../src/graphics/particles_475bd0.cpp), [68-byte wake records](../../../src/graphics/particles_475ef0.cpp), [32-byte smoke](../../../src/graphics/particles_476210.cpp) and [32-byte TimedSub records](../../../src/graphics/particles_476490.cpp). Growth allocates/copies storage and invalidates old iterators; erase shifts records. Those record sizes are separate from the 76-byte **system** pool slot. Element-vector destruction releases storage without freeing borrowed GAF assets.

All six families, member/free creation wrappers, pool/list services, record simulation/display and support entries are covered. The module has no particle save-bank format, replication packet, damage application or construction ledger. It consumes requested effects, tick/environment data and borrowed animation assets, then supplies transient presentation. Pool blocking/exhaustion, layer choices, visibility, raw parameter limits and the persistent TimedSub behavior explain why an effect can be absent or outlive its nominal request. No source listings, builds, installs or game execution were used for this documentation.
