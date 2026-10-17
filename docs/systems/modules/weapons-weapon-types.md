# Weapon definitions, flags, asset binding and damage tables

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md)

Module group: `weapons/weapon_types`, three functions in [weapon_types.cpp](../../../src/weapons/weapon_types.cpp): LoadWeaponTypes `0x42e310`, LoadWeaponType `0x42e440` and FreeWeaponTypes `0x42f3a0`. [WeaponDef](../../../src/weapons/weapon_def.h) is the shared packed 0x115-byte record. This module defines interpretation of TDF inputs; live acquisition, aiming, firing, projectile motion and impact consume those values elsewhere.

## 1. Discovery, IDs and initialization

The game embeds 256 definition records. LoadWeaponTypes assigns each record its byte index and clears only the first byte of its name before enumerating Weapons/*.tdf. It builds a path through BuildDataPath, loads the TDF and accepts it when parser.flags is nonzero or FUN_0041d8a0 returns zero. That latter check belongs to archive/CD compatibility handling; this body alone does not justify calling it a weapon-validity test.

Each accepted file is visited by top-level section index, resetting current-record traversal before each selection. Every selected section calls LoadWeaponType. The ID field, default -1, indexes directly into the embedded array. There is no local 0..255 validation or automatic ID allocation. Missing/out-of-range IDs can index outside it. Duplicate IDs overwrite the same record; no duplicate diagnostic or prior-resource cleanup occurs here.

The section name becomes the internal 32-byte name through an unbounded strcpy. The human-readable name field goes into a 64-byte description with an empty default. Record zero is not specially skipped. The outer initializer is not a full memset of every model/table/flag field, so ordinary game allocation/teardown order supplies initial zero state for fields such as the damage-table pointer. Reloading arbitrary records in place is not an independently safe API.

## 2. Physical and range parameters

| TDF field | Stored interpretation/default |
| --- | --- |
| weaponvelocity | Integer input times 65536/30; default zero |
| startvelocity | Same conversion; default zero |
| weaponacceleration | Integer input times 65536/900; default zero |
| range | Signed integer, default 32767 |
| coverage | Signed integer, default zero |
| reloadtime | Unsigned-short ticks, seconds times 30 |
| energypershot, metalpershot | Floats, default zero |
| areaofeffect | Unsigned short, default zero |
| edgeeffectiveness | Float, default zero |
| weapontimer | Unsigned-short ticks, seconds times 30 |
| turnrate | Unsigned short, input divided by 30 |
| minbarrelangle | Float radians, degrees times pi/180; default -11.25 degrees |

Velocity conversion produces 16.16 world units per tick, and acceleration per tick squared. Range and coverage remain ordinary world-unit integer parameters; this loader does not convert them to fixed-point or derive one from the other. Range has no automatic cap to map size. Turnrate retains the engine angular-unit convention of its input, rather than this function converting degrees to radians as it does for minbarrelangle.

There is no broad positivity/finite-value clamp. Casts into short/byte fields constrain representation and can truncate; valid content supplies usable lifetimes, angles and divisors. A zero velocity is not repaired into a usable ballistic speed by this loader.

## 3. Burst, lifetime, effects and camera fields

Burst and sprayangle become unsigned shorts directly. Burstrate, duration, randomdecay, smokedelay, flighttime and holdtime multiply seconds by 30 into unsigned shorts, all defaulting to zero. Holdtime is the field later copied to camera hold/snap timing by blast code; it is not a generic reload delay.

Accuracy is signed short; tolerance and pitchtolerance are unsigned shorts. Firestarter, rendertype, color and color2 become bytes. Shakemagnitude is integer and shakeduration is integer seconds times 30. All default zero except minbarrelangle noted above.

Rendertype selects a later drawing style, while model binds a 3D projectile object. Explosion animation, smoke flags/rate, palette colors, sound and shake are independent inputs. Setting one does not automatically populate the others. In particular a smoke flag with zero delay is left for downstream effect scheduling to interpret.

Stockpiled production uses the +0xe4 timing field loaded from **reloadtime** as its build-progress total in [BuildWeapon](orders-unit-orders.md). A provisional buildTime name in that handler is not a separate weapon TDF buildtime parsed here. Unit buildtime and weapon reloadtime are different definition fields despite both being divisors in work code.

## 4. Complete loaded flag map

Every field below defaults to zero and is assigned to a one-bit member. The assignment uses the low bit of the parsed integer; a nonzero even value is not equivalent to normalized true.

| Bit | TDF field | Consumer concern |
| --- | --- | --- |
| 0 | lineofsight | Fire/trajectory handler selection and reach |
| 1 | ballistic | Ballistic trajectory family |
| 2 | shellweapon | Shell-style weapon behavior |
| 3 | beamweapon | Beam family |
| 4 | vlaunch | Vertical launch mode |
| 5 | meteor | Meteor special behavior |
| 6 | noradar | Weapon radar-related policy |
| 7 | paralyzer | Paralysis damage behavior |
| 8 | dropped | Dropped/bomb behavior |
| 9 | startsmoke | Start smoke effect |
| 10 | endsmoke | End smoke effect |
| 11 | soundtrigger | Sound timing policy |
| 12 | guidance | Guided flight mode |
| 13 | tracks | Tracking behavior |
| 14 | unitsonly | Target-kind restriction |
| 15 | groundbounce | Ground collision/bounce behavior |
| 16 | waterweapon | Water target/trajectory policy |
| 17 | toairweapon | Aircraft targeting policy |
| 18 | smoketrail | Trail effects |
| 19 | turret | Aim/weapon mounting behavior |
| 20 | selfprop | Propulsion behavior |
| 21 | propeller | Propeller-related projectile behavior |
| 22 | noexplode | Impact/explosion policy |
| 23 | burnblow | Expiry/detonation policy |
| 24 | twophase | Flight phase behavior |
| 25 | cruise | Cruise flight behavior |
| 26 | commandfire | Explicit-command firing policy |
| 27 | noautorange | Range-policy exception |
| 28 | stockpile | Stockpiled ammunition |
| 29 | targetable | Projectile can be interception target |
| 30 | interceptor | Interception weapon |

Bit 31 has no parsed field assignment in this loader. The table states flag provenance, not complete downstream conditions. Waterweapon does not alone prove every submerged target is visible or reachable; toairweapon does not alone establish every other target exclusion. Interceptor/targetable are separate sides of the interaction. Consumers combine flags with sensor, category, target geometry and weapon state.

SetWeaponFireHandler is called after the full record loads. It selects the executable fire callback from the completed flag combination in the weapon runtime module. A TDF flag's name is evidence of its input identity, while exact firing precedence must be read from that selector and its handlers.

## 5. Projectile model ownership and reuse

With a model field, LoadWeaponType scans numeric records from zero up to but excluding this weapon's byte index. A case-insensitive modelName match borrows that earlier record's model pointer and clears the new record's modelName. Otherwise it loads objects3d/<model>.3DO, reports FatalError on a null model, mirrors it, binds textures and records both pointer and ownership name. Missing model sets pointer zero.

The scan is by **lower numeric ID**, not earlier section/file load order. A previously loaded higher-ID model is not considered. Borrowing records have empty ownership names; the original owner retains the nonempty name. FreeWeaponTypes uses that convention to free a shared model once. A borrower can remain with a stale pointer after teardown because only the owning record is nulled on that path; live use must have ended before teardown.

Duplicate/repeated loads are not protected by per-record cleanup and can replace ownership fields or allocate duplicates. Model names copy into a 64-byte field without a local destination-length check after parsing into a larger temporary. These are content/lifecycle assumptions of the original loader.

## 6. Land, water and lava explosion art

The ordinary explosion pointer starts at null. Both explosiongaf and explosionart must exist before loading the named GAF and locating its entry. The loader writes zero to byte +2 of the found entry and stores it as explosionSeq. It does not check a null lookup before that byte write.

The second effect pointer, splash, starts null. A nonzero mission field at +0xd44 selects lavaexplosiongaf/lavaexplosionart; otherwise waterexplosiongaf/waterexplosionart. The same load/find/byte-reset sequence applies. It selects **one alternative pair at load time**, not separate simultaneous stored water and lava animations. The impact runtime later chooses ordinary versus this alternate effect according to collision context.

Missing either string leaves that effect null rather than substituting ordinary explosion art. Invalid GAF/entry resolution lacks an independent guard here. Visible water splashes, palette treatment, surface intersection and particles remain rendering/projectile responsibilities.

## 7. Sound binding

Soundstart, soundhit and soundwater each resolve through LoadSoundByName with a zero first parameter. Absent names store unsigned-short 0xffff. They bind start, ordinary impact and water impact independently. They do not select a sound category or spatial volume in this loader, and a missing water sound is not explicitly replaced by soundhit here.

Soundtrigger flag controls downstream timing policy; loading a start sound does not itself play it. Failure behavior of a present-but-unresolved sample comes from the sound loader's returned value, not an extra weapon-specific fallback.

## 8. Default and per-unit damage

A DAMAGE subrecord supplies default as an unsigned short, default zero. Without DAMAGE, default damage is zero. Other fields become signed integer override values keyed by their field names. The table is allocated lazily on the first nondefault entry and retained in the definition.

LowerBound uses a case-insensitive name ordering, but its equality decision uses case-sensitive StringRef comparison. An exact matching key updates the existing value; a differently cased equivalent key can insert another entry into the case-insensitive ordering. The loader does not normalize all keys to one case or resolve them to unit IDs here. Runtime damage lookup is a separate consumer and determines which matching entry is effective.

Only the default value is narrowed to 16 bits. Per-name entries remain int, so their representable range differs. These keys name units, rather than introducing an armor-class registry in this module. Damage application additionally depends on area falloff, unit state and paralysis policy in consumers; projectile interception is a separate interaction. The table itself only supplies base values.

## 9. Teardown and boundaries

FreeWeaponTypes scans all 256 records. A nonempty modelName frees the model with GameFreeThunk, zeros the pointer and clears the ownership name. A nonnull override table is deleted and nulled. It does not free GAF entries or sound samples here, clear every weapon name/flag, or serialize definitions. Those assets belong to their own managers; definitions are rebuilt from data at load time.

All three functions, every parsed numeric/flag field, model sharing, explosion alternatives, sounds and damage-table behavior are covered through static source inspection. Exact named-unit matchups require FBI weapon-slot bindings plus these TDFs, category rules, sensors and runtime checks. No source listings, builds or game execution were used.
