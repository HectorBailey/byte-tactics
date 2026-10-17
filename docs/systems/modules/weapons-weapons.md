# Live weapons, projectiles, impacts and weapon rendering

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Weapon definition inputs](weapons-weapon-types.md)

Module group: `weapons/weapons`, 52 annotated entry points from `0x499a30` through `0x49e630`. Sources: [weapon runtime and renderer](../../../src/weapons/weapons.cpp), [projectile update](../../../src/weapons/weapons_49b720.cpp) and [area damage](../../../src/weapons/weapons_49a120.cpp). This module owns firing execution, projectile storage/flight/collision, weapon damage, replication and projectile drawing. Sensor acquisition and category exclusions also involve other modules; assigning an order target does not bypass them.

## 1. Definition binding and three live slots

FindWeaponByName (`0x49e5b0`) returns the first case-insensitive match across 256 definitions, rejecting null/empty input. The returned name begins at the record's address. It does not allocate another definition.

InitUnitWeaponSlots (`0x49e070`) resets all three reload timers and stockpiles, binds definition weapon pointers and encodes slot index in flags bits 2..3. A nonzero weapon definition index sets enabled bit 1; flag 0x10 is also installed. Zero is therefore the absent/default weapon index for this enable test even though the loader allocates a record for it. Other retained flags are not completely zeroed.

For each slot it queries muzzle and aim-from positions and caches `(muzzle Z - aim-from Z) * 1.25`. This value participates in ballistic launch correction. It finds the maximum reloadtime and invokes SetMaxReloadTime with integer milliseconds `maximum * 1000 / 30`. COB script geometry and return callbacks are covered in [script services](units-unit-scripts.md) and [COB interface](units-unit-script-calls.md).

SetWeaponFireHandler (`0x49e010`) has strict precedence: turret, then vlaunch, then lineofsight/selfprop, then dropped. No match leaves the existing callback untouched. Ballistic alone is not a final selector branch: ordinary ballistic weapons need a compatible selected family such as turret. This differs from projectile-update and network-receive precedence. Arbitrary contradictory TDF flags are not normalized into one canonical type.

## 2. Per-tick aiming and firing decision

UpdateUnitWeapons (`0x49e1a0`) visits enabled slots. It decrements a positive reload counter, resolves target position and clears aim-in-progress bit 0 when no position is available. A null fire callback skips the rest. Target resolution has already applied its own storage/information policy.

For a turret not already aiming, ballistic geometry uses aim-from position and SolveLaunchAngle; lineofsight uses CalcAimAngles. Other turret combinations fail to produce a valid aim in this branch. Success stores heading/pitch, clears the callback result and starts the selected AimPrimary/Secondary/Tertiary script with heading/pitch and a slot callback. It also sends the script call through the network helper and sets aim-in-progress. Non-turret vlaunch starts the same aim script with zero angles when ammunition policy allows and no aim is pending.

A nonzero reload timer prevents firing but does not prevent the preceding aim work. Once ready, WeaponCanReachPos checks current shooter/target geometry. Stockpile weapons require nonzero ammunition. Others first compare current owner energy/metal with per-shot costs. The selected callback receives shooter, slot, optional unit target and position. Failure leaves resources/ammunition unchanged in this loop.

Successful stockpile fire decrements ammunition and refreshes its focused build menu. Ordinary fire computes a new reload timer: experience tier is integer kills/5 capped at five; health term is integer health*20/maxHealth; timer is integer `(120 - healthTerm) * ((100 - 6 * tier) * reloadtime / 100) / 100`. Healthy inexperienced reload is approximately base time, damaged units reload more slowly and experience reduces it. Intermediate truncation matters. The result is stored through short representation.

The unit receives event 0x800 for commandfire or 0x400 otherwise. Nonstockpile shots then call SpendEnergyAndMetal. The returned spending result is not checked after the callback has already created the shot; the earlier balance comparison is the local precondition. Stockpile fire does not set the ordinary reload timer in that branch, relying on aim/firing/ammunition behavior for its cadence. Failed reach marks event 0x1000 through the unit event word. Zero maxHealth and malformed slot pointers are not comprehensively guarded here.

## 3. Position reach versus unit reach

WeaponCanReachPos (`0x49aa80`) checks squared horizontal world-unit distance against range squared. Waterweapon then succeeds without the remaining surface/ballistic tests. Other weapons reject a shooter whose position Y plus model-top whole height is at or below sea level; ballistic additionally requires a launch solution. This method does not independently test target unit type, toairweapon or target model-top depth because it receives positions.

WeaponCanReachUnit (`0x49abb0`) has more target-specific conditions:

| Weapon branch | Unit-target conditions |
| --- | --- |
| waterweapon | A target above sea is rejected unless floater bit 19 is set; a canhover target whose half model-top height plus Y exceeds sea is rejected; then horizontal range applies |
| other | Both shooter and target model tops must be strictly above sea; toairweapon requires target runtime mode 2; ballistic must have a solution; then horizontal range applies |

These are distinct tests from sonar/radar visibility, alliance/category acquisition and command dispatch. Waterweapon is not a simple universal underwater-only class, and toairweapon examines runtime mode rather than only target canfly. Range is horizontal; model height affects sea eligibility separately. GetWeaponRange (`0x49adf0`) retrieves the selected slot definition's integer range using the low byte of the index, without independent three-slot bounds checking.

## 4. Launch angle and aim tolerance

SolveLaunchAngle (`0x49a890`) solves the gravity/speed/displacement quadratic using horizontal hypotenuse, vertical difference and current map gravity. Negative discriminant returns sentinel 0x8000. It computes two candidate angles from acos of a speed ratio, substitutes pi/2 for nonpositive roots, then accepts the first candidate above minbarrelangle and at most pi/4, otherwise the second under the same limits. Neither a high-arc fallback above 45 degrees nor arbitrary zero-distance/speed validation is supplied. The chosen radians become engine angular units by multiplying 32768/pi.

CalcAimAngles (`0x49d910`) uses aim-from geometry, computes shooter-relative horizontal heading and pitch from whole vertical difference versus whole horizontal distance, and returns success. It does not itself cast a terrain-obstruction ray. AimWithinTolerance (`0x49d880`) compares wrapped signed-short heading/pitch errors inclusively. Explicit tolerance uses pitchtolerance when nonzero, otherwise the same tolerance for both. Zero tolerance means defaults 2000 when runtime moving mask 0x0c is present, otherwise 150. A default zero is therefore not exact-only alignment.

VectorLength (`0x49a850`) truncates Euclidean 3D length. SubtractVec3 (`0x49e570`) writes three component differences. These support geometry and do not select victims.

## 5. Four firing families

FireTurretWeapon (`0x49d580`) requires pending-aim bit and nonzero COB aim result. It recalculates ballistic or lineofsight angles and checks tolerance, clearing pending state when it cannot proceed. Failed ballistic/aim calculation additionally sets an out-of-reach event. It queries the actual muzzle, adds shooter heading and applies spread based on accuracy plus damage state: signed-short accuracy minus integer health*2048/maxHealth plus 2048. Integer kills/12 greater than one divides the unsigned spread. Nonzero spread independently jitters heading/pitch around half its range.

It selects lineofsight/selfprop spawning before ballistic spawning. Success clears COB result and pending bit and optionally broadcasts a fire packet. A legacy source comment alleges a machine-level argument-slot corruption in this function, but the current C++ body does not directly establish that allegation. This reference does not promote that comment to confirmed runtime behavior without independent instruction-level evidence.

FireLineOfSightWeapon (`0x49d9c0`) computes world heading/pitch from muzzle to target, checks them against shooter body heading/pitch using tolerance and spawns a straight projectile. Its packet origin uses the unit position even though local spawning uses muzzle position. That positional difference is literal behavior relevant to peer visual alignment.

FireVLaunchWeapon (`0x49db70`) requires nonzero aim result, queries muzzle and desired angles, and for interceptor weapons obtains an unclaimed targetable projectile. No candidate prevents firing. Successful vertical launch clears aim result and pending bit and sends target/interceptor information.

FireDroppedWeapon (`0x49dd60`) obtains muzzle, allocates a projectile, initializes heading from the unit, zeroes scalar speed and vertical velocity, and derives horizontal velocity from unit motion's +0x20 value through heading trig. Its provisional owner/id names do not denote player identity on that expression. It sends the ordinary fire packet on success. It does not solve a ballistic pitch before release.

FireWeaponByFlags (`0x49d0c0`) is a smaller spawn dispatcher, selecting lineofsight/selfprop first, then ballistic, otherwise failure, with its final argument order adapted to callees. The outer fire-family selector remains separate.

## 6. Projectile pool and initialization

AllocWeaponArray (`0x499a30`) allocates and zeroes 0x7d64 bytes: 300 records of 0x6b bytes, then resets count. FreeWeaponArray (`0x499a80`) frees and nulls the pointer. It does not reset count in that body. AllocProjectile (`0x49b6e0`) reserves the next record only while count is below 300, clears dead bit 1 and targetUnit, and returns null when full. It does not clear the entire reused record.

InitProjectile (`0x49c740`) installs definition, current/start position, optional aim point and spawn tick. It clears selected runtime bits, burst/activity count, phase and interception/target references and sets next smoke tick to current tick. With an owner it calls the projectile ownership interface, can make a factory-follow camera track this shot, queries the weapon piece by matching definition across three slots and sets owner workTime to tick plus 600. Without owner it uses player index 10 and null owner. It plays start sound immediately.

An owner with no matching slot can reach a piece query using index 3; ordinary definitions/callers must satisfy that relationship. Optional aim does not zero the stored aim when absent. Initializers/spawn families must establish each field they later consume.

CopyWeaponVelocityParam (`0x49c980`) selects explicit startvelocity when nonzero, otherwise maximum speed when acceleration is zero, otherwise zero. ComputeProjectileTime (`0x49c920`) uses current tick plus range*65536/speed when speed is nonzero and noautorange is clear; otherwise weapontimer. Thus configured lifetime and range-derived expiry are alternatives.

## 7. Spawn-specific state and effects

FireLineOfSightProjectile (`0x49c9c0`) allocates, initializes aim/owner and computes heading/pitch from muzzle to aim. It stores horizontal distance for burst timing, initializes scalar speed by the preceding rule, computes velocity, expiry, target reference and burst count. FireVLaunchProjectile (`0x49cc20`) instead starts heading zero, pitch 0x4000, velocity zero, scalar speed selected as above, target/interception references and burst count. It resets the slot aim result during spawning.

FireBallisticProjectile (`0x49cde0`) uses the slot's heading/pitch, speed and cached muzzle correction. Initial vertical velocity is trig pitch*speed minus integer `(muzzle correction / speed) * gravity`. Burnblow uses horizontal distance divided by horizontal velocity scale for expiry; other ballistic shots use weapontimer. Speed and scale divisions assume valid content. It stores target and burst count without supplying a new fixed aim to InitProjectile.

These three families start the selected Fire script and RockUnit recoil script, and request weapon smoke kind 9 when startsmoke is set. RockUnit (`0x499c10`) separately provides the same heading-relative trig recoil arguments at magnitude 800. Cosmetic recoil is a COB call, not physical knockback applied to shooter world velocity.

SpawnProjectileFromUnitMotion (`0x49d000`) supplies the dropped-style zero vertical/scalar-speed and heading-derived horizontal initialization. SpawnProjectile (`0x49df10`) is the ownerless explicit position/velocity entry used by meteors. It optionally broadcasts that position/velocity. The latter does not run an owning unit's Fire script or ordinary shot-cost check.

## 8. Burst emitters and finite capacity

UpdateProjectiles (`0x49b720`) snapshots count before walking records. Newly emitted burst children are not independently updated until a later call. A nonzero counter denotes an emitter/template, not a normally flying visible shot. When tick reaches last-emission plus burstrate it can refresh muzzle position, decrements counter, advances emission timestamp and attempts one child allocation.

Muzzle refresh occurs for burstrate at least five or odd remaining counter. A child copies the template, receives current spawn tick, zero counter, optional soundtrigger playback and a new expiry from lifetime or `(stored horizontal range + 16 world units) / scalar speed`. Randomdecay jitters that expiry around half its configured interval. Sprayangle modifies **template** horizontal velocity after copying the child, so that modification affects subsequent copied shots rather than the child already emitted. When count reaches zero the template is removed even if the final child could not allocate.

Only one emission occurs per emitter per update, even after a large tick jump. Bursts are not charged as separate ordinary UpdateUnitWeapons calls per emitted child in this module. Pool exhaustion can discard burst emissions; the template still advances/decrements. A zero speed with range-derived child lifetime has no local division guard.

## 9. Flight-family precedence and movement

For ordinary counter-zero records, propeller spin increments by 0x400 and flight selects the first matching family: selfprop, lineofsight, ballistic, dropped, meteor.

Selfprop before expiry accelerates scalar speed toward maximum, clamping the upper overshoot. A waterweapon at/above sea instead applies gravity and clears pitch before movement. Otherwise seeking is enabled by guidance, or by a nonzero phase for twophase. It turns toward GetProjectileAimPoint, detonating if turn fails, then rebuilds velocity from heading/pitch/speed. Expired burnblow detonates. Other expired selfprop applies gravity; a twophase first expiry installs flighttime, increments phase and clears unit/interception tracking if tracks is absent. It then moves and checks collision.

Lineofsight moves only before expiry, then marks dead without impact effects. Beamweapon initially keeps start fixed; after duration it sets runtime bit 0 and later advances start along with the head. This controls drawn beam length independently of damage area. Ballistic adds velocity and wind, subtracts gravity from vertical velocity and checks collision. With nonzero lifetime its expiry detonates for burnblow or emits white smoke/removes otherwise; zero lifetime keeps the ballistic flight path going until collision/removal elsewhere.

Dropped adds velocity and wind, applies gravity and checks collision without that ballistic lifetime branch. Meteor adds supplied velocity, tumbles roll/pitch from whole horizontal velocity and checks collision, without applying the same gravity update. Meteor scheduling is covered in [environment events](map-meteors.md).

## 10. Guidance aim points and turning

GetProjectileAimPoint (`0x49b3e0`) for cruise uses cached destination, not live target-unit tracking: at full 3D distance over 1024 world units it returns that destination with height whole part 700; nearer it sets height to max(ground, sea). At the far branch the whole-height assignment preserves the low fractional half. Noncruise first follows its intercepted projectile position, otherwise a live runtime-bit target unit position, otherwise fixed aim.

TurnUnitTowardsPoint (`0x49b520`) computes heading/pitch toward that aim and moves each by at most turnrate. Burnblow rejects a desired difference above 27000 angular units. Absolute error is stored through signed short, so the extreme half-turn representation is not an independently normalized wide integer. Heading can already change before a later pitch rejection. ComputeVelocityFromAngles (`0x49b680`) reconstructs vertical sine and horizontal cosine/trig components.

Guidance, tracks, twophase and cruise therefore serve different roles. A named guided missile does not necessarily follow a target's latest position through every phase.

## 11. Collision and water surface behavior

CheckProjectileCollision (`0x49b090`) marks a projectile dead outside map cells, also preserving camera hold position when tracked. An intercepted-projectile reference can detonate it when 3D distance is strictly less than areaOfEffect. It then records average terrain high/low height for rendering.

Primary occupancy can cause direct impact when owner player differs and projectile Y is below target model top. That path does not test target model minimum Y. Secondary occupancy instead requires Y within both model minimum and maximum. Player equality, rather than alliance, suppresses these direct occupancy collisions. Unitsonly returns after unit tests, bypassing feature, terrain and water-surface collision below them.

Feature collision resolves continuation cells, checks feature height above cell low bound and remembers last feature cell to suppress repeated same-cell triggers. The continuation lookup checks the resolved ID against sentinel range but omits the ordinary feature-count bound, a concrete source asymmetry. Impact occurs when the feature remains eligible.

Below terrain low height, groundbounce replaces vertical velocity with negative one quarter via signed shift and returns; it does not reposition the projectile above terrain there. Otherwise waterweapon can continue below the sea surface. Nonwater shots at/above sea continue; below sea they continue only with mission noSeaLevelTrigger, otherwise detonate. Terrain penetration still detonates water weapons when the earlier terrain branch selects it.

After movement, a surviving projectile emits smoketrail only before expiry and when next-smoke tick is strictly less than current tick, then advances that deadline by smokedelay. It emits once per update. Crossing from strictly above sea to at/below sea can independently add splash art if the map cell's high bound is below sea and surface triggers are enabled. That visual crossing effect is distinct from terminal water impact.

## 12. Detonation, ownership authority and direct damage

UntrackProjectile (`0x499e50`) copies tracked position to camera snap state for definition holdtime, clears tracking and sets dead bit. DetonateProjectile (`0x499eb0`) normally does that removal unless noexplode is set. Water classification uses the impact cell's high height below sea, not simply projectile Y.

With mission noSeaLevelTrigger, water classification and no direct unit hit, it removes and returns before effects/damage. Otherwise it accumulates configured screen shake. Water-without-unit uses soundwater and splash art. Other cases use soundhit and either endsmoke white smoke or ordinary explosion animation. A direct hit on a ship can consequently choose ordinary hit effects even over a water cell.

Damage authority is skipped for a projectile owner that is active controller type 3, while visual effects still occur. Other owners apply a direct unit hit when areaOfEffect is at most 16 and a direct target exists; otherwise they call area damage. Noexplode can leave a projectile alive after detonation/effects, so it is not synonymous with harmless visual-only ammunition.

ApplyWeaponDamage (`0x499cd0`) selects default or case-insensitive unit-name override, truncates scale multiplication, computes attack direction relative to target heading, then increases damage by six percent per integer five attacker kills, capped at five tiers (30 percent). Global flags 7/8 double/halve in that order. Paralyzer selects damage kind 2; ordinary selects 1. [Damage handling](units-unit-targets.md) owns health/paralysis/state reactions. The function returns computed damage rather than measured actual health lost.

ApplyWeaponHit (`0x499c70`) applies scale one and reports damage split to owner AI bookkeeping. Friendly classification here means same player index, not allied relation. Direct detonation has its own equivalent report. DetonateUnitWeapon (`0x49b000`) constructs a temporary projectile using explodeas or selfdestructas and invokes detonation, without allocating from the live projectile pool. ApplyAreaDamageAt (`0x49a0c0`) constructs a zeroed temporary wrapper with neutral owner index 10 and invokes area processing.

## 13. Area damage, falloff and interception

ApplyAreaDamage (`0x49a120`) uses **areaOfEffect divided by two** as its ordinary unit/feature radius. It scans a clamped terrain rectangle and both occupant IDs, skipping null and the attacking unit itself. Distance is 3D distance to the nearest point of each unit's axis-aligned model bounds, not distance to its center. Strict distance below radius qualifies.

At zero distance scale is one. Otherwise scale is `edgeeffectiveness + (1 - edgeeffectiveness) * (distance/radius - 1)^2`. It uses whole-world distance before that formula. Ordinary blast damage can hit other same-player units even though direct collision skipped them. Reported friendly/enemy totals again compare player indices.

Deduplication remembers at most 20 unit pointers and 64 feature-origin pointers. Once a list fills, new objects still qualify without being recorded; later occupied cells for those unrecorded objects can apply damage again. This capacity limits duplicate suppression, not the total number of targets damaged.

Unitsonly skips feature processing. Feature continuation resolves to origin, distance uses placed spot or computed feature center, and strict radius plus deduplication gates DamageFeature. The feature helper receives the weapon definition without the unit falloff scale, so this body does not apply the same scaled-damage argument to both object kinds.

For interceptor weapons it additionally scans all other nondead projectiles and detonates any within full areaOfEffect 3D radius, then sends removal packets for both. This area loop does not independently require targetable or hostile ownership. The earlier acquisition check is narrower. It assumes a usable attacker for packet owner access in that branch. Interception damage radius is therefore distinct from ordinary half-area blast radius.

## 14. Choosing an interception target

FindTargetableProjectile (`0x49d120`) requires nonzero slot stockpile, then returns the first projectile with a different player index and targetable definition inside an axis-aligned X/Z coverage square. It tests the projectile's stored aim at +0x28, not current position. It rejects candidates already referenced by any projectile's interception pointer. It does not separately check alliance, circular coverage, Y, dead flag or approach direction here.

This explains why coverage display or alliance expectations alone do not predict selection. Projectile destination within coverage and available stockpile are concrete gates, and one already assigned interceptor suppresses another claim in this scan.

## 15. Fire/removal replication

WeaponFirePacket is 36 bytes: type 0x0d, origin XYZ, aim/velocity XYZ, weapon definition byte, interceptor bit, heading/pitch, target ID, owner ID and slot byte. Most owner firing paths populate these fields, but unused upper bits of the interceptor byte remain uninitialized. SendWeaponFirePacket (`0x499ab0`) and ordinary firing wrappers share that layout. BroadcastWeaponFire (`0x499ba0`) and ownerless SpawnProjectile populate only the meteor-relevant prefix; remaining bytes are not initialized locally.

ApplyWeaponFirePacket (`0x49d270`) handles meteor first, directly initializing ownerless position/velocity. Other packets require owner ID and owner live bit, update received slot angles and resolve optional target. It does not independently bounds-check owner/target array indices or slot byte. It selects ballistic first, then vlaunch, then lineofsight/selfprop, then dropped. This differs from local callback and update precedence for contradictory flags. Spawn routines use the owner's bound slot definition, so consistent loaded assets are required.

FindRemoteProjectile (`0x49d1e0`) and its inlined receiver twin match interceptor requests by nonlocal player, exact stored aim coordinates and owner ID compared against the packet's target ID. It does not use a stable global projectile handle. ProjectileDetonatePacket is 14 bytes: type 0x0e, exact stored aim XYZ and weapon index. ApplyProjectileHitPacket (`0x49af90`) finds the first exact match and invokes detonation. Duplicate aim/type projectiles are not distinguished by an extra sequence number.

## 16. Compaction and owner cleanup

CompactProjectiles (`0x49ae20`) marks old indices, removes dead records and moves survivors down. Camera tracking follows a moved record. It collects interception references for moved survivors and resolves target old indices in a second pass. It does not collect the same fixups for survivors that stayed before the first hole, and a removed target has no replacement match. These limits can leave stale interception references in those cases; the routine is not a complete pointer graph relocation service.

RemoveUnitProjectiles (`0x49c880`) removes records with nonzero activity/burst field and matching owner, compacting inside its forward loop. Adjacent matching records can shift into the just-visited slot and be skipped as iteration increments. It does not universally delete every flying projectile ever fired by that unit. The normal update ends by compacting the pool after its snapshotted walk.

## 17. Projectile display and visibility

DrawProjectiles (`0x49be60`) draws counter-zero records through local information policy. Map flag 2 uses a projected explored-cell byte test; otherwise IsPointVisible decides. It projects screen Y as Z minus half height and uses viewport offsets X+128/Y+32. This is independent of collision-cell coordinates and target acquisition.

| Rendertype | Display |
| --- | --- |
| 0 | Line from start to current point; optional second palette color thickens/offsets the line |
| 1 | Ground shadow, model with heading/pitch offset by 0x8000, optional child while before expiry; propeller rotates child bank |
| 2 | Lens effect at projected point; outside-region result returns from the entire drawing function |
| 3 | Ground shadow and model using an uninitialized local angle record |
| 4 | Ground shadow and color-selected cannon/plasma/ultra-shell GAF, animated by elapsed tick modulo frame count |
| 5 | Blended flame-stream GAF frame from remaining lifetime fraction |
| 6 | Ground shadow and model using stored angles |
| 7 | Two jittered polyline passes along start-to-current vector, about five world units per segment |

Type 4 maps color selectors 0..4 to the fixed shell/plasma sequences; other values need not draw a projectile sprite. Type 5 divides by lifetime without a local zero guard. Type 7 jitters whole XYZ independently by -5..5 and draws the projected segments in one palette color. Shadows use terrain average height, rather than projectile altitude, for vertical placement.

The type-2 early return can suppress drawing later projectiles in the array. Type-3 uninitialized angles are an observed source limitation. The renderer checks counter and visibility, without a separate dead-bit test in this loop; ordinary update compaction normally removes dead records before display. Cosmetic smoke/splash/explosion allocation goes through effect modules, not these primitive model/line branches.

## 18. Coverage and remaining dependencies

The final two annotated static-lifetime entries (`0x49e610`, `0x49e630`) construct/destroy a global TdfFile parser; they are not extra projectile actions. All 52 entry points and major update/render branches are accounted for here. This module defines no independent projectile save writer in these bodies; persistence must be traced at its actual serializer rather than inferred from fire packets.

For naval interactions, distinguish sensor eligibility, unit-versus-position reach, model top relative to sea, waterweapon/canhover/floater tests, direct collision player filtering, blast friendly fire and visual splash policy. They are separate gates. This reference documents static source behavior and its explicit limits, without builds, game runs or unverified named-unit matchup claims.
