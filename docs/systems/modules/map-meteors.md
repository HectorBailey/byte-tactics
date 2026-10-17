# Meteor environment events

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `map/meteors`. Implementation: [meteors.cpp](../../../src/map/meteors.cpp). Meteor showers are simulation events that create ordinary weapon projectiles. Their scheduling is distinct from the trajectory, collision and damage processing of those projectiles.

## Configuration and state

The configuration consists of weapon name, spawn radius, density, shower duration and interval between showers. Runtime state includes an enabled flag, an active-shower flag, selected weapon pointer, next shower time, current shower end time, next projectile time, and origin/target coordinates.

Defaults are read from the `Default` record of the meteor TDF under `gamedata`. The loader requires a weapon name and nonzero radius, density, duration and interval when it reaches its validation block. Invalid recognized default data invokes the fatal-error path. If the outer file-load or record-selection condition fails, the shown body does not take that validation failure branch; it should not be described as an unconditional fatal requirement for every invocation.

Parameter installation copies the weapon name and radius, converts density to a projectile interval by dividing 30 by density and truncating, and converts shower duration and between-shower interval from seconds to ticks by multiplying by 30 and truncating. The installer itself does not repeat the default loader's nonzero checks.

These conversions create edge cases for custom values. A sufficiently high density can truncate the spawn interval to zero; the update loop still has one emission branch per call, not an inner loop that emits infinitely many projectiles. Zero density passed directly to the installer is not guarded by this routine. Negative or otherwise unusual parameters require investigation at their source rather than an assumed sanitization layer.

Evidence: default loader `0x438320`; parameter installer `0x437d60`.

## Initialization and weapon validation

Initialization clears the active-shower flag and sets the next shower timestamp from the configured spawn interval. It resolves the configured weapon name. If lookup fails or the weapon lacks its meteor flag, the selected pointer falls back to the default weapon entry.

The routine therefore validates both existence and trajectory classification. The name alone does not turn an arbitrary weapon into a meteor. The default entry is a fallback pointer, not proof that the requested shower will retain the damage or artwork of the missing named weapon.

The empty shutdown hook present in this module performs no cleanup work in its own body. Its place in the larger teardown sequence should not be expanded into invented meteor destruction behavior.

Evidence: initialization `0x437cd0`; empty hook `0x437d30`.

## Starting a shower

Starting a shower marks it active, sets its end to current tick plus duration, sets the following shower time to that end plus the configured gap, and makes the first projectile eligible at the current tick.

The target is chosen from random terrain-cell coordinates within the map dimensions. The origin is offset from that target by separate randomized X and Z cell ranges. The shown offset expressions are asymmetric: X uses a 30-wide range minus 15, while Z uses a 10-wide range minus 15. They should not be rewritten as a symmetric circular origin distribution.

This setup exists as the explicit start entry point and an equivalent inlined start path in the update routine. A manual start changes active state and deadlines directly. The separate enabled flag is not tested by the explicit start routine itself.

Evidence: start `0x438070`; inline start in `0x437de0`.

## Per-tick scheduling

Each update first checks whether the next-shower timestamp has arrived. If so, it sets up a new shower and then clears its active flag if meteor generation is disabled. This means the deadline can advance even when the resulting automatically scheduled shower is suppressed.

Enable and disable entry points only set the enabled flag. Disabling does not immediately clear a shower already active in the shown setter. An existing active shower can therefore continue until its own end condition or a later scheduling transition changes it. This is different from deleting all meteor projectiles in flight.

While a shower is active, a due-projectile check sets the next projectile time to current tick plus spawn interval and creates one projectile. The shower-end comparison occurs after that emission branch. At a tick on which both a projectile and the end condition are due, the emission branch can run before the active flag is cleared.

All these deadlines are simulation ticks. Pausing or reducing effective game speed affects their wall-clock spacing through the main simulation scheduler.

Evidence: enable/disable `0x437d40`, `0x437d50`; update `0x437de0`; [simulation scheduling](../02-time-simulation.md).

## Projectile initial conditions

Horizontal velocity is the origin-to-target cell displacement converted to fixed-point world units and divided by 90. Vertical velocity starts at minus 15 world units per tick. Starting height is the negated vertical velocity times 90, placing the projectile high above the plane used for the calculation.

An additional horizontal offset uses a randomized radius below the configured spawn magnitude and a randomized angle. The offset is applied around the shower origin. The source does not establish a uniform-area disk distribution; choosing radius directly is not the same as choosing uniformly by disk area.

The resulting initial position and velocity are handed to `SpawnProjectile` with the selected meteor weapon. The weapon subsystem owns subsequent movement, model rotation, collision, explosion and damage. This module does not independently apply a second meteor-damage calculation.

The 90-tick construction is a nominal origin-to-target velocity relation. Random spawn offsets and weapon/update behavior mean it should not be advertised as a guarantee that every impact lands on one exact point after exactly 90 ticks.

## Saving and restoration

The `Meteor` HapiBank account stores enabled/active flags, next shower time, shower end, next projectile time, and origin/target coordinates under named integer items. Loading reads those values back with zero defaults when items are absent.

The serialized state preserves an in-progress shower's timing and location. It does not store the complete weapon definition, all configuration parameters, or a list of meteor projectiles in these items. Definitions/configuration and projectile persistence are handled elsewhere. Reinitializing after loading these timestamps could erase the restored schedule; callers must preserve the intended load order.

Evidence: save `0x438180`; load `0x438250`.

## Adjacent initialization artifact

The file also contains the compiler-generated initializer/destructor pair for a global vector-shaped mission-order table container. Its cleanup frees storage. Its presence does not make it meteor configuration or a collection of falling meteors. Documentation of a gathered translation unit must account for such support entries without assigning their data the wrong gameplay purpose.

Evidence: static lifetime entries `0x438450`, `0x438480`.

## Observable interactions and diagnostics

| Observation | Implementation explanation to check |
| --- | --- |
| Disabling meteors does not remove existing impacts | The setter changes enabled state, not active projectiles |
| A final meteor appears at shower end | Projectile eligibility is processed before the end check |
| A shower schedules but produces unexpected art/damage | Weapon lookup or meteor-flag validation may select the fallback |
| Density increases stop increasing spawn rate | Integer interval truncation and one emission branch per update |
| Restored shower starts in the wrong place | Origin/target items, base map dimensions and restore ordering |
| Showers have directional bias | Different X/Z origin offset ranges are present in setup |

This writeup covers configuration, initialization, enable/disable, explicit and automatic start, update, projectile handoff, persistence and the module's adjacent static-lifetime support. Exact stock-map frequencies and named-weapon damage remain asset-dependent.
