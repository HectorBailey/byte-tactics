# 03. Economy and resource accounting

[Manual index](index.md) | [Previous: time and simulation](02-time-simulation.md)

## 1. The account model

The economy maintains energy and metal balances at player level, but much of the work is tracked through per-unit resource accounts. Those accounts accumulate production, attempted use, accepted demand, unpaid backlog, and previous-period display values. A player also has an account outside the ordinary unit list.

The key distinction is between requesting work and immediately spending a balance. Construction and ongoing usage can contribute to account demand and later settlement. Weapon shots and cloak-related paths can debit the player's current balance directly. These interfaces have different success conditions and must not be treated as aliases.

Primary implementation: [resource helpers](../../src/game/economy.cpp), `0x401070` through `0x401320`; [player settlement](../../src/game/economy_401360.cpp), `0x401360`; [shared account layout](../../src/game/unit_resources.h).

## 2. Account state and naming

Each resource has six floating-point fields in the account:

| Concept used here | Shared-header name, energy example | Meaning established by the update |
| --- | --- | --- |
| Produced | `energyMake` | Newly accumulated resource income |
| Used | `energyUse` | Reported attempted/accounted usage |
| Demand | `energyUsePaid` | Accepted work demand awaiting the global settlement calculation |
| Backlog | `energyStall` | Unpaid accepted demand carried between settlements |
| Previous produced | `energyMakePrev` | Last settled period's produced value |
| Previous used | `energyUsePrev` | Last settled period's use value |

Metal has corresponding fields after the energy block. The account also holds an owner-player pointer. Reset clears the account and installs the selected player pointer.

The historical/header name “paid” should not be interpreted as proof that the balance was already debited at the instant a request helper sets it. The settlement routine explicitly apportions accepted demand and carries unpaid portions into backlog. This chapter uses “demand” to describe that operational role.

Different local struct views use different labels for the same bytes. The important evidence is how a field is changed and consumed, not whether one reconstructed member name sounds final.

Evidence: [account header](../../src/game/unit_resources.h); [local account view and settlement](../../src/game/economy_401360.cpp); [reset](../../src/game/economy.cpp), `0x401070`.

## 3. Request helpers

### 3.1 Energy-only requests

An energy request adds the amount to usage first. If the account already has positive energy backlog, it returns failure without adding accepted demand. Otherwise it adds the amount to demand and returns success.

This helper does not inspect the player's current energy balance. Its decision is based on existing account shortage state. The later player-wide calculation determines how much of the accepted work is funded and what backlog remains.

### 3.2 Coupled energy and metal requests

A paired request adds both quantities to their usage fields. It succeeds only when both existing backlogs are nonpositive. On success it adds both quantities to accepted demand. If either backlog is positive, neither accepted-demand field is incremented by that call.

This couples the progress decision: a metal shortage can stop work requiring both resources even when energy alone would have been acceptable, and vice versa. The reported usage still records the attempted request.

The request helper is not a reservation against a freshly computed global pool. Account feedback and phase ordering provide the control loop. [Chapter 02](02-time-simulation.md) establishes that unit actions occur before the player settlement phase.

Evidence: [request helpers](../../src/game/economy.cpp), `0x401180`, `0x4011c0`.

## 4. Immediate spending

The energy and metal spend helpers check the corresponding current player balance. On success they subtract the amount and record usage; otherwise they return failure without that debit.

The paired spend helper checks that both balances meet the requested costs before subtracting. It subtracts energy and records its use, then handles metal through the shown subsequent branch. These are sequential operations in the implementation, but the initial eligibility check requires both resources.

Thus a positive balance can allow an immediate shot even while a construction account retains backlog. Conversely, a construction request can be accepted from an account without backlog before later settlement discovers insufficient aggregate funds. There is no universal resource helper that all actions use.

This description assumes ordinary nonnegative costs. These helper bodies do not themselves establish comprehensive sanitization for pathological negative or nonfinite mod data.

Evidence: [spend helpers](../../src/game/economy.cpp), `0x401220`, `0x401260`, `0x4012a0`; [weapon spending](../../src/weapons/weapons.cpp), `0x49e1a0`.

## 5. Collecting production and ongoing costs

### 5.1 Eligible units and activation

The player economy pass resets total storage capacities and aggregate production/use/demand/backlog values, then enumerates live units. Different runtime classes take different ongoing energy-use branches. For the stationary/building-associated branch, activation controls the special production chain. Other qualifying units request their ongoing energy use when active or moving.

A complete unit whose remaining build fraction is exactly zero adds its ordinary `energymake` and `metalmake` income and its storage contributions. The explicit completion test is therefore important: an unfinished storage structure does not contribute that storage through this branch.

The special active-production chain and the completion-gated base-income/storage block are separate pieces of the function. Do not state that every possible economic action shares one blanket completion gate unless its actual caller/state prevents the other path.

### 5.2 Special production chain

For the qualifying active building branch, the updater first requests its configured energy use and then chooses the first applicable special producer:

| Priority | Condition | Contribution |
| --- | --- | --- |
| 1 | Positive extraction rating | Cached metal extraction, only if energy request succeeds |
| 2 | Nonzero metal-conversion output | Defined converted metal, only if energy request succeeds |
| 3 | Positive wind-generator rating | Wind fraction times rating |
| 4 | Positive tidal-generator rating | Map tidal strength times rating |

This is a mutually exclusive chain. Combining several special-producer fields in one definition does not make this path sum all of them. Ordinary base income is handled separately.

The wind and tidal branches do not test the earlier energy-request result before adding their contributions in the inspected chain. That is an implementation detail that matters for unusual combined definitions.

Evidence: [producer logic](../../src/game/economy_401360.cpp), `0x401360`.

## 6. Extractors, wind, and tidal generation

### 6.1 Extractor output

The extraction updater scans the unit's occupied footprint cells and adds each available cell's metal value plus one. It stores the sum in a fixed-point accumulator, multiplies by the unit's extraction rating, and caches the result on the unit. A script `SetSpeed` call receives the accumulated whole value when a script exists.

The plus-one term matters: a zero metal byte is not necessarily a zero contribution to this scan. Output depends on the footprint, covered cells, map data, and extraction rating. It is not solely a point sample beneath the model origin.

The player economy later adds the cached extraction amount when its active/energy-request conditions pass. Recomputing extraction geometry and crediting the cached value are separate operations.

Evidence: [extraction scan](../../src/units/unit_motion.cpp), `0x437840`.

### 6.2 Wind state

The environment updater schedules changes by game tick, selects a speed from configured bounds, selects direction when speed is nonzero, computes vector components, and derives a normalized wind fraction from speed divided by a denominator. Battle loading initializes that denominator to 5000. The normalized fraction is capped above at one in the shown routine.

The generator's economic contribution is that fraction times its rating, before any applicable income scaling. Its script receives direction and speed notifications when the environment's new-wind indicator is set. Intervening updates clear the indicator, not the persistent vector/strength.

Evidence: [wind selection](../../src/game/game_state.cpp), `0x490c40`; [initial denominator](../../src/game/game_state.cpp), `0x4917d0`; [script notifications](../../src/units/unit_motion.cpp), `0x437910`.

### 6.3 Tidal strength

Map loading sets tidal strength from a nonnegative mission setting or falls back to 0.5. The economy's active special tidal branch multiplies that strength by the unit's tidal-generator rating. This establishes the product used by the account update; converting arbitrary asset values into a player-facing per-second figure still requires their loaded units and simulation cadence.

This general economy trace resolves the base tidal-product calculation left open in the earlier naval investigation. It does not establish the exact stock output of a named generator without its FBI data.

Evidence: [map tidal initialization](../../src/map/line_of_sight.cpp), `0x483610`; [tidal contribution](../../src/game/economy_401360.cpp), `0x401360`.

## 7. Difficulty-dependent income scaling

The income helpers inspect whether the owner is active and type 2. For that branch, difficulty 0 credits 50% of the supplied income and difficulty 1 credits 70%; the default branch credits the full amount. Other owner types use the ordinary amount in these helpers.

These helpers apply to the producer contributions described above and selected other credits that call them. This is not evidence that every possible economy operation is globally multiplied by one difficulty factor. Direct initial balances, immediate spending, transfers, refunds, and other producers have to be checked at their own call sites.

The code uses several equivalent-looking float/double helper variants. Their compilation differences matter for matching, but the documentation concern is the actual branch and credited amount, not the history of how a match was obtained.

Evidence: [income helper variants](../../src/game/economy_401360.cpp); [construction refund conditions](../../src/ingame/control_panel.cpp), `0x41ba60`.

## 8. Player-wide settlement

### 8.1 Aggregation

The updater sums each live unit's two resource blocks, then includes the player's extra account. It can add separate starting/storage bonus fields when the player flag enables them. It records current income/usage and increments lifetime produced/consumed counters from the aggregate values.

Available funds are newly produced resources plus the current player balance. Energy and metal then go through analogous allocation calculations. No complete-unit-specific priority ordering is imposed by this aggregate stage; it computes ratios shared across the accounts.

### 8.2 Backlog is funded first

For each resource, existing backlog receives available funds before newly accepted demand. If funds cover backlog, its funded ratio is one. Otherwise its ratio is available funds divided by total backlog, and all available funds are consumed by that stage.

The remaining funds are then applied to current demand in the same manner: fully fund when possible, otherwise compute a proportional funded ratio. The unspent remainder becomes the player's new balance.

This establishes an important priority: previously unpaid accepted work precedes current accepted work. It does not establish an ordering favoring an individual factory over another factory within the same aggregate demand class.

### 8.3 Account feedback

Each unit and the extra player account receives the two funded ratios. Its old backlog is reduced by the funded portion, and its unfunded current demand is added. Production/use are copied into previous-period fields and cleared, and current demand is cleared.

In mathematical terms, next backlog equals old backlog times the unfunded backlog fraction, plus current demand times the unfunded demand fraction. This is the feedback used by subsequent request calls.

Repeated shortage can therefore make an account refuse new work until its carried unpaid amount is cleared, while its attempted usage still contributes to the reported load. Reading only the player balance misses that state.

Evidence: [allocation and feedback](../../src/game/economy_401360.cpp), `0x401360`; [settlement helper](../../src/game/economy.cpp), `0x401320`.

## 9. Storage and waste

Storage capacity is recomputed from eligible complete live units, plus enabled bonus fields. After backlog and demand allocation, a balance above capacity is clamped to capacity and the excess increments the player's wasted-resource counter.

Consequences follow directly:

- Finishing a storage unit can change the next recomputed capacity.
- Destroying capacity can cause funds to exceed the new cap at settlement.
- Excess production is not necessarily wasted if backlog or demand consumes it before the final clamp.
- A construction account can remain stalled even when the display later shows some stored resource, depending on which resource and when account feedback was sampled.

This routine does not establish a universal lower-bound clamp or protection against all pathological data values. Its ordinary accounting behavior should not be extended into uninspected validation guarantees.

Evidence: [capacity and waste handling](../../src/game/economy_401360.cpp), `0x401360`.

## 10. Construction, assistance, and repair

The common construction helper treats remaining build fraction as one for unbuilt and zero for complete. Positive work reduces the remaining fraction by work divided by build time, bounded to the valid range. The actual fraction change determines proportional energy and metal demand, and work advances only if the paired request accepts it.

Accepted work increases health by the corresponding integer contribution. Since the request helper uses account backlog rather than an immediate global balance check, the settlement feedback in this chapter is necessary to explain why later ticks may stall.

Several builders can call the common progress mechanism through their own orders/accounts. The exact assistance rate, approach requirements, experience modifiers, and order scheduling belong to the construction chapter. The accounting boundary established here is that each successful request contributes demand and progress through its calling account.

Negative construction work dismantles progress, adjusts health, credits metal through its refund path, and destroys a fully unbuilt target. Factory cancellation has an additional path computing invested-metal refund. Energy is not refunded by the inspected cancellation branch. Ordinary repair uses a separate helper and energy request rather than the full construction energy/metal fraction formula.

The construction renderer reads remaining fraction and tick, not the account's demand or balance directly. Appearance is therefore a consumer of progress, not the financial record itself.

Evidence: [construction and repair](../../src/ingame/control_panel.cpp), `0x41ba60`, `0x41bcd0`, `0x41bd10`; [factory cancellation](../../src/orders/unit_orders.cpp), `0x402640`; [construction display](../../src/graphics/model_render.cpp), `0x458dd0`.

## 11. Weapon costs and stockpiles

The common weapon update checks player energy/metal against per-shot costs for nonstockpile weapons, invokes the firing handler, and spends only after a successful fire result. A target reference or an aiming animation alone does not prove ammunition costs were charged.

Stockpile weapons instead require a positive built-round count and decrement it on successful fire. The weapon-building order accounts for producing ammunition separately. A factory building a unit that carries a stockpile weapon and that unit producing stockpiled rounds are two different construction/economy interactions.

Damage and experience affect other weapon properties, including reload calculation, but should not be silently treated as modifiers of per-shot resource cost unless their cost path uses them.

Evidence: [weapon update](../../src/weapons/weapons.cpp), `0x49e1a0`; [stockpile building](../../src/orders/unit_orders.cpp), `0x402b70`.

## 12. Cloaking as an immediate cost

The economy pass handles the cloak-request runtime flag for eligible players. If the unit is not blocked by the proximity/state flag and its work-time deadline has elapsed, it selects moving or stationary cloak cost from the movement-state bits, converts that selected cost to an integer, and attempts immediate energy spending. It sets or clears the cloak activation state from the result.

If the blocking/deadline conditions fail, or cloak request is not active, it clears that activation state through the shown branches. Thus requesting cloak, being permitted to cloak, and having energy to maintain it are separate facts.

The proximity detector and its delay are established in the sensor update, not in the spend helper. A complete cloak explanation must follow both systems. This chapter establishes the financial and activation boundary, including the integer conversion of the selected cost.

Evidence: [cloak cost branch](../../src/game/economy_401360.cpp), `0x401360`; [proximity state](../../src/ingame/info_panel_467440.cpp), `0x467440`.

## 13. Starting balances and sharing

Starting balances are initialized only when the relevant pending-save condition allows it. Campaign uses mission start-energy/start-metal arrays and a starting-storage-bonus helper. Skirmish uses configured slot values. Multiplayer uses host information multiplied by 100, with a host-slot fallback in the shown path.

These are initialization choices, not production. They should not be multiplied by the generator income scaling merely because the player is computer-controlled.

Sharing commands configure local thresholds and flags under multiplayer conditions. Metal/energy thresholds are capped above at current capacity in the inspected command handlers. Separate flags control mapping and radar sharing, so resource sharing and information sharing are not one identical operation. The actual transfer allocator must be documented from its own routine before claiming a specific ally distribution formula.

The main tick invokes resource sharing for the local player in the qualifying network/packet-manager path after the main world phases. This timing can matter relative to settlement, reports, and later shots.

Evidence: [starting balances](../../src/game/players_464290.cpp), `0x465e30`; [sharing commands](../../src/game/share_commands.cpp), `0x419340`, `0x419400`; [sharing scheduling](../../src/ingame/dialogs_493340.cpp), `0x495490`.

## 14. Persistence, network reports, and HUD values

Per-unit resource save records store both resource blocks, including accumulated and carried state, under a unit-ID-based box. Restoring a resource account is therefore more than reconstructing player balances and generator definitions. Removing backlog from a hypothetical restore would alter future request eligibility.

The player update sends multiplayer economy information on a separate reporting cadence. The HUD further smooths displayed amounts and updates displayed rates using its own deadline. Lifetime counters, current balance, previous account use, network reports, and screen bars are different observations of the system.

Evidence: [account save/load](../../src/game/economy.cpp), `0x4010b0`, `0x401110`; [economy reporting](../../src/game/players_464290.cpp), `0x464f80`; [HUD sampling](../../src/ingame/info_panel_468cf0.cpp), `0x468cf0`.

## 15. Worked shortage example

Suppose the aggregate energy backlog is 40, current accepted demand is 60, and available energy after adding production is 70. The backlog stage funds all 40, leaving 30. The current-demand stage funds half its 60. No energy remains for storage.

Every account's old backlog receives a funded ratio of one, and its current energy demand receives a funded ratio of one half. An account contributing 20 of current demand carries 10 forward as new energy backlog. Its next positive-energy request can fail until that backlog is cleared by settlement.

Metal is settled independently, but paired construction requests require both account backlogs to be nonpositive. This combination explains why one resource can gate work even when the other is plentiful.

The numbers are an explanatory example of the confirmed allocation formula, not stock unit costs or a runtime capture.

## 16. Diagnostic guide and remaining work

| Symptom | Inspect |
| --- | --- |
| Factory has resources visible but pauses | Both account backlogs, paired request result, HUD delay, tick phase |
| Usage exceeds actual accepted work | Usage added before backlog rejection, plus account aggregation |
| Generator animation changes but output seems stable | Notification cadence, persistent wind fraction, definition rating |
| Combined mod producer omits an output | Priority chain selecting extraction/conversion/wind/tidal |
| Storage disappears at destruction | Recomputed complete-unit capacity and final balance clamp |
| Shot does not consume expected resources | Target/aim/reload gates and successful firing-handler return |
| Cloak drops despite positive energy | Proximity/deadline gates, moving cost, integer conversion, spend timing |
| Save reload behaves differently financially | Per-unit account backlog and owner links, not balances alone |
| AI income differs by difficulty | Specific income helper and owner type, not a guessed global multiplier |

Still to establish in later work: exact allied transfer distribution and packets, all economy-producing call sites, complete initial-storage bonus semantics, every repair/reclaim/capture/resurrection formula, and the full HUD rate presentation. The core account, production-chain, allocation, and feedback mechanisms above are directly traced.
