# Item Manager ownership editing — v1.1.1

The closed 20-ID catalog now grants absent supported items and removes actual native ownership. Six quantity stacks no longer require prior acquisition; quantity remains bounded by the current native capacity. The game persists these mutations through its ordinary Save flow. Quest/trade items, bottles, keys, beans, arbitrary IDs and raw save writes are excluded.

## Native mechanisms

The exact supported main remains Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`.

| Interface | Analysis address | Use |
|---|---|---|
| Pouch count | `805B63E0` | Read actual count / gear ownership |
| Pouch capacity | `805B5CF0` | Bound stack edits; gear has binary ownership |
| Native give | `8112D570` | One positive canonical-item delta, existing nine-argument ABI, zero optional flags |
| Native consume | `833C4FA0` | One positive removal delta, existing six-argument ABI, zero optional flags |
| Native equipped predicate | `80A91080` | Check removed equipment is no longer equipped before success |
| Native equipment cleanup | `833C6A40` | Called by native consumption when final equipped gear stock is removed |
| Link age | `804D6DB0` | Enforce the catalog's verified age rules |

Primary `PouchTable`, `PouchActorInfo` and `PouchContentParam` data from the pinned owner executable/container establish the canonical names, categories, age and stock behavior. All twenty runtime item metadata records use the ordinary `None` special-deal path; quest-sensitive special categories are outside this write catalog. No proprietary metadata, binaries, saves or private inspection clients are distributed.

Native final-stock consumption checks equipment at `833C5970`, dispatches native cleanup at `833C5E74`, and separately clears the active selectable item through the game's normal mechanism. The equipped predicate covers swords/shields/tunics/boots; it is not proof of every Material/Bow quickslot. Native runtime tests must establish visible/default equipment behavior separately.

## Mutation policy

- Show OWNED when real stock is positive; an empty stack is NOT OWNED even if its bag/tool remains. Removing arrows/bombs does not delete the Bow/Bomb Bag merely to hide the stock game's zero-ammunition icon.
- Grant gear from zero to one and remove from one to zero. Stacks support add, zero stock, bounded quantity and refill. No gameplay-acquisition evidence is required.
- Match current Link age. Refuse incompatible age edits and invalid/zero stack capacity with explanations.
- Hookshot/Longshot share a stock UI slot: refuse conflicting grants until the other variant is removed. Do not guess an upgrade/downgrade transaction.
- Require the Bow/Slingshot before adding arrows/seeds. Require zero arrows/seeds before removing the associated tool. Reductions/removal of existing orphan stock remain possible.
- Infinite Items must be OFF during editing. Its refill policy checks current Bow/Slingshot ownership every time, so cached evidence cannot revive orphan ammunition after tool deletion. Re-enabling Infinite Items intentionally refills otherwise eligible owned bags/tools.
- Freeze a complete snapshot for confirmation. Re-read target count/capacity/age and live dependencies before dispatch. Confirmation cancellation, held A and stale requests cannot write twice.
- Publish the temporary Save/travel hold before one native writer call. Poll count and, for removed gear, native equipped state. Revalidate context after getters, including the final equipped getter.
- Keep prior deferred-write, timeout, uncertainty and save-epoch behavior. Pending acknowledgement suspends new edits/refills/travel/autosave and cancels stock manual Save; stock Load remains available. Two seconds or ambiguous state ends the hold and requires normal save reload before further edits. Never retry or guess a rollback.

## Current verification

The warnings-as-errors ARM64 build and complete portable suite pass on module `5a88f46f2ad013c2881085d81c063a483fd51e00c37b44410c37233aac425443`. Tests cover all twenty ID grants/removals, both ages, shared-slot/tool dependencies, changed dependencies after confirmation, native equipment cleanup acknowledgement, world invalidation inside the final getter, deferred dispatch, timeout/uncertainty and save-epoch fences. Item-policy fixtures reproduce cached Bow/Slingshot evidence with zero/positive orphan stock and verify that refill remains refused until the tool exists again. Menu fixtures exercise add/remove/quantity confirmation and navigation; the renderer runs 936 state/scale/format cases.

The final module passed the full Goron ownership cycle in the stock UI: absent0, add1, Adult equip, Save; unsaved equipped removal0 restored normal armor, then stock Load restored1/equipped Goron. Saving the subsequent equipped removal0, adding1 unsaved and stock Loading restored0/normal armor. Native Mirror Shield/Iron Boots equipped removal and Megaton Hammer grants/removals also pass. All six stacks passed empty/add/refill across Adult and Young Link. Age/slot/tool restrictions, Infinite Items refusal/no orphan regrant, and ordinary restored-Bow firing30→29 pass. Exact observations are in [v1.1.1 native evidence](v1.1.1-native-results.json) and the current test report.

Only Goron Tunic has the full new stock disk-persistence cycle here; other IDs retain their stated native/portable scope. The preceding `f7a2a185…` checkpoint proved the initial Goron mechanism separately and is not substituted for final-module tests.

The v1.1.0 document below is retained as historical evidence for its own module and narrower read-only-gear behavior.

---

# Item Manager safety and verification

The Item Manager uses a closed catalog of 20 native item names. Fourteen weapon, shield and equipment entries report real pouch ownership and remain view only. Six previously acquired consumable/ammunition stacks support confirmed quantity changes, additions up to the unlocked capacity, removal to zero and refill. It cannot grant an unacquired weapon, shield, equipment upgrade or quest item.

## Verified interfaces and catalog

The exact supported executable is the project's pinned main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`. The analysis-address interfaces reused from v1.0.0 are:

| Interface | Address | Call shape |
|---|---|---|
| Native pouch count | `805B63E0` | `(pouch, &native_name, 0, 0) -> int` |
| Native capacity | `805B5CF0` | `(pouch, &native_name, 0, 0) -> int` |
| Native acquisition/increment | `8112D570` | `(pouch, &native_name, 0, positive_delta, 0, 0, 0, 0, 0)` |
| Native consumption | `833C4FA0` | `(pouch, &native_name, positive_delta, 0, 0, 0)` |
| Current Link age | `804D6DB0` | `(identified_link) -> child` |
| Persisted action-guide evidence | `80A96B10` | `(0, 36) -> Bombchu previously used` |

The native consumption routine checks the requested amount against the pouch count at `833C55F0`–`833C55F8` and queries capacity at `833C5654`. The manager always supplies a canonical single-item name and a bounded positive delta. Native count readback determines success; an apparent native return status alone does not.

The local supported-game `PouchTable` data verifies the exact weapon/shield/equipment names and age restrictions used in `inventory_catalog`. The six stack names, bag associations and age rules also match the established v1.0.0 resource policy. Local `PouchContentParam` inspection confirms stock/capacity associations for arrows, seeds, bombs, Bombchus, nuts and sticks. The private extraction/inspection script is retained outside the repository; proprietary game data is not included in the distribution.

| Category | Real ownership views | Editable quantities |
|---|---|---|
| Weapons | Kokiri Sword, Fairy Bow, Fairy Slingshot, Boomerang, Hookshot, Longshot, Megaton Hammer | None |
| Shields | Deku Shield, Hylian Shield, Mirror Shield | None |
| Equipment | Goron Tunic, Zora Tunic, Iron Boots, Hover Boots | None |
| Consumables | Deku Sticks, Deku Nuts | Previously acquired stacks |
| Ammunition | Arrows, Bombs, Deku Seeds, Bombchus | Previously acquired stacks |

Weapon/equipment acquisition and removal are deliberately excluded. The general native acquisition/removal APIs also contain equip and special-item handling; safe first acquisition, equipped-item removal and progression interactions have not been independently established for these entries. Master Sword, trade/quest items, bottles, beans, dungeon keys, maps, tokens and arbitrary names/flags are absent from the write allowlist.

## Mutation rules

- Read through the current, mapped pouch and identified Link, with supported world/readiness checks. Never access an arbitrary caller-supplied save address.
- Build a complete local snapshot, then publish only if the actor, pouch, world readiness and save epoch are still current. A failed read invalidates the output instead of publishing a partial inventory.
- Require positive stock, the corresponding already owned tool/bag, or verified persisted Bombchu-use evidence. Positive capacity alone never establishes acquisition. A Bomb Bag does not establish Bombchu ownership.
- Retain positive acquisition evidence only within the same valid actor, pouch, age and save epoch. Removing the final Bombchu can therefore be undone during that identified session; an unrelated save cannot inherit that evidence.
- Enforce the current Link age. Arrow edits require Adult Link; seeds/sticks require Young Link. Capacity comes from the native getter, must be between 1 and 999, and bounds every target quantity. Invalid counts disable the affected entry.
- Require a separate confirmation for every actual mutation. Re-read current count/capacity and age before applying; a changed snapshot requires review and confirmation again.
- Require Infinite Items OFF before editing. This prevents its ordinary refill from immediately undoing removal or a reduced quantity.
- Publish a pending-operation hold before performing one bounded native increment or consumption call. The game can queue the actual update for later frames; an unchanged immediate count is pending acknowledgement, not a failure.
- Poll the frozen item, actor, pouch, age, capacity and save epoch for up to two seconds. Report success only after the native count equals the confirmed target. Further inventory mutations are refused while acknowledgement is pending; the original native call is never repeated.
- While pending, suspend Infinite Items refills, disable new Fast Travel requests and hold native autosave requests. The manual Save confirmation follows Back/Cancel while the hold exists; the stock Load flow remains usable. A normal save reload advances the epoch and revokes the old pending operation.
- A timeout, unexpected intermediate quantity, changed capacity/age/actor, or unsafe context after dispatch clears the busy hold and makes inventory editing read only for that save epoch. The explanation asks the user to reload a save before further edits. Readback, menu reopening or a same-epoch reset cannot clear this uncertainty. This prevents a late queued update from stacking with another request.
- Release the temporary Save/Travel hold when acknowledgement succeeds or ends. A failure does not lock normal gameplay or saving indefinitely. Never retry automatically or speculate about rolling back native side effects.
- Never create a live-save backup, edit a raw save file, unlock a bag, write a quest flag or dispatch a get-item cutscene.

The native APIs update the game's ordinary inventory state. The final native check below demonstrates stock Save/Load persistence for an edited arrow stack on the exact integrated binary and a copied test save. The portable fixture alone does not establish disk persistence or native UI integration.

## Portable verification

`tests/inventory_test.c` passed a warnings-as-errors host build. Its mocked native interfaces exercise all 20 allowed IDs, six editable stacks, both ages, missing ownership, empty owned bags, live capacity upgrades, out-of-range quantities, read-only equipment, confirmation and Infinite Items refusals, duplicate/stale requests, pointer/world failures, epoch changes inside getters and writers, and Bombchu evidence isolation. Deferred-write fixtures retain the old count for twelve polls, then acknowledge the actual target without another write. They also verify the pre-dispatch busy hold, rejection of new requests, stale callers retaining an existing hold, the two-second timeout, unexpected partial updates, late completion after failure, same-epoch reset refusal and read-only uncertainty clearing only after a new save epoch.

These are production-policy fixtures. They do not claim emulator execution, persistence, controller acceptance or a successful release. Native evidence belongs in the integrated test report with its actual binary identity.

## Native regression evidence and provenance

The initial integrated module `7e81bc00…` reproduced a real acknowledgement defect: confirmed removal from 30 to 29 and addition from 29 to 30 reached the correct native quantities on subsequent frames, while the original immediate-readback logic incorrectly reported failure. Emptying and refilling the owned stack also changed real quantities. This is failure evidence for that earlier binary, not a successful test of its notices.

The fixed module SHA-256 `98ae67c95dacb1244dcfdbd50c2ab0ec548359dbcdbbbccd97826de6c742e989` subsequently passed native cancellation, deferred removal to 29, addition to 30, emptying to zero and refill to 30 with the verified-success notice. Native checks also exercised nuts, bombs and Bombchus; equipment remained read only, Adult Link's child-only ammunition was refused, and Infinite Items conflicts were refused. The private `inventory-fixed-tests.json` retains those measured quantities, notices and exact module identity.

Those earlier results retain their original module identities. The subsequent `557ef38bc4fa26cc8094a6b2e4c8ca8b379f00a31b6d27bd5290dee38490ffe9` integration checkpoint is superseded by the separately tested final module below. No earlier failure, notice or pass is promoted to final-binary coverage.

## Final native validation

The final module SHA-256 is `ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`. All **82,728 loaded RX bytes** matched SHA-256 `8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922` on Windows Lichii Build 177 and the pinned supported executable. Tests used ordinary controller navigation and the game's native item/Save/Load flows in an isolated profile with copied saves. The private result records retain the measured quantities and module identity; they are not distributed with game data or test saves.

| Final native check | Result |
|---|---|
| Cancel an arrow edit | Stock stayed at 5. |
| Confirm removal and addition | Removal reached 4; addition restored 5. Both completed with `Inventory change verified.` after native acknowledgement. |
| Empty an acquired arrow stack and refill | Emptying reached 0; refill restored the native capacity of 30. |
| Acquired nuts, bombs and Bombchus | Confirmed removal/refill checks returned to 20/20 nuts, 30/30 bombs and 30/30 Bombchus. Ownership and capacity remained valid. |
| Weapon/equipment mutation | Refused with the read-only gear explanation. |
| Adult Link editing child-only seeds | Refused with the age restriction explanation. |
| Infinite Items active during an edit | Refused with the instruction to disable Infinite Items. |
| Young Link's acquired seed stack | A confirmed edit changed 40 to 39; refill restored 40/40 with the verified-success notice. |

The corrected acknowledgement flow therefore has genuine final-module coverage for cancellation, bounded additions/removals, empty-stack refill, both ages and the tested safety refusals. An unchanged immediate read after a native request is handled as pending; the final success notice follows actual readback. The original immediate-readback failure remains historical evidence above.

### Stock Save/Load persistence and resets

On the same final module, an edited arrow count of **7** was saved through the stock Save flow. A later edit reduced the in-memory count to **3** without saving. Stock Load restored **7**, establishing persistence of the saved item edit and discarding the later unsaved change.

The normal load also reset the active cheat mask to zero, effective Movement Speed to 1× and effective Damage Multiplier to 1×. The preferred damage setting remained stored as 3× without reactivating it; Flight Speed remained a preference, and the Favorite count remained four. These are measured reset/preference results from that copied-save run, not a claim that arbitrary inventory states or every save slot were tested.

### Coverage limits

- The native mutation cases cover arrows, nuts, bombs, Bombchus and Young Link's seeds. Sticks retain portable policy/age/capacity coverage without a separate native mutation claim here.
- The stock persistence case covers the saved arrow stack. Separate disk persistence for every editable stack, equipment combination or progression state is not inferred.
- Weapons, shields and equipment remain view only. No native test authorizes granting an unowned item, removing equipped gear, editing quest state or writing arbitrary save memory.
- Timeout, unexpected partial-update and epoch/failure fences have production fixtures. These controlled fault cases are not presented as naturally reproduced native filesystem or engine failures.
