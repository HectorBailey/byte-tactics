# Resource-account lifecycle and complete economy module contract

[Systems manual](../index.md) | [Completion tracker](../../doc-writeup-roadmap.md) | [Detailed economy chapter](../03-economy.md)

Module group: `game/economy`, twelve annotated entry points from `0x401000` through `0x401360`. Sources: [account methods](../../../src/game/economy.cpp), [production and settlement](../../../src/game/economy_401360.cpp), and [UnitResources layout](../../../src/game/unit_resources.h). The detailed chapter explains each accounting phase and gameplay examples; this companion closes the module's lifecycle, support-function and persistence coverage.

## 1. Owned state and interfaces

A UnitResources object contains two six-float blocks plus a player pointer, totaling 0x34 bytes. Each block stores produced, used, accepted demand, unpaid backlog, previous produced and previous used. Ordinary units embed one account and each player has an extra account for operations not represented by an ordinary live unit.

Reset (`0x401070`) zeros the complete object and installs a player pointer from the supplied index. It clears backlog and previous observations as well as current production/use, so invoking Reset on an active account is a real accounting reset. It does not refund old demand or serialize the owner's identity independently.

The names energyUsePaid/metalUsePaid in a shared declaration mean accepted demand operationally, not a guaranteed immediate debit. Player balances and account demand/backlog are separate state with different mutation rules.

## 2. Requests versus immediate spending

RequestEnergy (`0x401180`) adds attempted usage to its explicitly supplied account, then refuses when that account has positive energy backlog. Otherwise it adds accepted energy demand. Its extra account parameter matters: the body does not simply use whichever object pointer a misleading call-site spelling might suggest.

RequestEnergyAndMetal (`0x4011c0`) records both usage amounts, succeeds only with both backlogs nonpositive and then adds both demands. A failure can therefore affect reported usage without allowing the requested work. Neither request interface tests current player balances directly.

SpendEnergy (`0x401220`) and SpendMetal (`0x401260`) instead compare current player balances, debit immediately and record usage only on success. SpendEnergyAndMetal (`0x4012a0`) initially requires both balances, debits energy, then applies its metal branch. This is the immediate spending service used by costs such as shots or cloak; it does not create the same deferred demand as work requests.

Inputs are expected to represent valid nonnegative costs. These methods do not independently clamp arbitrary negative/nonfinite values, validate every player index or implement a global transaction ledger. Their differences are intentional observable contracts, not interchangeable naming styles.

## 3. Producer and cost eligibility

UpdatePlayerEconomy (`0x401360`) scans live unit accounts and resets/rebuilds capacities each pass. The building-associated active branch requests configured energy use and selects only the first special producer: extractor, converter output, wind or tidal. Extraction/conversion require the energy-request success; wind/tidal do not reuse that result as a qualification. Other active/moving classes contribute their ongoing energy requests through a different branch.

Completed units separately add ordinary energy/metal production and storage. This completion test does not automatically wrap every earlier special-production branch. Difficulty scaling applies to selected income helper calls for active type-2 ownership, with factors 0.5/0.7 at difficulty 0/1. It is not a universal multiplier applied to every initial balance or debit.

Cloak processing reads the request/runtime conditions and work deadline, chooses moving/stationary integer cost, attempts immediate energy spending and sets activation bit 4 according to success. The extra player account joins the same aggregate settlement, and enabled starting-storage bonuses augment capacity.

The [detailed chapter](../03-economy.md) gives exact branch ordering and dependencies. [Extraction/wind callbacks](units-unit-motion.md), [definition fields](units-unit-types.md) and [player scheduling](game-players.md) establish where the parameters and cadence come from.

## 4. Settlement and feedback

Production plus current balance supplies available funds for each resource. The updater funds aggregate old backlog first, fully when possible or with a proportional ratio. It then funds current accepted demand from the remainder with a second ratio. The residual becomes stored balance, capped to rebuilt capacity; excess increments lifetime waste.

The ratios apply across all live accounts and the extra player account, rather than selecting construction sites one by one in this allocator. SettleResourceAccount (`0x401320`) copies current produced/used into previous fields, retains the unpaid fraction of old backlog, adds the unpaid fraction of accepted demand and clears current produced/used/demand.

A positive resulting backlog blocks subsequent request acceptance even if an immediate spender can still use a positive balance. Conversely, a request accepted before aggregate settlement can later acquire unpaid backlog. This feedback explains intermittent construction progress without requiring a hidden arbitrary per-building priority in this function.

Income/use and lifetime counters reflect the function's aggregate fields. They are not guaranteed to equal a UI-smoothed per-second rate or every account's finally paid demand. [HUD presentation](../03-economy.md) and network economy reports observe that state on separate schedules.

## 5. Account persistence

SaveUnitAccounts (`0x4010b0`) selects `u%04xacc` using unit ID and writes two consecutive 0x18-byte blocks. That preserves backlog, accepted demand and previous-period observations in addition to current production/use. It does not write the trailing owner pointer.

LoadUnitAccounts (`0x401110`) restores those two blocks when the named box exists; a missing box leaves existing account state untouched. It does not check each ReadBox result or require a separately validated exact-size box in its body. Short reads can therefore leave a mixture of restored and preexisting fields, depending on bank read semantics.

The unit loader creates/resets the account first, establishing ownership, then overlays saved blocks. This distinction prevents restoring a raw pointer from another process while allowing economic backlog to resume. Player-level balances/statistics use the separate [player serializer](game-players.md).

## 6. Array lifetime support

The remaining annotated helper at `0x401000` is the compiler-generated vector constructor iterator. It accounts for repeated construction over array elements rather than resource production. CallOnEachReverse (`0x401030`) starts at base plus count times stride, walks backward and invokes a supplied member function once per element. It is generic reverse-order lifetime support, not a second economy allocation pass.

These entries explain the module's early address range without assigning revenue or scheduling behavior to compiler support artifacts. Module boundaries in this reconstruction can include such infrastructure alongside gameplay methods.

## 7. Cross-module responsibility and completeness

All twelve entry points are covered between this companion and the [economy chapter](../03-economy.md). Remaining cross-system investigations listed in that chapter, such as allied transfer distribution, full repair/capture/resurrection work formulas and HUD smoothing, belong to their own module groups. Their pending coverage does not mean this account allocator secretly implements those policies.

The module writes resource-account save boxes but emits no network packet directly. Unit/player lifecycle owns allocation/reset ordering; work and weapon handlers choose request versus spending interfaces; sharing supplies transfers; player scheduling invokes settlement; rendering/UI interprets balances/rates. Source-only tracing establishes these contracts without claiming a game run or byte-match test.
