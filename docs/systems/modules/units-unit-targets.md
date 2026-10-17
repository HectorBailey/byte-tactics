# Unit references, target storage, damage and ground alignment

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `units/unit_targets`, from `0x489280` through the boundary before `0x48a7f0`. The main [unit_targets.cpp](../../../src/units/unit_targets.cpp) contains 23 annotated entry points, and [unit.cpp](../../../src/units/unit.cpp) contains six more within this range. This module combines reference-list lifetime, compact weapon targets, damage/healing/paralysis, unit interaction eligibility, definition-property formatting and model-ground alignment. Weapon eligibility and projectile simulation are separate consumers of these services.

## 1. References protect reusable unit slots

A [UnitRef](../../../src/units/unit_ref.h) is a 0x10-byte node containing a virtual-table pointer, referenced unit, next-node link and optional listener. Units keep the head of their incoming-reference list at +0xa2. The field called owner on the reference means the unit being referenced, not the unit's player.

`LinkToUnit`, `0x489540`, prepends to a nonnull unit whose definition index is nonzero. Otherwise it clears the reference. It does not first unlink an existing association. `UnlinkFromUnit`, `0x489580`, searches the current owner's chain, reconnects the predecessor and clears the node. The search assumes list membership and does not handle a missing node as a separate failure case.

PathOrderAttach construction (`0x4895c0`), its deleting destructor (`0x489600`), the unlink/destructor entry (`0x489650`) and SetUnit (`0x489690`) apply the same intrusive-list contract. SetUnit removes the old association before attaching a valid new unit. These objects carry a listener value used by their owning order/target object; linking does not transfer ownership of the referenced unit.

`ClearRef`, `0x4896f0`, saves its unit, notifies the listener with event 8, then unlinks only if notification left the same unit associated. The callback can detach or redirect the reference, so blindly clearing it afterward would discard a deliberate new association.

`ClearUnitRefs`, `0x489740`, repeatedly processes the current list head with that event-8/unchanged-owner rule until the old unit has no incoming references. [Unit death](units-units.md) calls it before final component release and slot reuse. `NotifyUnitRefs`, `0x4897b0`, instead walks the chain once and forwards an arbitrary event without automatic clearing. Its direct next-link traversal has a different mutation contract from the head-restarting destruction loop.

These routines do not validate arbitrary pointers or repair corrupted membership. They ensure orderly reference invalidation when callers use the attach/detach API consistently, which matters because unit IDs can later identify another occupant of the same slot.

## 2. Compact weapon-target encoding

Each of the three 0x1c-byte weapon slots starts with a pair of 16-bit target values. The second value 0x8000 means a unit-ID target; first value zero then means no target. A different second value encodes a world X/Z ground point in whole units.

`SetWeaponTargetUnit`, `0x48a060`, writes the target slot ID and sentinel. `SetWeaponTargetPos`, `0x48a0a0`, stores the high words of fixed-point X/Z and changes a Z of 0x8000 to 0x8001 to avoid colliding with the unit-target tag. Both clear bits 10..14 in the unit's network-dirty field. They do not perform weapon range, alliance, sight, sonar, altitude or damage-eligibility checks.

`ResetWeaponTarget`, `0x48a160`, writes the empty pair without callbacks. `ClearWeaponTarget`, `0x48a0f0`, does the same only when nonempty, looks up the StartBuilding script name and invokes TargetCleared with the weapon index. The StartBuilding operation here is FindScript, not starting construction. The implementation expects the appropriate script interface to exist on this callback path.

`GetWeaponTargetUnit`, `0x48a190`, returns a global unit-slot pointer only for a nonzero unit-ID encoding. It does not itself verify that the slot is still occupied. `IsTargetingUnit`, `0x48a440`, compares all three encoded IDs with the supplied unit's ID. These are storage queries, not complete target-validity decisions.

`ChooseWeapon`, `0x4897e0`, prioritizes slot 0 when its flag bit 1 is set, otherwise slot 1, otherwise returns slot 2's masked bit. Thus slot 2 yields 2 when that bit is set, but the all-clear case yields 0. A returned zero is not sufficient evidence that slot 0 is usable; callers need the surrounding flag/eligibility state.

## 3. Resolving an aim position and leading a target

`GetWeaponTargetPos`, `0x48a1e0`, reconstructs a point target's X/Z in fixed point and uses the greater of sampled ground and sea level for Y. It does not preserve an arbitrary input Y. This gives ground-point targeting a surface-relative interpretation.

For a unit target it rejects the empty pair. If the referenced slot has definition index zero, it clears the pair, performs the TargetCleared callback and returns zero. Otherwise it obtains the target's script/model sweet spot.

The routine can then lead a moving target when the weapon-slot bit is set, the weapon is not cruise, the target has motion, the firing unit has more than five kills and weapon speed is nonzero. The weapon layout is established in [weapon_def.h](../../../src/weapons/weapon_def.h): the provisional radius field at +0x68 in this file is weapon speed, and bit 25 is cruise.

It computes three-dimensional distance to the sweet spot, derives a time-like scale from distance divided by weapon speed multiplied by 0xcccc/65536 (approximately 0.8), and adds the target motion vector times that scale. This is a velocity-based approximation with explicit gates, not a general acceleration-aware intercept solver. Projectile firing and later collision still determine whether the shot hits.

## 4. Damage event construction and scaling

`DamageUnit`, `0x489bb0`, builds a nine-byte type-11 event: victim ID, optional attacker ID, 16-bit amount, heading high byte and damage-kind byte. It first computes the amount except for kind 10, the healing path.

For other kinds, activation bit 2 enables the definition's 16.16 damage modifier when the requested amount is below 30000. Experience then reduces damage by four percent per five kills, up to five reduction steps (20 percent at 25 kills). The integer computation applies these factors before narrowing into the event amount field. Kind 10 bypasses these damage reductions.

The routine applies the event locally, then broadcasts when the victim has an active type-3 owner and the kind differs from 11. The packet's sender identity comes from the source player's network ID when a source exists, otherwise the local identity. The body does not spread damage to all units of the same definition; a provisional source comment should not be interpreted as that behavior.

Only the high byte of the supplied 16-bit extra/heading reaches the event. A caller supplying a direction only in the low byte will consequently transmit zero in that field. Range/ownership validation of packet IDs is outside this primitive, and normal damage scaling dereferences the target before its optional-ID encoding, so a null victim is not a valid general input.

## 5. Applying healing, ordinary damage and script reactions

`ApplyUnitDamage`, `0x489ce0`, resolves victim/attacker slots. It rejects an absent victim, a victim lacking the live flag, or one already marked with 0x4000. It does not independently bound-check every nonzero slot ID.

Kind 10 adds the event amount to health and clamps to definition maximum health, then returns. It does not run the ordinary attack reaction or damage-script branches. The receiving amount is interpreted as an unsigned 16-bit field, so this is an encoded healing amount rather than an arbitrary signed health delta.

Other kinds mark the victim recently damaged, call attack reaction except for kind 11, record the damage kind, and, when an attacker exists, update last-attacker pointer/player slot. Local participation in the hit can add an activity sample for music. An event without an attacker does not explicitly reset the previous attacker metadata in this body.

For ordinary nonparalysis kinds it subtracts the amount from health. At nonpositive health, active type-1/type-2 owners get pending-death bit 0x4000 and the function returns. Other roles have health clamped to zero. The full [death lifecycle](units-units.md) is separate from this health mutation, explaining why damage application itself does not immediately free every unit component.

For kind 1 surviving this branch, it converts the heading byte to two directional script arguments using trigonometric helpers with scale 400, starts HitByWeapon and starts TakeDamage with health percentage clamped to 0..100. These callbacks expose the hit to COB animation; their actual effects depend on the loaded script. Lethal locally controlled hits return before these callback calls.

## 6. Paralysis is an order effect

Kind 2 uses the event amount as a tick duration rather than subtracting health. It requires a live, non-pending-death unit, an active type-1/type-2 owner and absence of the definition's 0x4000000 immunity bit. If the current order is already paralyze it extends that order's ticks; otherwise it appends a newly constructed paralyze order.

`ParalyzeUnit`, `0x489fa0`, provides the same guarded order-effect operation directly. Despite a provisional comment referring to completed construction, its shown body does not test buildLeft. The confirmed guards are the live/pending flags, owner role/activity and definition immunity.

The existing-effect test looks at the current order head, not every queued order. The routine itself does not define paralysis expiration behavior, animation or blocked actions; those belong to the paralyze order handler and scheduler. A kind-2 event that fails the owner/immunity conditions still does not fall through into ordinary health subtraction.

## 7. Build-list membership and property text

`UnitCanBuild`, `0x4894f0`, scans the definition's buildable-ID table and returns whether the requested ID occurs. It does not check current metal/energy, build-site geometry, per-type limits, factory state or work range. It answers definition membership, one part of a construction decision.

`MakePropList`, `0x489280`, allocates a 0xc0-byte buffer and writes a sequence of terminated property strings. It includes resource costs, one additional definition statistic, separators, and mobile speed/acceleration/turn-rate strings with translated units. Speed and acceleration use fixed-point conversion, tick rate and a 0.4 display-unit factor. Nonmobile definitions receive N/A strings for movement properties.

The output is a sequence of strings rather than one ordinary paragraph. The caller owns the returned allocation. The formatting body uses legacy unbounded appends within its fixed allocation; it contains no independent total-length check for expanded localized unit labels. The unnamed definition statistic remains described by its field role here rather than assigning an unsupported original name.

## 8. Four-point model-ground alignment and water bobbing

`AlignUnitToGround`, `0x48a490`, obtains a designated row of four vertex indices from the unit model. A negative model count returns without changes. Each vertex's X/Z is rotated by heading and translated into world coordinates, then bilinearly samples the terrain's neighboring height bytes. A sample outside the usable right/bottom-neighbor range returns before final alignment writes.

For a definition with bit 0x1000, while live and not pending death, the sampled height is raised to at least sea level. A phase derived from GetTicks, vertex number and the unit's randomized hover phase adds a small trigonometric offset. Its amplitude depends on current motion speed relative to half definition speed and fades according to elapsed simulation ticks since the motion timestamp, clamped at 60 ticks. This mixes a time-service phase with a simulation-age envelope; it is not a fixed displacement applied equally to every ship every frame.

From the four sample heights, the final code sets the whole word of world Y to the average support height, computes pitch from the difference between the two end-pair means, and computes bank from the side-height difference. Heading is used for sampling and is not rewritten by the final stores. A provisional local name calls the Y whole word roll, but its offset lies inside position Y, as established by the unit layout. Reading it as a separate third rotation would misdescribe the operation.

The sea-clamp/bob branch assumes the expected motion object and a usable definition speed denominator. This function provides alignment of a model's support geometry, separate from route passability, collision, footprint claims and construction support-height validation.

## 9. Claiming and releasing weapon control

`ReleaseWeapons`, `0x489800`, and `ClaimWeapons`, `0x4898b0`, operate on an enabled weapon slot's bit 4. Release requires that bit clear and sets it; claim requires it set and clears it. On the transition they clear a nonempty target and invoke TargetCleared with the slot index. Their StartBuilding operation is a script lookup, not a construction action.

Index 3 is an all-slots request, implemented by recursively processing slots 0 and 1 then continuing with slot 2. It is not a fourth weapon. Repeating the same operation while its required bit transition is absent does not repeatedly clear the target. These helpers coordinate weapon ownership/control state rather than freeing weapon-definition assets or firing a shot.

## 10. Reclaim, repair and transport eligibility

`CanReclaim`, `0x489960`, requires the acting definition's reclaim bit (flags2 0x400), rejects target runtime mode 2 and rejects a target definition with flags2 0x1000. It does not itself validate player alliance, distance, work rate or whether the target pointer is null. The order/dispatcher supplies the rest of the command context.

`CanRepair`, `0x4899b0`, requires a target, the actor's repair bit (flags2 0x200), target health different from maximum, and target mode other than 2. It tests the target's whole Y plus model-top height against water limits. A flying actor without the relevant submerged-operation permission rejects a top below sea level. A nonflying actor instead rejects a top below sea level minus the actor's maximum water depth. Equality passes these below-limit tests. Health different from maximum is the literal predicate, not a separate less-than clamp.

`CountCargo`, `0x489a70`, walks the child chain and counts only nodes whose carrier points back to this unit. `CanLoad`, `0x489a90`, requires actor transport capability, remaining capacity, a target with motion, target footprint X no greater than the actor's size capacity, target mode other than 2, absence of the target's no-transport definition bit, and exactly zero buildLeft. It also rejects a fully submerged target whose model top is at or below sea level.

A nonflying transporter additionally requires the target's minimum water depth to be negative. That is a signed definition test, not merely a check that the target looks like a ground vehicle. Capacity is read as an unsigned byte. This helper checks footprint X, not independently both footprint axes. It does not itself establish an alliance, claim a free model piece, perform approach movement or attach the target. Those steps remain in transport orders and attachment services.

These predicates help explain why a unit can be selectable or spatially nearby yet fail a service interaction. They are narrower contracts than every gate required for a completed repair, reclaim or pickup.

## 11. Boundaries and completeness

All 29 annotated entry points, including the shared-file capability methods, reference destructors and simple target queries, are covered. The module emits/consumes the compact damage event but owns no independent save-bank serializer. Weapon target persistence and reconstruction belong to unit save/position services; intrusive listeners are runtime relationships rebuilt through their owners.

To explain what shoots at what, combine these target encodings and aim resolution with weapon eligibility, sensor visibility, standing fire orders, script aim callbacks and firing/projectile handlers. A stored target is not proof that every permission and firing gate passed. To explain a ship's visible motion, combine this alignment with unit movement, model rendering and water/particle effects. The dependencies prevent a storage helper from being mistaken for the entire combat or naval system.
