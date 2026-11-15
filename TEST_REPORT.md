# Test report — Deku Seeds and Magic Beans v1.1.3

15 November 2026. **Local native, host and cleanup acceptance PASS.** Magic Beans now appear in Consumables, and Deku Seeds support first acquisition through the game's required base-bag mechanism. Both retain confirmed ADD ITEM / REMOVE ITEM, ownership indicators and bounded quantity controls. The catalog has 21 item IDs and seven editable stacks. [Inventory contract](docs/v1.1.3-inventory-verification.md) records verified IDs/mechanisms; [native evidence](docs/v1.1.3-native-results.json) contains 48 validated observations with exact binary scope.

## Corrected behavior and binary identity

A first seed grant requires Young Link, zero seeds, raw capacity 0 and a separately owned Fairy Slingshot. Confirmation explains initialization of the related native 30-seed bag. Native Give must reach actual capacity 30 / stock 30 before a lower requested quantity is consumed down to its exact target. Save/travel remains held across both phases under the original two-second deadline. Existing 40/50 capacities remain on the ordinary quantity path; no Slingshot or unrelated equipment is granted automatically. Beans use native `Item_MagicBean`, maximum 10, without purchase prerequisites. Infinite Items excludes beans and must be OFF for manual edits.

Final module SHA-256 `b247a9ac86c52b3ae033c9367e55bff0d67d68a94437ca5d170cbe22b46f81ba`; loaded RX **83,672 bytes**, SHA-256 `61ac1ef8cca49a2984eb2d1babcf8c27dcde891a57e980088eb0ddfbef1d705e`. Corrected gameplay checkpoint `70aef1a94c5f9fc996acf638531ce541261919cc1b2ced3e802c9505467a70a1` has byte-identical RX and RW. Its read-only segment differs only in two footer characters, from V1.3.0 to the requested V1.1.3. Gameplay observations retain their actual module identity; this comparison is not presented as an implicit repeated gameplay run. The final module's loaded RX identity and visible V1.1.3 footer were separately verified.

## Completed native results

Tests used isolated Windows Lichii Build 177 with copied data and a fresh-game profile, pinned main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`, logical controller inputs, stock game flows and independent read-only state inspection.

| Scenario | Measured result and scope |
|---|---|
| Never-purchased beans | On `70aef1…`, purchase counter 0/all measured planted flags 0: beans ADD 0 → 1 → refill 10; progression unchanged. |
| First seed prerequisites | On `70aef1…`, no tool/no bag: addition refused. Separately adding the Slingshot grants ownership 1 without silently adding seeds. |
| Exact first seed grant | On `70aef1…`, confirmed native bag initialization gives exactly 1 seed / capacity 30, clears the pending-bag indicator, then ordinary refill reaches 30. |
| Native quantity persistence | On `70aef1…`, Save 30 seeds / 10 beans → unsaved 0 / 0 → normal Load 30 / 10 (epoch 4 → 6). The acquired base bag persists. |
| Normal Slingshot use | On `70aef1…`, stock-selected firing consumes seeds 30 → 29. |
| Virgin bean planting / Infinite Items | On `70aef1…`, Infinite Items ON: planting consumes beans 10 → 9; purchase counter stays 0 and only new Kokiri planted flag 8 becomes 1. Beans stay finite. |
| Saved removal persists | On `70aef1…`, Infinite Items OFF: remove beans 9 → 0 and seeds 30 → 0; Save 0 / 0 with the plant → unsaved re-add 1 / 1 → Load 0 / 0 (epoch 6 → 8). Plant and purchase counter 0 persist. |
| Existing 40-seed bag and both stack controls | On final `b247a9…`, both stacks support removal, cancelled addition, zero-stock ADD, native maximum, decrement and increment. Existing seed capacity 40 is retained without downgrade; bean maximum is 10. Purchase counter 4 and two previous planted locations stay unchanged through edits. |
| Adult, tool and Infinite Items safeguards | On final `b247a9…`, both Adult edits refused; seeds without the Slingshot refused; explicit separate tool grant permits seeds. Both edits refused with Infinite Items ON; eligible seeds refill and beans remain empty. |
| Edit isolation | The measured manual edits preserve every other catalog count and the recorded bean progression values. No raw save, purchase-counter or planted-location writes were introduced. |
| Final footer / runtime identity | Final `b247a9…` loaded RX matches the hash above; V1.1.3 footer is visible and was visually reviewed. |

## Automated checks and cleanup

Final v1.1.3 warnings-as-errors ARM64 compilation and the complete runner pass: **19 C fixtures**, **1,176 renderer cases**, installation/source ZIP checksum and layout checks, exact source/document bytes, NSO hashes and supported-profile validation. The frozen module matches `b247a9…` exactly. Finalized documentation/evidence is repackaged and checked before commit.

The isolated emulator is stopped. All **15 original copied save files**, **two preference files**, **two ExeFS files** and **three config files** match their pretest baseline after restoration. The owner's installation and saves were untouched.

**Local release GO within the documented scope.** CI, publication, exact tag and downloaded-asset byte verification are separate delivery steps after commit; they are not claimed complete in this source record.

## Coverage limits and earlier finding

Native seed capacities 30 and 40 were observed. Capacity 50 has verified native-parameter and host-policy coverage, without a separate native 50-capacity run. Progression measurements cover the listed purchase/dialogue/soil/planted values, not every progression flag. Kokiri planting does not establish every soil location or Adult bean-platform ride. Logical inputs were tested; physical controller hardware is not claimed. Existing cheats, travel and camera retain their historical native scope plus current host regressions; every encounter was not replayed.

Intermediate module `63f50870fa2626eb671afed695e3e9e0e8c61ac40ee8e5491dcd00099d57c030` exposed seed capacity 0 after a fresh native Slingshot grant. That finding led to the explicit base-bag path tested above. Its earlier observations remain separate checkpoint evidence and are not promoted to the final binary. Earlier release reports below retain their original identities and scope.

---

# Test report — Footer spacing patch v1.1.2

15 November 2026. The only runtime-source change from v1.1.1 commit `822ba40` raises the creator credit and version label from Y275 to Y272 and changes the footer version to V1.1.2. Inventory, equipment cleanup, save safety and all other gameplay implementations remain source-identical. ZIP creation now pins the creator platform to Unix, with an existing package-layout assertion, to remove the observed Windows/Linux metadata difference without changing v1.1.1 assets.

Module SHA-256 `35e338333455b091d9b1208bcd9c5a1d3b301db5163ade208e15d46b9fe1c942`; RX 82,728 bytes, SHA-256 `1034241d558b1e834129e5e5be04a775a8d1d992b5c9e4bca7611b6ffd0a7624`. Warnings-as-errors ARM64 compilation, the complete host runner (19 C fixtures), 936 renderer cases and exact installation/source archive validation pass for this new binary. The existing renderer covers buffer guards, all six scales/four formats and ownership/confirmation states; no new position-mirroring test was added for this visual adjustment.

No native ownership/Save/Load test was repeated for the footer-only binary. The v1.1.1 native results below remain evidence for module `5a88f46f2ad013c2881085d81c063a483fd51e00c37b44410c37233aac425443` with their original scope. They are not relabelled as v1.1.2 results.

Native footer acceptance passes on the new binary: the in-game capture was reviewed and both credit/version labels clear the lower inner border. [Footer evidence](docs/v1.1.2-footer-results.json) records the visual check separately from the retained v1.1.1 inventory gameplay evidence.

The isolated emulator was stopped after the footer smoke. All 15 copied saves, two preference files, two ExeFS files and three configs match the fresh baseline; owner data was untouched.

CI, publication and downloaded-asset verification remain separate delivery checks after commit.

---

# Test report — Item Manager ownership patch v1.1.1

15 November 2026. **Native ownership/persistence and portable regressions pass on the supported target.** Final frozen-source archive/CI/publication checks are separate delivery gates; their future results are not invented in this source snapshot.

## Exact tested binary and method

Module SHA-256 `5a88f46f2ad013c2881085d81c063a483fd51e00c37b44410c37233aac425443`; all **82,728 loaded RX bytes** matched SHA-256 `09346ed8625397d0856986ac42bc37aec05e54ce2990e99254a49338eff044ef`. Windows Lichii Build 177, title `0400486024DEC000`, main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`, displayed update 2.0.0. Testing used logical controller inputs, stock game inventory/equipment/Save/Load screens and independent read-only native state inspection in a separate profile with copied saves. Physical controller hardware is not claimed.

[Sanitized native evidence](docs/v1.1.1-native-results.json) preserves exact observations and source-record hashes without runtime addresses, private paths or serialized saves. Native screenshots are retained privately for review; the README's v1.1.0 captures remain historical.

## Final native results

| Scenario | Measured result |
|---|---|
| Absent Goron Tunic / cancel | Initial count 0. Cancelling addition leaves0. |
| Add / native equipment / Adult equip | Confirmed add reaches1; stock equipment screen displays the tunic and stock equip stores `Armor_Adult_Goron`. |
| Grant persists | Save equipped count 1; unsaved equipped removal reaches0 / `Armor_Adult_Normal`; stock Load restores1 / `Armor_Adult_Goron` (epoch 4→6). |
| Equipped removal persists | Remove equipped count 1→0 / `Armor_Adult_Normal`, Save; unsaved add reaches1; stock Load restores0 / normal tunic (epoch 6→8). Stock slot is empty. |
| Shields / boots / weapon | Previously absent Mirror Shield and Iron Boots0→1, stock equip, removal→0 with saved shield/boots references empty. Previously absent Megaton Hammer0→1→0. |
| Six stacks | Adult nuts/arrows/bombs/Bombchus each0→ADD1→refill 20/30/30/30. Actual Young Link sticks/seeds each0→ADD1→refill 20/40. |
| Age and slot restrictions | Adult seeds and Young Goron edits refused; Hookshot/Longshot conflict and Bow removal with17 arrows refused without mutation. Young travel settled to an actual child snapshot. |
| Brief existing-feature smoke | Cross-age travel to Young Kakariko completed in 67.656 s; both tab wrap directions retained selection/viewport. Free Camera translated 11.815995 while Link displacement was exactly zero; R3+B restored native eye within 7.168e-6. |
| Infinite Items / normal use | Edits refused while enabled. With Bow removed, Infinite Items left Bow 0/arrows 0 through 120 native frames. OFF, restored Bow/arrows 30; stock Bow selection and ordinary firing reduced arrows 30→29. |

After testing, the isolated emulator was stopped. All 15 copied save hashes, two preference files, two ExeFS files and three config files match the fresh pretest baseline; the owner installation/saves were untouched. Final loaded RX bytes matched again after the scenarios.

These results do not establish separate save persistence for every catalog ID, every quickslot or arbitrary progression/equipment combinations. The complete Goron Save/Load cycle is actual disk-persistence evidence, rather than a portable fixture or in-memory readback alone. Native general acquisition may perform its normal inventory bookkeeping; no direct story/quest flag writes were added.

## Automated regressions and package gates

The warnings-as-errors ARM64 build and the complete host runner pass:19 C fixtures (including storage's separate unavailable case), archive guards and **936 renderer cases** (39 states × 6 scales × 4 native formats). Inventory cases cover all 20 supported IDs from zero through removal, both ages, capacity bounds, live dependency changes, confirmation/held input, equipped acknowledgement, world invalidation inside its final getter, deferred native writes, timeout/uncertainty and save epochs. Infinite Items fixtures reject cached arrow/seed evidence after their tool is removed.

Original movement/landing/recovery/Noclip, Free Camera, combat, travel, resource, Save guard, preferences/migration and storage fixtures remain in the full runner. Their unchanged broad native gameplay results below retain their original module identity; they are not relabelled as new v1.1.1 runs.

Before this source snapshot is committed, rebuild installation/source ZIPs, run the complete runner against their maintained bytes, compare packaged NSO with this native-tested module, and reproduce both ZIPs plus both SHA sidecars byte-for-byte from clean extracted source. After commit, require successful branch CI before updating main, successful main publication, exact tag target and downloaded asset checksums. No future CI run or release URL is claimed here.

## Historical v1.1.0 and earlier evidence

The previous report below is preserved for its own versions/binaries and narrower read-only-gear behavior.

---

# Test report — Hyrule Cheat Menu v1.1.0

15 November 2026. **Local release decision: GO for v1.1.0 on the supported target.** Runtime, host, package and source-reproduction acceptance have passed. CI, publication and downloaded-asset verification are separate delivery checks after commit. The owner approved the three-tab design and authorized publication after successful release tests. This report separates final-module evidence, preceding native checkpoints and portable fixtures.

## Target and current module

- Windows Lichii **Build 177**, displayed game update **2.0.0**, title `0400486024DEC000`.
- Main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`; compressed main SHA-256 `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`. Compatibility follows these bytes and guarded interfaces, not every container sharing the version label.
- Current module SHA-256: **`ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`**.
- Running RX identity: **82,728 bytes**, SHA-256 **`8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922`**.
- Native QA uses the isolated emulator profile and copied test saves. The isolated emulator has been stopped; all 15 copied save hashes, isolated preferences and baseline ExeFS module were restored to their pretest state. The owner installation/saves were untouched. Game binaries/assets, keys, private debugger clients and owner save data are excluded from the distribution.
- Public sanitized evidence: [v1.1.0 native results](docs/v1.1.0-native-results.json). It omits actor/heap addresses, private paths and serialized saves and retains exact module identities and source-record checksums.

## Completed native checks on the current module

The private measurements retain their exact module hash. They are not promoted from an earlier executable.

| Check | Observed result on `ab2a5a1e…` |
|---|---|
| 1× / One Hit Kill OFF | A fresh Blue Tektite took ordinary Fairy Bow damage **3 → 1 HP**. Effective damage setting was 1× and the mod modification counter stayed **4 → 4**. Native assertion passed. |
| 1.5× melee | Master Sword accepted native damage **2 → 3** against an allowlisted Red Tektite with current life 2; checked attacker resolved to Link. Native assertion passed. |
| 2× projectile | Fairy Bow accepted native damage **2 → 4** against an allowlisted Red Tektite; checked attacker resolved to Link. Native assertion passed. |
| One Hit Kill priority | Fairy Bow's native **2** became **3** against a fresh Blue Tektite with life 3 while the stored multiplier remained 3×. The result used lethal current life, rather than the stored multiplier's 6. Native assertion passed. |
| Immediate multiplier resumption | Disabling One Hit Kill retained the selected 3× setting; the next accepted Fairy Bow hit against a fresh Blue Tektite changed **2 → 6**. Native assertion passed. |
| Received damage at 3× | With God Mode OFF and 3× outgoing damage selected, Link lost **40 → 38 HP**. Native damage **2** remained **2**, the exact Link target was refused as unsupported, and the modification counter stayed 4. |
| Inventory disk persistence | Stock Save recorded **7 arrows**; a later unsaved edit left 3; normal Load restored **7**. This verifies an actual native save/load roundtrip, not only a mocked count. |
| Save reset and preferences | The persistence flow advanced save epoch **4 → 6**. Active controls became **0**, effective normal speed and damage became **1×**, stored Flight Speed stayed 3×, stored damage selection stayed 3× and **four Favorites** remained. |
| Free Camera motion and restoration | Camera translated **28.968 units**, rotated and rose **7.936** / descended **7.736** units with all Link coordinates exactly unchanged. R3+B exit restored native matrix-derived eye within **3.55e-5**; menu-toggle OFF within **1.81e-5**; the main menu reopened. |
| Fly/Noclip/camera input handoff | Combined Fly+Noclip+Free Camera and continuously held-stick exit produced **zero Link displacement**. A continuous 50-frame exit/center/reapply-stick sequence while R3+B remained held also produced **exactly zero** displacement. After explicit neutral release, Fly resumed with **6.101067 units** of rise. |
| Camera Fast Travel shutdown | Camera deactivated and Adult Kakariko arrival completed in **6.360 seconds**, with controls OFF and Return stored. |
| Safe Fly OFF regression | God OFF: from **24.935349 units** above the starting ground, native descent measured **24.939110**, horizontal drift **0.040496**, health **40 → 40**, all active controls OFF and no recovery/Return notice. This is one measured natural landing, not all terrain/heights. |
| Inventory acknowledgement and refusals | Cancel retained **5 arrows**; confirmed removal reached **4**, addition **5**, emptying **0**, refill **30**, with verified acknowledgements. Nuts/bombs/Bombchus passed decrement/refill to their capacities **20/30/30**. Gear edits, Adult child-only ammunition and Infinite Items conflicts were refused. |
| Young inventory | In verified Young state, Deku Seeds changed **40 → 39 → 40**, respecting capacity 40, with the verified-success notice. |
| Cross-age travel and Return | Adult→Young Death Mountain Trail completed in **63.172 seconds** with actual child age and Return available. Return reached Adult Hyrule Field in **65.469 seconds**; after 120 settling frames the native inventory age was Adult. The independently selected Young destination list remained selected. Adult Kakariko follow-up completed in **14.625 seconds**. |
| Favorites and tab navigation | Favorites changed **4 → 5 → 4** and original IDs were restored exactly. From Item Manager, right tabs wrapped **Cheats→Travel→Items**, left **Travel→Cheats→Items**, preserving item cursor 14 and viewport. |


These results are recorded privately as `combat-final-15x.json`, `combat-final-2x.json`, `combat-final-ohk.json`, `combat-final-3x-resume.json`, `combat-final-1x-pass.json`, `combat-final-received.json` and `persistence-final.json`. Their sanitized results and final-module camera, inventory, travel, landing and UI records are included in the linked public evidence. Four genuine current-module 1920×1080 captures show [Cheats](docs/screenshots/v110-release-cheats.png), [Combat](docs/screenshots/v110-release-combat.png), [Fast Travel](docs/screenshots/v110-release-travel.png) and the real [Item Manager](docs/screenshots/v110-release-items.png).

The current native combat checks establish the tested melee/projectile path and accepted operands. They do not claim every allowlisted enemy, weapon, boss, reward or progression state was exercised. Bosses, minibosses, Link, friendly NPCs and unknown targets are excluded by the production allowlist and checked-source policy; negative host tests verify that policy. Their exclusion preserves native phases and caps rather than rewriting them. See [combat verification](docs/combat-verification.md).

## Reproduced defects and fixes

- **Combat cause mismatch:** native gameplay identified accepted player attacks using the checked cause path. The implementation and policy fixtures were corrected against the observed native dispatch; the current 1.5× melee, 2× projectile, One Hit Kill priority and 3× resumption results above pass. The final 1× control and God-OFF received-damage exclusion also pass.
- **Deferred inventory acknowledgement:** module `7e81bc00…` dispatched real quantity changes but incorrectly interpreted an unchanged immediate getter as failure. The fixed backend publishes a hold, dispatches once and polls for the actual target. Pending requests cannot stack; timeout/ambiguous results make that save epoch read only until normal reload. No retry or speculative rollback occurs.
- **Camera ownership after pause/idle:** earlier native checks exposed sleeping Link life jobs and native observation gaps. Active camera ownership now revalidates the same Link, save, scene and view without broadening resource/inventory ownership.
- **Camera-to-Fly input leakage:** an earlier combined Fly/Noclip/camera checkpoint moved Link about 3.439 units when a stick stayed held after emergency exit. The corrected integration drains native, Fly and normal-speed inputs through neutral release, including the R3+B combination.

## Preceding native checkpoint evidence

| Module scope | Completed native result |
|---|---|
| `98ae67c95dacb1244dcfdbd50c2ab0ec548359dbcdbbbccd97826de6c742e989` | Cancel left the original quantity untouched; verified delayed removal **30 → 29**, addition **29 → 30**, emptying to **0** and refill to **30**. Nuts, bombs and Bombchus were inspected/refilled; gear stayed read only, Adult Link's child-only ammunition was refused, and Infinite Items conflicts were refused. |
| `557ef38bc4fa26cc8094a6b2e4c8ca8b379f00a31b6d27bd5290dee38490ffe9` | Camera translation about **15.656 units**, rotation and both altitude triggers left all Link coordinates exactly unchanged. R3+B and menu-toggle OFF restored native view; combined Fly/Noclip/camera kept zero Link displacement. Held-stick exit leakage was absent. Fast Travel deactivated camera and reached Adult Kakariko in about **16.672 seconds** with Return stored. |
| `7ec57f4d212db1dac58de80b7dacd9492a5928db51d7540fd1774a8df28d32a2` | Explicit exit edge: hold R3+B with stick input, center the stick while continuing to hold the combination, then move the stick again before releasing. Link displacement was **exactly zero** through the held sequence. After neutral release, ordinary Fly control resumed and rose **3.092857 units**. |

Those results retain their own binary scope. Current-module retests are listed separately above; historical passes are not relabelled as new measurements. [Free Camera verification](docs/free-camera-verification.md) and [inventory verification](docs/inventory-verification.md) explain exact boundaries and failure handling.

## Automated and build checks

- Current warnings-as-errors ARM64 compilation passes against the pinned profile.
- **All 21 portable C fixtures pass**, including combat policy, Free Camera, inventory acknowledgement/uncertainty, preference migration, original movement/recovery/resource/Save guards and storage failure handling.
- The renderer passes **744 cases**: six scales, four native formats and 31 menu/inventory states, with buffer guards, exact 85% software coverage and cache invalidation for real item counts/capacities/status, drafts and confirmations.
- Executed ARM64 bridge fixtures verified preserved registers, complete vectors, stack/status state, the replayed camera instruction and the combat LSE atomic-add behavior. These are ABI checks, not gameplay tests.
- Preference fixtures use frozen schema-1/schema-2 bytes, all 214 legacy Favorites, all four stored damage settings, malformed/checksum-valid invalid payloads and rejection atomicity. Storage readback compares the damage selection as well as speeds, generation and Favorites.
- **The complete release build and `tests/run_tests.py --source-package` passed with exit 0**, including all host fixtures and install/source archive checks. Checks cover exact membership, duplicate/stale path rejection, canonical metadata, checksums and maintained source/document bytes.
- The independent archive verifier confirmed that packaged `subsdk8` is byte-identical to the native-tested `ab2a5a1e…` module. A clean extraction of the source archive rebuilt both ZIPs and both SHA-256 sidecars **byte-for-byte identically**. Final document-only refreshes are revalidated before publication; archive checksums live in their sidecars, avoiding a self-referential checksum inside an archive.

## Local release GO and delivery verification

All required local runtime, host, install/source package and clean-source reproduction gates have passed in the stated scope. No unresolved release-blocking failure is known in that scope. The approved design, genuine current-module captures and sanitized native evidence are included in the distribution.

The final document-only archive refresh is checked against this same native-tested module. After commit/push, verify successful CI, exact v1.1.0 tag/commit target, public release assets and downloaded SHA-256 sidecars. This source report does not invent those later external outcomes or a release URL.

Crashes, save corruption, serious camera/controller faults, mandatory-feature failures or major v1.0.0 regressions block publication. Native boss-phase/NPC testing, a separately recorded save load with Free Camera already ON, additional maps/cutscenes, physical controllers, displays, progression/equipment states and long-session combinations remain disclosed follow-up unless an actual serious defect is found. Normal-save reload is required after a savestate because reliable savestate notifications are unsupported. Safe Fly OFF/recovery limitations remain documented in [README.md](README.md).

## Historical v1.0.0 evidence

The collapsed material below preserves previous checks and release decisions exclusively for their named v1.0.0 binaries. It is not a v1.1.0 GO decision or new test coverage.

<details>
<summary>Preserved v1.0.0 reports and earlier RC evidence</summary>

# Test report — v1.0.0 Full Release

15 November 2026. **Release decision: GO for practical v1.0.0 publication.** Supported target: Windows Lichii Build 177, displayed update 2.0.0. The UI displays **V1.0.0**, without EXP labels or experimental consent for the supported features. Runtime signature and availability checks remain active. The owner reports extended play without observed bugs; no duration or exhaustive scenario list is inferred.

## Release binary and evidence

- Title `0400486024DEC000`, main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`; compressed main SHA-256 `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`. This executable is byte-identical to the earlier private 1.1.1-profile executable. Version labels alone do not establish compatibility.
- Final module SHA-256 **`d2a061f7b31071e7f536f64f5d3d1885ef2ffd9ed0130d422de850ce935d0772`**. All **60,296 running RX bytes** matched SHA-256 `6391427b75dd307eae5e1dae55c276af6b5cbf8ed8aaa002bc2ea3ca232af5a3`.
- [Fly/landing evidence](docs/fly-landing-verification.json) distinguishes final-module checks, preceding iterations and the private native fall control. Earlier six-speed, 100-cycle, standalone and destination evidence retains its original provenance below; unchanged long tests were not repeated.
- Tests used a separate portable profile and copied saves. Private executables, debugger clients and save data are excluded from both archives.

## Reproduced defects and fixes

1. **Ordinary Fly OFF rewind:** the earlier `f95dfb15…` build reloaded its safe anchor after a roughly nine-unit rise, taking 5.344 seconds. Fly now releases its velocity override at the current ordinary position. Native gravity/collision perform the descent. A one-shot actor/save-scoped guard skips only positive cause-4 damage at the verified native fall callsite. Other damage and actors pass through unchanged; landing expires the unused token and pause time does not consume a suspended descent.
2. **Failed extreme recovery:** `8ca0396d…` interpreted local recovery queueing as success, releasing Fly when the native event manager refused it. A 100-unit drop reduced life 40→20. Recovery now retains zero hover until native completion; refusal/timeout restores visible Fly/menu control. The final binary repeated that refusal with life 40 unchanged and zero positional drift.
3. **Safe identification gap:** a fixture reproduced lost recovery hover during an actorless poll while the world remained ready. The final fix preserves the existing zero publication during that narrow gap; actual loading still invalidates the old handle. The regression failed before and passed after the fix.
4. **Noclip unavailable after refusal:** native testing on `3752860c…` found that queued Fly recovery cleared OFF Noclip's checked anchor before a real world change. After refusal below a floor, Noclip could not be re-enabled. The final fix retains that anchor only during same-save, same-scene Flight recovery while the world remains ready; its movement publication is revoked. Actual loading/scene/save changes still clear it. The exact fixture failed before and passed after the fix.

## Targeted native results

| Check | Observed scope |
|---|---|
| Native fall control | Private passthrough build, roughly 12 units, God OFF: life 40→38 and positive native fall dispatch logged. This control is not shipped. |
| Ordinary landing | Final module: roughly 12 units, God OFF, natural descent at the current horizontal position, life 40→40, no travel transaction, landing token cleared. |
| Additional heights | `bb161c99…`: roughly 75 units God OFF and 100 units God ON landed naturally with life 40. `8ca0396d…` additionally checked roughly 25 units. These precede the final recovery-gap and OFF Noclip anchor fixes. |
| Refused extreme recovery | Final module: roughly 100 units God OFF, Fly/menu restored after refusal, zero hover drift and life 40. No automatic teleport success is claimed. |
| Startup/settings | Final module: Kakariko save start all toggles OFF, normal 1×, Return unavailable; stored Favorites and Flight Speed 3× retained. R3 and both tabs worked. |
| Fast Travel | `3752860c…`: original Adult Kakariko entrance completed in 7.672 seconds; final-module roundtrip is recorded below. |
| Combined movement | Final module: tested floor bypass y=97.408104→91.513260, Noclip OFF nearby acknowledgement 1 without fallback. Fly OFF refused safely; Noclip re-enabled, ascended and both controls turned OFF above the floor, life 40. |
| Repeated toggles/rejected travel | Six natural OFF/landing cycles passed, life 40, controls OFF and token cleared. Airborne travel was rejected while retaining the guard; subsequent landing kept life 40. |
| Cross-age travel/Return | Adult source → Kakariko Child in 44.985 s; Return in 65.203 s restored all source coordinates exactly, controls OFF. |

The engine can refuse recovery at extreme or clipped positions. Fly then stays visibly ON; descend to safe ground. Below a floor, enable Noclip with Fly, ascend into clear space, restore Noclip collision, then disable Fly. Automatic anchor recovery is not guaranteed in every native state. The landing guard does not enable God Mode or suppress enemy/fire/other damage. Production-hook fixtures verify other causes, actors and the original passthrough ABI.

## Automated checks and packaging

All production host fixtures, warnings-as-errors ARM64 compilation and exact install/source archive checks passed after the final code changes. This includes reproduced recovery refusal, actor-identification gap and OFF Noclip anchor regressions, fall-only native ABI passthrough, save/scene fences, storage faults, all speed factors, menu/Favorites and rendering. Installation has **26 files**, source **88 maintained files**. Final archives passed exact membership/checksum validation. A clean extraction of the source archive rebuilt both ZIPs byte-for-byte, including the native-tested module; SHA-256 sidecars are supplied.

On preceding `3752860c…`, below-floor Fly Save confirmation left all **15 copied .sav hashes unchanged**. Actual native Load restored Kakariko in **6.516 s**, controls OFF, normal 1×, Return cleared and landing token zero; preferences remained. The final additional change preserves only OFF Noclip anchors during pending Flight recovery, with negative save/scene/loading fixtures. All 214 original records and 37 groups remain intact. Two unstable river exits stay visibly blocked; trainer admission uses existing progression. The supplied MIT notice is retained; no project-wide license was added.

## Practical release scope

Additional geometry, water, equipment, damage/upgrades, progression, boss/minigame/audio, physical-controller, display/aspect-ratio and long-soak combinations are P2 follow-up unless an actual serious defect is reproduced. Savestate notification is unsupported; load a normal save after restoring a savestate. Owner acceptance, earlier scoped evidence and current targeted verification determine the practical release decision.

GitHub publication is authorized for the existing public repository. The authenticated account has admin/write rights. Issues were disabled and verified through the public API. Existing history, visibility and older tags/releases are preserved. This report is committed before publication; final CI/release/download status is checked externally against committed source and archive checksums.

---

# Preserved Build 177 checkpoint report (`f95dfb15…`)

The following checkpoint's DEV/EXP labels, release gates and unpublished status are historical. The later owner acceptance and practical criteria above supersede its exhaustive-coverage prerequisites. Measurements remain scoped to the original binary.

# Test report — v1.0.0 development / owner test

15 November 2026. **NO-GO for final v1.0.0 publication.** The local owner-test build retains gameplay **EXP** labels and displays **V1.0.0 DEV**. The installable archive's final-version filename does not certify completion. Current results below apply to this exact module and target; the complete earlier RC5 report is preserved separately below.

## Current target and binary identity

- **Emulator:** Windows Lichii Build 177, SCM `87d84d513c67275ad0d6ae50ba51cce76e7e3ded`.
- **Owner's displayed update:** 2.0.0. The title1 compressed main executable is byte-identical to the pinned executable used for the earlier 1.1.1-profile work: SHA-256 `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`; title `0400486024DEC000`, Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`. No hook-address port was needed for this exact executable. This does not establish compatibility with every container labelled 2.0.0.
- **Current module SHA-256:** `f95dfb15faeaf42e2567d5f94161b1cb1a462b00d7f8df5dd1aa6a7f4edecfdd`.
- **Running-code verification:** all **57,672 RX bytes** matched text SHA-256 `6d8a1479a543a0ab66bfd1510e7b655b941a3174ac0b21686f2fa6cf8a7bcddc`. The final identity and scoped runtime results are included in [docs/build177-verification.json](docs/build177-verification.json). The earlier private `running-identity.json` records a different development module and is not substituted for it.

## Current automated verification

The complete host fixtures, warnings-as-errors ARM64 compile/link, installable-package checks and source-archive checks passed. Coverage includes menu/controller state, all 214 pinned destination records, region labels, Favorites/preferences, storage faults, the software renderer, ammunition/wallet policies, recovery, loading gates, save guards, and movement/resource epoch fences. ZIP members, duplicate/unexpected paths, checksums, source/document bytes and NSO segment hashes are checked. The actual private native travel-request constructor also passed the guarded boundary fixture. These checks exercise code and file contracts; they do not simulate real collision geometry or replace gameplay acceptance. GitHub-hosted CI has not run.

The newly fixed defects and their evidence are distinguished:

- **Reproduced storage defect:** a checksum-valid readback containing different settings/Favorites but the expected generation was accepted. New fixtures failed against the old implementation, then passed after checking the complete pending payload and invalidating the rejected inactive slot. The previous valid generation remains loadable. Integration review also corrected the caller to keep `preferences_dirty` set after failed writes, so a later menu close retries.
- **Reproduced epoch defects:** old Fly/Noclip scopes remained usable between the native save hook and the next controller poll; a reset inside a native getter could also republish an old actor under the new epoch. Targeted fixtures reproduced these failures and passed with explicit caller epochs and consumer checks. Items refilled after a reset inside a capacity getter before its new guard; the corrected fixture rejects that write, stale callers and old-save ownership evidence. Ammunition protection rechecks its epoch after native eligibility calls.
- **Additional resource regression checks:** the actual production helpers now verify God refill bounds/no resurrection, unlocked magic and native call arguments, all four normal-speed factors across ground/swim/climb, temporary climb-request restoration, and resets inside resource getters/healing. Magic/Speed publications carry epochs; save reset invalidates jobs before clearing owners.
- **Audit-driven travel hardening:** an additional epoch recheck between movement updates and `travel_update` prevents continuing with the earlier world-readiness snapshot if a native movement getter begins a reload. This closes a code-review race window; no separate emulator reproduction of that exact interleaving is claimed.

## Current real emulator checks

The final module executed through logical Player 1 inputs and read-only position/state inspection. Private diagnostics and fixtures are not shipped. `battery-results.json` records a completed run on the module identified above.

| Check | Observed result | Limit |
|---|---|---|
| Start from loaded save | All cheat toggles OFF, effective Movement Speed 1×, Return unavailable; preferred Flight Speed remained 10× | One loaded-save start; not the full repeated Load/Game Over matrix |
| Menu pause | Link's exact position remained unchanged for six seconds; the native pause masks were active | Does not independently establish every enemy, projectile, boss, minigame timer or audio path |
| Six Flight Speeds | Every setting ascended, descended and stopped; all idle/stop coordinate deltas were zero | Short selected-scene samples; not a streaming-boundary or real-time throughput benchmark |
| Fly + Noclip | Descended from y=96.605927 to y=85.943001 through the tested floor; coordinates stayed exactly fixed for 90 presented idle frames | Selected Kakariko geometry, not broad ceiling/water/equipment coverage |
| Noclip OFF with Fly retained | Matching request/completed/nearby acknowledgement; no fallback; Fly and four core toggles stayed selected | One checked nearby restoration |
| Fly OFF | Native recovery returned to the last safe ground position in **5.297 seconds** | One recovery; not a new long stress run |
| Combined state | Four core toggles and normal 3× remained selected after movement/recovery | Selection preservation is not complete independent damage, consumption or progression verification |
| Cross-age Fast Travel | Adult source → `KakarikoVillage_Child` completed in **57.844 seconds**, preserving four core toggles and normal 3×; Return location became available | One arrival; Fly/Noclip were already OFF, so this does not repeat active-movement shutdown during travel |
| Walking Noclip OFF | Matching request/completed/nearby acknowledgement, no fallback; four core toggles retained | Confirms the tested safe shutdown, not successful traversal of the obstructing wall |
| Return | Completed in **54.359 seconds** and restored all three source coordinates exactly: `(-119.782829, 96.615799, 58.692837)`; four core toggles and normal 3× retained | One recorded roundtrip |

Vertical deltas from the six-setting sweep are in world units. Each ascent/descent input lasted 18 presented frames; host pacing varied. Idle used 60 presents and stopped input 30. Native contact introduced small horizontal corrections during some descents.

| Flight Speed | Ascent Δy | Descent Δy | Idle / stopped drift |
|---|---:|---:|---|
| 1× | 2.391617 | -2.180313 | Zero / zero |
| 1.5× | 3.386765 | -3.281616 | Zero / zero |
| 2× | 4.519424 | -4.781883 | Zero / zero |
| 3× | 6.777687 | -6.832939 | Zero / zero |
| 5× | 11.699989 | -11.700905 | Zero / zero |
| 10× | 25.687935 | -25.695633 | Zero / zero |

The completed `travel-wall-results.json` records the cross-age arrival, walking-Noclip shutdown and exact Return above. Its attempted wall passage oscillated around the obstacle and the stationary pose returned to a last clear point. The script's final-40-frame progress comparison (1.269 units with Noclip versus 0.296 native) passed its numerical assertion, but **does not prove a complete wall traversal**. Conservative geometry/floor restrictions still require broader investigation and acceptance. Historical destination arrivals, pause tests, FPS-mod controls and recovery stress retain their earlier module/build qualifications below.

## Additional current native results

- **Independent supported barrier:** a fixed world-direction control stopped at the ranch fence near z=231.897 while ordinary collision was active. With Noclip ON and Fly OFF, the repeated shorter trial reached z=257.138 on the inside; height stayed at y=150.595703. Noclip OFF received matching nearby acknowledgement 105 and restored ordinary collision at the same horizontal point, with a small native vertical settling correction. A longer sweep crossed the same barrier and later lost verified floor support beyond the field; the conservative guard restored the last safe source. This proves the selected fence and guard behavior, not arbitrary walls or streaming safety. The earlier camera-relative fence attempt is not used as a traversal proof.
- **100 acknowledged recovery cycles:** 50 grounded and 50 with Fly passed in **261.64 seconds**, without fallback/reload or stuck acknowledgement. Four core toggles and normal 3x survived. Eleven four-second present samples ranged **30.36–32.26 FPS**. Whole-emulator private memory started at 11,994,066,944 bytes (also the sampled peak) and ended at 11,537,047,552; working set fell from 16,296,935,424 to 14,423,126,016. No sustained rise was observed in this short run. These are whole-process totals, not mod-only memory, a long-session leak proof or a controlled mod-free performance comparison.
- **Favorites:** Y added, removed and re-added the ranch destination while preserving the exact selected row/cursor and viewport. Closing the menu wrote verified preference generation 193, retaining all existing Favorites. Fresh-process acceptance is recorded separately when exercised.

- **Unsafe Save / actual Load:** with all four core cheats, normal 3x, Fly and Noclip active, Link descended from y=150.595505 to y=144.521698 below the ranch floor. One native Save confirmation followed Cancel; all 15 copied `.sav` hashes stayed unchanged. Native Load then restored the Kakariko manual save in 9.078 seconds: all toggles OFF, effective normal speed 1x, Return cleared and preferred Flight Speed 2x/Favorites retained. The first confirmation selected the native default Cancel and was not counted as a reload; the steady dialog was observed before selecting Load explicitly. Complete running RX bytes matched again after the native run.
- **Hardware scope:** all four XInput slots returned disconnected (1167) in the read-only current check. This does not enumerate every SDL/non-XInput device or establish physical-controller acceptance.

- **Fresh-process standalone GUI:** with the private local debug server disabled (no listener on port 6551), the ordinary Build 177 GUI loaded the copied Market save. Temporary private keyboard mappings to logical Player1 opened R3, showed all visible core toggles OFF/normal1x and stored Flight Speed 2x, switched tabs with RB, expanded Favorites with A and displayed both the earlier Kakariko Favorite and newly stored ranch Favorite. R3 closed the menu and resumed gameplay. Native window capture was used; no debug input client ran. Temporary keyboard latches were restored after closing. This is a logical-input acceptance check, not physical-controller testing.
- **Built-in mods:** the current startup log records Max Resolution and FPS Unlock applied; Dynamic FPS by Dark was skipped as included in the applied built-in patch. The new native checks establish selected coexistence in that configuration. An FPS-mods-OFF control and other mod combinations were not repeated on Build 177.

## Final local packaging

The final installable archive contains **22 files**; the separate source archive contains **81 maintained files**. Full host checks and exact install/source archive validation passed. A clean extraction of the source ZIP rebuilt **both ZIPs byte-for-byte**, including the native-tested `f95dfb15…` module. SHA-256 sidecars are supplied beside both archives. No GitHub runner or remote/public release was invoked. The private emulator was closed and its temporary keyboard mappings restored after shutdown.

## Remaining release gates

Mandatory broader checks remain: independent grounded Noclip and combined Fly/Noclip across varied geometry, moving/unsafe surfaces, water, ages and equipment; all movement modes/speeds and collision restoration; progression-sensitive and final-build destination arrivals/Return; every damage/upgrade/consumption case; repeated save/Game Over reset; complete boss/projectile/minigame/audio pause; physical controller/focus behavior; 1080p/1440p/4K and aspect ratios; Build 177 FPS-unlocker controls; and controlled long-session/performance comparisons. Savestate notification remains unsupported. Final owner gameplay acceptance, the Discord URL, repository Issues configuration and explicit publication approval are also outstanding. Nothing was published publicly.

---

# Preserved historical RC5 report

The following original report covers 12–13 November 2026 and older development revisions. Its “current” wording refers to those historical checkpoints, not the owner-test build above. Earlier results are not relabelled as Build 177 tests.

# Test report — v1.0.0-rc5

12–13 November 2026. Tests use an isolated portable Lichii Build 130 and disposable copied saves. The owner installation, saves and original Fast Travel module were not modified. Results below distinguish real execution, native fixtures and static validation. **NO-GO for final v1.0.0; gameplay EXP labels remain.**

## Current reviewed integration — 13 November

Module SHA-256: `262fae4c1e7926ee1e04b9266c7238edfdf539527e5057c90edd9a356c970686`. All **55,592 running RX bytes** match text SHA-256 `49ed97e438b342661dfb81ee3d88cb6faafd1ee2d25561740fb829558caf0e87`. Boot/native Load starts all toggles OFF, effective normal 1x and Return cleared. Flight Speed remains an inactive preference.

- **Shared movement:** all four walking and A-held ground settings passed with Noclip ON. The ordinary ground hook leaves the bypass input unscaled, and the bypass applies the shared normal factor once; native water/climb paths retain their separate handling. This fixes an observed earlier build holding A-movement near the same speed at every setting. Current A-held mean speeds were approximately 6.3 / 10.2 / 12.9 / 19.6; endpoint native requests were the unscaled 6.37. Startup, camera and sampling effects remain in those means.
- **Flight/travel:** all six native flight speeds (4 / 6 / 8 / 12 / 20 / 40), exact hover, airborne Noclip activation and Noclip OFF with Fly retained passed. Clipped-source cross-age travel completed in 50.67 s, preserving four core toggles and normal 3x while clearing Fly/Noclip. Return restored (-83.951859, 150.595398, 227.372528), within 0.00002 of the safe source.
- **Anchor defect and fix:** 32927e7f… safely cancelled travel because an ascent contact flag lag let Noclip capture y=152.027100 while the actual pre-flight ground was y=150.595398. Worker acknowledgement was already complete; the target was airborne. Capture is now suspended while the other movement cheat or its recovery owns the body. Separate production-code fixtures reproduce both capture directions. Scene-change fixtures also clear old movement selection/publications before the new Link is identified. a5e70156… and the current build passed the formerly failing travel/Return sequence.
- **Recovery stress:** the current build passed 100 acknowledged cycles (50 walking / 50 flying) in **282.50 s**, without fallback/reload or stuck recovery. Eleven four-second whole-emulator samples measured 25.17–30.62 FPS and p99 40.48–68.80 ms in that ranch scene. Private memory fell from 12.858 to 12.780 GB; working set from 14.895 to 14.369 GB. CPU used 971.70 s over 277.77 s between samples, averaging 3.50 logical cores on a 16-thread host. This short run showed no sustained growth; it does not establish a long-session leak result, isolated mod overhead or input latency.
- **FPS compatibility:** the same module passed Noclip walking/A-held 1x and 3x controls and all six flight values with both built-in FPS mods disabled. Read-only inspection confirmed the original instruction `0x1e201828` in place of the enabled FPS branch. Movement uses native game time. With the FPS mods disabled, host-time displacement slowed with the game; these results do not claim to correct emulator/game slowdown. Ordinary 1x/3x walk and A-held controls retain native velocity/acceleration; one short 3x A-held trial did not match a constant-velocity distance assumption and is excluded from that claim.
- **Current performance sample:** same process/pose, three four-second windows each: 45.27–48.54 FPS all OFF, 47.16–48.61 with four core toggles ON, and 46.97–48.63 menu open. The enabled/disabled comparison showed no clear new cost in these short samples. It is not a comparison against an unmodified game.
- **Trainer:** the current module repeated the controlled native quest/card fixture arrival in 26.64 s, then reopened R3 after 180 presents in `GerudoFortress_Adult` without capture. The CSV retains this hash and fixture qualification; natural full quest completion is not claimed.
- **Standalone visible UI:** the same installed module loaded in the ordinary Lichii GUI with the debug server disabled; port 6551 refused connections. Using private keyboard mappings to logical controller inputs, R3 opened the menu on a copied child Potion Shop save. Switching focus to Explorer and back preserved the open menu and selection. Disconnecting Player 1 through Lichii's controller settings left it open; the native controller applet then reconnected Player 1, after which held D-pad navigation and R3 close worked. This is a logical controller/focus roundtrip, not a physical-device or background-physics measurement. Temporary test input latches were removed by restoring the exact private configuration backup; owner settings were untouched.

All expanded host fixtures and package checks pass, including a clean LF checkout. Default and private-executable builds reproduce the current tested module exactly. The refreshed local distribution retains the RC5 filename and EXP labels; broader acceptance gates below still block final v1.0.0. Historical results retain their own hashes and scope.

## Post-freeze release development

The original frozen RC5 remains in its verified 255-file rollback snapshot. The local distribution has been refreshed with the reproduced current integration; later development results are identified separately below. Private diagnostic fixtures never ship.

**Earlier integration — `87fb500cdac02079c9ac92d32f89db337ed3f2f9a31cb344e5af2c7187aceb77`:** all 55,176 running RX bytes match SHA-256 `ee619832c0da9233ae39eec4c020447a1eea829f7532ac60380cf95d92a8a600`. It boots all resource toggles OFF, effective normal 1x and Return cleared; stored Flight Speed remains a preference. Full source/build/package fixtures pass, including new safe identification-gap cases. ZIP checksums are now generated automatically and checked for exact content/name.

A real integration regression found that a safe post-pause identification gap cleared Noclip's inactive ground anchor, making Noclip unavailable after ascending with Fly. The fix retains only the same-scene anchor through that gap; actual unsafe/save transitions still clear it. The rebuilt module passed all six flight settings with native horizontal velocities 4 / 6 / 8 / 12 / 20 / 40, exact stationary hover and all four core toggles plus normal 3x retained. Noclip enabled while already airborne; below-ground hover stayed exact. Noclip OFF kept Fly enabled and received matching request/completed/nearby acknowledgement with no fallback. From that source, cross-age travel completed in 50.67 s, clearing Fly/Noclip while preserving the four core toggles and normal 3x. Return restored the captured Adult ground position within 0.00002 units.

**Recovery stress:** 100 real acknowledged ON/OFF cycles (50 walking, 50 with Fly) passed in 273.72 s, with no fallback/reload or stuck recovery. Core toggles and normal 3x survived; Fly disabled afterward. Eleven four-second samples measured 29.05–31.44 FPS and p99 36.52–46.27 ms in that ranch scene. Whole-emulator private memory began at 12.391 GB, peaked at 12.641 GB and ended at 12.292 GB; working set fell from 15.152 to 13.697 GB. CPU consumption was 952.70 s over 268.84 s between samples (3.54 logical cores on this 16-thread host). This short run showed no sustained growth; it does not establish mod-only overhead, a long-session leak result or input latency. Compilation briefly overlapped earlier samples.

The production trainer repeat on the same controlled native progression save completed in 24.61 s; after another 180 presents R3 opened in `GerudoFortress_Adult` without capture. The destination CSV records this exact integration hash. No natural full quest completion is claimed.

An initial higher-speed village flight measurement hit native tree/roof collision with Noclip OFF. That partial sweep is excluded from speed/FPS claims; safe ground recovery succeeded and the clear-air repeat passed all six settings. The current integration section records the later FPS-add-on OFF controls.


- **Wallet/event fix — `b47709d203bc9d1c858b716ecedb1a97f942671e715b0ba5223f1e3d19101ba6`:** the frozen module allowed a 50-rupee purchase to display 450 until dialogue ended. A safe identified-player binding now permits wallet refill during ordinary events and invalidates on object/loading/save changes. The fixed build held 500 through a completed fairy purchase; OFF charged a 30-rupee potion to 470. A real native Save at 470, enable to 500, then native Load restored 470, all toggles OFF, normal 1x and Return unavailable, including after 180 presents. Releasing/rebuying a bottled fairy confirmed the bottle exclusion. The observed shared-scene destination is labelled Potion shop without changing its registry bytes.
- **Natural sticks and rentals:** on the recorded RC5 build a lit torch depleted 20→19→18 OFF; ON held 20 for 32.91 seconds. On the wallet-fixed build, all ten bowling rentals expired normally (HUD 10→9→8→4→0), regular Bombchu stock stayed 30, and ownership-guide flags stayed 0. The native failure/replay flow remained functional. R3 held Link's transform during the round and suppressed ZR; closing resumed play. These supersede the earlier pending burn/rental observations for those cases only.
- **Native geometry:** independently traced full-player-shape overlap and downward ray calls passed clear-air, self-body, floor, below-floor and doorway-wall controls with intact owned scratch guards. A bounding-sphere alternative was rejected because it falsely intersected the floor. Production queries borrow the verified player shape without destroying it and check object identity, world/loading readiness and save generation before authorizing recovery.
- **Nearby recovery — `22d7f1f1babe43b12b9ee8be8d3851ea881a1c3583e53c065e7a939dd623adfb`:** from inside the Kakariko arch wall at (-45.409893, 109.939507, -15.013204), OFF relocated about 28 cm to (-45.409893, 110.138725, -15.212422). Core toggles and Fly remained enabled, hover held for 120 presents, and the movement job acknowledged nearby recovery without a reload. A separate clear-floor walking control passed; it is not labelled an inside-wall test.
- **Walking support — `0b522711d122ba54d8dc3a1541781625ca07daa1185a81eba60a49bb17022bfd`:** with Fly OFF, walking Noclip traversed the arch side wall while native floor contact was hidden. Actual downward hits maintained y≈109.97–110.20. OFF found a checked nearby grounded position without scene reload. This supersedes the older reload-only implementation, which remains the fallback when no point is verified.
- **Reset fence — `21741e9c87439d139ad41288bf2f70e3e09a962ef85331e3e640dd9bfd663126`:** the walking wall/recovery repeat passed with a generation/owner check before custom CCT writes and after the original movement call. Complete running RX bytes matched. Host fixtures also reset during a query and reject stale writes, exercise grounded support/vertical-velocity clearing, and keep autosaves held through recovery handoff.
- **Native loading hold:** native RequestPause/ReleasePause controls changed readiness 1→0→1 with both calls succeeding. The production hold uses the independently traced LoadingPause refcount, including nested pauses; host fixtures check invalid/missing/duplicate tables, resume and isolation from HCM's own pause. Noclip resumes from the native position instead of an old target. No predictive streaming-boundary claim is made.
- **Trainer investigation:** card/receipt fixtures were captured. Changing the archived fortress enum to Step02 made GameData's expression true, but capture persisted, even after changing the trainer subquest enum. Read-only inspection then confirmed the **live** fortress quest record still at Ready. The placed trainer's actual actor-logic graph enables security when the live step is before or equal to Step01. Production now reads the same live quest order through independently traced, signature-checked layouts, with bounded mapped tables and generation fences. A corrected native quest-service fixture (`729ed482a43600a6b1ccaf740bfe4fa318b0b1b3e67d91bb775ceec645cfedd7`, complete running RX checked) updated live and archived progress together, passed admission, completed arrival in 21.49 seconds and reopened R3 after 180 presents. Native Save produced a disposable qualified fixture. Pure production `58c690c59c0c94322dc51c4a8f258dccfa146fed23813bde71a04835477a4337` loaded it all OFF/1x/Return cleared, completed the trainer arrival in 15.34 seconds and reopened R3 after another 180 presents without capture. All 54,920 running RX bytes match SHA-256 `5cad84ba216cc5671282feadcdc7e1bdce9b928fa6136d0227bfcf6b5d8e6451`. This is a controlled native progression fixture, not a natural full quest playthrough. Saved-data enum changes alone are not accepted as a qualified progression proof. No grant/setter/fixture code is included in production.

Two movement sweeps near the Market bridge mixed camera turning, collisions and swimming; they exercise state transitions but are excluded from quantitative speed validation. The native game safely refused a moat travel request. A subsequent normal manual Load reset all active core toggles, normal 3x and Return; no forced warp was attempted.

**Recovery acknowledgement defect found during integration:** on production 58c690c5…, a verified nearby point at (-72.089752, 150.564636, 230.527985) settled to (-72.089516, 150.548904, 230.528183). The 1 cm acknowledgement bound timed out and safely reopened Noclip. First refinement `1a4c96809b3b2f3c9bf7b83ed78d72c3b1649598e96c32bee03903dff58aa624` passed native 1x/1.5x/2x repeats, but another 3x endpoint remained unconfirmed. Its actual dry pose was about 2 cm below the checked point; the cached contact flag can lag recovery. Current source retains the horizontal bound and verifies a fresh native floor ray within the existing query clearance before acknowledging that downward correction. Changed/missing floors, excessive drops, horizontal displacement and save resets remain rejected by production fixtures. The final refinement `3a6e0fbee7f12fc72971803b2fe1934c72f9339c55c1dd9fb32d41816541b9f0` passed all four native walking/Noclip shutdown repeats: each request received its matching nearby/completed acknowledgement without fallback or reload. Its 55,144 running RX bytes match SHA-256 `4b8cc3deef958e525ad559fbfdd2d0c672cbb068b058efa97ffd778f11aaced9`. Earlier affected prototypes do not replace the frozen fallback.

**Flat-ground speed controls on 58c690c5…:** native resolved/requested walking speeds were 5.4 / 8.1 / 10.8 / 16.2 for 1x / 1.5x / 2x / 3x. Sprinting gave 6.37 / 9.555 / 12.74 / 19.11. Actual displacement increased accordingly, with startup acceleration included. The ordinary four-setting controls are valid; the later Noclip shutdown failure is recorded separately. Current-build movement and FPS-off results are recorded above.

The expanded recovery, loading, admission, wallet and Noclip host fixtures pass. A rebuilt temporary production package passed member/document/NSO checks at this stage. Later integrated native results are recorded above; they do not extend the scope of these earlier checks.

## Current target and build provenance

Title `0400486024DEC000`, main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`; Windows Lichii SCM `3dae119aeac255a9334386cdf10ca707ef96867d`. The private executable's text hash, original hook signatures, native function signatures and free bridge padding are verified before building. The default build uses the same checked-in profile without a proprietary input.

The local game container was renamed since RC2. Its displayed metadata says 2.0.0, but the loaded executable retains the exact pinned Build ID and all checked 1.1.1 code signatures. This is evidence for that executable only; no general support for a different 2.0.0 executable or region is claimed.

The preserved RC3 baseline module has SHA-256 `2755cfb74fc2a0a11c145090f1923455326f7e2ecd1cbc143ea6572961d4a1ae`. Read-only inspection of all 45,240 loaded RX bytes matched text SHA-256 `63c1d09a375890579230b56d3b76107b98644d4a305e9ccbc86e19705861a9ca`. This identifies the tested running module. That hash identifies the preserved RC3 package; RC4 provenance is below. It contains the stock-menu input, owned-bag refill, Save-screen input guard, both river restrictions, membership lock and verified follow-on handling. Most per-entry smoke tests ran on the recorded `4ff32883…` / `5e7ac638…` revisions; the CSV retains that provenance. The RC3 final build repeated selected combined regressions and the changed arrival paths.

The integrated Noclip/handoff build exercised below has module SHA-256 `e73305c39ee17c003b0f57cb252a9747eb1d4f8866cbe0608387547cc5bfca59`. Earlier UX-only and collision prototypes have separate provenance and are not release binaries. Private debug clients, constructor fixtures, game dumps, copied saves and research code remain outside the package.

## RC5 implementation and focused checks — 12 November 2026

The complete RC4 project was backed up first: 232 files hash compared at sibling `Hyrule-Cheat-Menu-rc4-snapshot-20261112-161755`. Wayfarer was inspected as a technical reference only; no source, graphics or fonts were copied. Its inventory-function lead was independently traced against the pinned main. Its active-view lead confirmed that HCM's existing inverse camera matrix is correct. Its flight code does not supply verified streaming, capsule overlap or nearest-free safety.

The new consumption guard targets five ordinary ammunition-use calls, preserving the tail branch as well as ordinary calls. A sixth, nut-only guard is necessary because actual nut use reaches the shared native inventory event. Guards require identified Link, enabled Items, the current pouch, a supported age/name, unit quantity, zero unknown/alternate flags and safe gameplay. The generic native remove entry is not patched in the release. Other shared-event item names and non-unit removals remain native; single-nut scripted removals in otherwise safe gameplay share the nut guard. Broader quest/rental coverage remains a gate.

Production host fixtures pass for the actual guard and item policy: both ages, all six names, other event names, bottles/beans/keys/quest items, quantities, every flag, unreadable or unterminated names, loading/events and OFF/reset passthrough. The new bridges and native remove prologue were checked against the private dump and its zero-filled padding. Existing state, travel, storage, rendering and constructor tests remain separate from gameplay evidence.

**Controlled native inventory fixtures:** private diagnostic module `23737f76c3d4f77f7db0d59c4eda88bf2296f2ddc54b9160afef82358087b036` ran on copied saves. Its extra entry trace and L3 inventory actions are outside the installable source/package. Removing 20 existing sticks with the native remove API left zero stock and the owning bags intact; native Save recorded that zero. Items restored 20 while paused. Native Give upgrades then changed nuts 20 to 30, bombs 30 to 40 and wallet 500 to 1,000 with the cheats already ON. Giving the already-owned medium stick bag correctly left its capacity at 20. These are live native API fixtures of the production refill/capacity code, not natural depletion, purchase or progression proofs. Fresh-process public-build and ammunition-use results are recorded separately below after completion.

**Installable RC5 execution:** module `601d23736511dd1c8b919071d3249243fc64766c13273049c87275c83a9d49b0`; all 47,224 running RX bytes match text SHA-256 `0fad68bcd36504686526bdaee984855c52d5b45f55b5ce03b2ca72a5dcfc5a85`. A fresh process loaded the controlled zero-stick manual save with medium bag owned and stock zero; paused activation restored 20. No private trace, inventory action or extra entry hook is present in this build.

Real controller-use controls passed: nuts OFF 20→19→18→17 and ON five uses at 20; bombs and Bombchus OFF 30→29→29→28→28 (take out/release), ON six input cycles at 30; child seeds OFF 40→40→39→38→37 and ON six cycles at 40; adult arrows OFF 30→29→28→27→26 and ON six shots at 30. The consumption guard executed in the running game. Ordinary stick swings/jump attempts did not deplete stock and are not counted as break/burn proof; natural stick depletion remains open. Disabling Items retained quantities before subsequent normal use.

Combined regression passed on this module: core ON/normal 3x, Fly 10x and Noclip, bit-identical hover at y=85.605904, and Noclip OFF restored (-41.200005, 109.934250, -15.999056) with Fly retained. Clipped-source cross-age travel reached Young Lost Woods in 52.76 seconds with core states/3x retained and life 40, rupees 500, magic 240. Return reached the captured Adult Kakariko point in 52.17 seconds within 0.00001 units. Both river blocks and zero-card trainer/Favorites checks passed. An ordinary in-game Load from core ON/3x reset all OFF/1x and cleared Return; preferences remained. One private input sequence first cancelled the native Load confirmation and was corrected after inspecting the screen; that cancellation was not counted as a reset.

Short same-process, same-pose RC5 measurements after the controlled manual load: three four-second windows per condition, medians 52.27 FPS all OFF, 53.10 core/normal 3x, 58.43 menu open. Maximum observed hitches were 28.51, 154.95 and 113.23 ms respectively. These samples do not isolate mod overhead or establish a long-session leak result. The earlier RC3/RC4 same-save comparison remains historical.

**RC5 artifact checks:** private-dump and dump-free builds reproduce the tested module and ZIP. All host/native policy fixtures, the real constructor boundary fixture, exact 17-member/document checks, CRC and NSO hashes pass. Altered Build ID and code bytes are rejected. No diagnostic fixture, dump, save, external runtime or Wayfarer code/assets are included. The archive checksum is supplied alongside it; release work continues beyond this fallback candidate.

## RC4 focused regression — 12 November 2026

Current module SHA-256: `9fafe25854242f072693b7cfe2afb6550b2a6d896308d6631b0913e33c3f4528`. All 45,288 running RX bytes matched text SHA-256 `c5dcf0b8396a063313f3173d63055916b22d0e203851afb34624fbae2bf0e2eb`. The complete RC3 project was backed up first: 212 files hash compared.

**Bombchus:** ordinary controller use with Items OFF depleted 30 to zero. Paused activation refilled 30/30; OFF retained 30. A native manual save was made at zero. After closing and starting a fresh emulator process, that zero-stock save still refilled 30/30 while paused, before this process had observed positive stock. This uses the signature-checked native guide getter, enum Bombchu 36, rather than a guessed ownership offset or writes to saved flags. The private data mapping and native guide UI were traced; actual use changed the independent persisted flag. A Bomb Bag is never used as sufficient Bombchu evidence.

**Negative / reset cases:** after enabling all four core cheats and normal 3x, native New Game cleared effective toggles and speed to OFF/1x and reset the independent Bombchu flag. During the opening cutscene R3 did not open the menu. Once playable, all six ammunition stacks and Magic Beans started at zero and remained zero with Items ON. Magic stayed 0/0; its row was grey/readable and A could not enable it. Infinite Rupees filled the actual new-game wallet to 200; the stock menu independently displayed 200/200. An initial private assertion assumed the older game's 99 capacity and was corrected after observing the native 200 capacity; no implementation maximum was changed. Loading the progressed manual save normally reset toggles, speed and Return again.

The new-game bowling desk remained progression-gated after its normal explanatory dialogue; stock and guide flag stayed zero. Its actual rental-item lifecycle was not exercised. Prior acquisition/use supports the recorded saved-zero path, but rental/progression variations are still a final-release gate. No general never-unlock proof across every quest state is claimed.

**Combined RC4 regression:** all four core cheats plus normal 3x, Fly 10x and Noclip operated together. Below-floor y=84.403549 held bit-identical through 90 idle presents. Noclip OFF returned exactly to (-41.200050, 109.934242, -15.998958) while retaining Fly. A second clipped-source departure arrived at Young Lost Woods in 52.78 seconds with core toggles and normal 3x retained, health 40, rupees 500 and magic 240. Native Return completed in 66.94 seconds within 0.00001 units of the captured Adult Kakariko ground point. Both river activations and the zero-card trainer entrance were blocked; toggling Favorites preserved focus and card count stayed zero. These are selected regressions, not a repeat of all 214 arrivals.

**Automated / hardware scope:** production ammo-policy fixtures cover OFF observation without writes, saved ownership, unavailable archive, different-save reset, ages, owning bags, capacities and unknown ownership. The real constructor boundary fixture passed. No physical XInput devices were connected (all four slots disconnected). Logical controller injection is not physical-controller acceptance. The native safe-drop helper found during collision research checks a 0.2-unit sphere and does not establish Link capsule/streaming safety; it was not called from the mod. Historical RC3 pause, travel and performance evidence below is not silently relabeled as an RC4 run.

**RC4 performance / A-B-A repeat:** fresh-process samples used the same 09:24 copied manual save, exact Desert Colossus position (-155.041855, 95.379532, 10.319995), normal speed limit and three four-second windows per condition. Initial RC4 medians were 50.01 FPS all OFF, 50.98 core ON/normal 3x, 50.06 core+Fly+Noclip idle, and 52.63 menu open. The core sample included a 73.48 ms maximum hitch. Whole-emulator CPU was 4.99–5.06 CPU seconds/second during gameplay and 3.83 paused; private memory was about 10,001–10,474 MiB and working set 12,841–12,900 MiB. These are process totals, not isolated mod costs.

Because this was below the earlier RC3 roughly 60 FPS session, the preserved RC3 module was reinstalled and its entire running text checked. At the exact same source it measured 52.07 median FPS (p50 18.95 ms; p99 22.40–25.60 ms). Restoring RC4 and repeating gave 52.45 FPS (p50 18.81 ms; p99 23.27–25.08 ms). The current short A-B-A comparison does not show a clear new FPS regression, while the earlier 60 FPS result is not reproduced. It does not establish zero overhead, physical input latency or a controlled long-session leak result. RC4 was restored and its complete running text rechecked afterward.

**RC4 package checks:** ARM64 private-dump validation, current host/native fixtures, exact 17-file member list, all document bytes, ZIP CRC and NSO segment hashes pass. Private-validated and dump-free builds reproduce the same tested module and archive. Altered Build ID and original-code bytes are rejected. The checksum sidecar identifies the final local archive. Source/CI publication remains unexecuted.

## RC3 automated checks

- ARM64 compile/link with warnings as errors and no undefined imports; private-dump validation passed.
- Registry SHA-256 remains `eab3838d9626e28f432d97231a1befedb309349096d68fad7cd652186fc5d59d`: all 214 IDs, age/scene/spawn parameters and float bytes are unchanged.
- Every entry appears exactly once in its non-Favorite leaf group for its age. All 37 group links/counts, exact shared-scene exceptions, unique short labels, actual row-width limits and alphabetical Favorites pass independent assertions.
- Navigation tests cover wrap, repeat delay, one-step speed input, endpoints, reconnect edges, EXP consent, unsupported activation, nested Back/Left/Right, 100 section toggles, Favorite duplicate-focus handling and per-tab/per-age viewport restoration.
- The native CCT fixture executes the **production Noclip C implementation** with test native functions. It covers Link-only scope, original return/argument preservation, equal displacement over 30/60 elapsed-time steps, native jumps/floors, conservative floor loss, paused holding, Fly independence, safe OFF, scene/save invalidation, and acknowledged ground restoration before travel. A first-post-pause missing-actor regression and travel before the first movement job are included. This fixture does not validate real Havok geometry or game scene loading.
- The production Save input filter passes native-fixture checks for all travel/flight/Noclip hold combinations, Save-only Confirm-to-Back behavior, unaffected Load/stock UI values, preserved unrelated 64-bit input bits, and unavailable UI state. Its real controller/file-hash checks are recorded below.
- A second native fixture executes the **production travel arrival state machine**: a stable recognized scene mismatch releases the failed transaction after two seconds; delayed correct arrivals still complete; native events, reserved loading, missing actors and unsafe world states reset that mismatch window. Native recovery remains independent. This is a state-machine test, not destination geometry evidence.
- Dual-generation preference encoding, every truncated/single-byte-corrupt payload, invalid/duplicate IDs, future schemas, short writes, creation failure, newest-generation corruption and unavailable SD pass host tests. All 214 Favorites fit without schema migration.
- ZIP CRC, exact member list/document bytes, IPS profile, NSO segment hashes and malformed-executable rejection pass when rebuilt with current documents. Constructor boundary execution is described with the preserved RC2 evidence and is repeated during final packaging.

## RC3 real emulator results

**Fast Travel UI:** the compact region overview, separate Lake Hylia entrances, nested Water Temple group, concise labels, full confirmation, B cancellation, both age lists, stable Favorite insertion, saved viewport on tab/age changes, immediate Favorite notices and 30 real header toggles passed on the UX revision. Closing the menu flushed preference generation 21 and verified readback; subsequent integrated restarts retained those Favorites and the selected flight-speed preference. A real startup restored effective normal speed to 1× with all toggles OFF.

**Grounded Noclip:** a fixed world-direction control stopped ordinary movement against the Kakariko rock wall after about 6.4 world units. The independent bypass passed that boundary. The initial unrestricted prototype reached unsupported outer terrain, fell and triggered native recovery; it was rejected. The floor guard then repeatedly restored the last clear point instead of continuing into the void. Integrated 3× walking tests reached beyond the wall while preserving native floor heights; native contact correction can still move Link slightly inside geometry. Walking-mode OFF used native same-scene recovery and restored exact ground coordinates in about 6.1–6.5 seconds while preserving the four core toggles and normal speed.

**Noclip + Fly:** the integrated build crossed below Kakariko's floor and held exactly stationary through 90-present idle intervals. Five consecutive ON / descend / hover / OFF cycles returned exactly to `(-41.199421, 109.934326, -16.000332)` while retaining Fly, all four core toggles and normal 3× speed. Tested below-floor heights included y=93.914063 and y=99.182106. Both collision bypass and zero-drift hover were actual native execution, not fixture results.

**Travel from clipped geometry:** the first direct handoff raced native void recovery and was refused. The fix keeps Link scoped across the post-pause identification gap, requests safe-ground restoration in the movement job, waits for acknowledgement and dry-ground confirmation, then releases the native event. From y=99.776405 below terrain, the final tested handoff arrived at Young Lost Woods / From the Sacred Meadow in about 21.56 seconds. Fly and Noclip were OFF; God/Rupees/Items/Magic remained ON and normal speed remained 3×. Return restored the exact original safe Kakariko coordinates in about 6.53 seconds, rather than the clipped source.

The post-pause shutdown race was reproduced in an intermediate build: the first unpaused pad poll could temporarily lack Link identification and clear the recovery publication. Retaining the pending recovery until worker acknowledgement fixed it; the five-cycle real regression above passed afterward. Superseded prototypes are not the release package.

**Underwater Noclip:** at the Lost Woods pool, native flight stopped on the underwater floor at `(63.528271, 92.982079, -25.561115)` with the native wet bit set. Noclip crossed that floor to y=87.151367 and held exactly stationary through 90 presented idle frames. Right-stick input changed the camera basis while the body remained unchanged. Noclip OFF restored the exact underwater collision-valid point with Fly retained; Fly OFF then recovered exactly to the dry bank `(49.123749, 100.001953, -25.671219)`.

**Ceiling Noclip:** normal 10× Fly ascent stopped at y=58.796799 in the Deku Tree boss chamber. A second held ascent remained blocked. Noclip crossed to y=82.629013. After neutral input was processed, a 90-present idle had exactly unchanged coordinates. The first immediate position read was one native motion step earlier than the settled sample; the harness now distinguishes presented-frame completion from processing the released input. Noclip OFF restored the ceiling contact point without disabling Fly; Fly OFF restored exact source `(6.668784, 47.097523, 67.634064)`.

**Swimming travel:** a normal swimming departure from Lake Hylia's Gerudo-river arrival completed at its Zora-domain arrival, retaining the pre-water dry Return. Return from water restored exact Kakariko coordinates. This path never records a submerged/falling position as a dry anchor; a fresh water-only session correctly shows no Return until safe ground has been captured. Tested module `4ff32883bf7f257caad60c3bfd471d055d2d50d59e1b315ba7ab026e7ae72277` also contains the renderer format fix. An intermittent explicit native-event veto was observed on intermediate first-load/immediate-arrival requests; a fresh request succeeded. Error diagnostics now distinguish that veto from capacity or reservation failures. This remains under investigation, not declared fixed by a timing guess or blind automatic retry.

**Underwater source refusal:** immediately after the Adult Water Temple entrance arrival, two fresh requests for its mid-boss entrance were explicitly rejected by the native event manager (flags `0x28000`, empty parent scope). Link was in the native underwater swimming state. Ordinary camera/swim input moved from `(-4.548996, 121.564026, 43.455143)` to `(-4.593497, 124.210938, 32.895550)`; the next fresh request completed at the expected mid-boss scene/position. No forced event flags, automatic retry or destination-data edit was used. The exact native veto predicate remains unresolved. This source-state limitation is disclosed; a successful retry does not establish unrestricted underwater departure.

The current `5e7ac638…` sweep reproduced a refusal after Zora's Fountain / lower Ice Cavern exit and another after the Ice Cavern back entrance. Both sources were in native swimming states. Ordinary camera/swim input followed by a fresh request completed destinations 189 and 191 in about 8.25 and 7.72 seconds. The Ice Cavern source moved from `(-46.875328, 106.562523, 68.705872)` to `(-46.575691, 108.957581, 54.922180)` before the accepted request. This extends the documented source-state limitation; it does not establish a general fix.

**Progression-sensitive first visits:** Young Desert Colossus / Spirit Temple door (index 87) triggered the native Requiem lesson; Adult Death Mountain Crater / Bolero warp (index 177) triggered the native Bolero lesson. Both requests timed out awaiting ordinary dialogue and song input, with R3 correctly blocked. Completing each lesson through the displayed controller notes and finishing its dialogue restored normal play; repeat arrivals passed. The Bolero repeat on current module `5e7ac638…` completed in about 5.64 seconds. No story flags were written by the test tools, and no lesson was skipped. These native events can alter ordinary save progression; a smoke pass after a lesson does not prove all pre-lesson progression states.

**Genuine load reset with Noclip active:** from below Kakariko terrain at y=101.405952, the four core toggles, Fly and Noclip were ON (mask 175), with normal 3× and flight 10×. Stock System → Load restored the 05:32 Kakariko copied checkpoint. All toggles were OFF, normal speed was 1×, Return was cleared and the safe ground/camera resumed. Favorites, session consent and preferred flight 10× survived. The stock-menu input fix was then exercised on module `5f584af3…`: left-stick navigation changed the native equipment selection from the tunic to the Deku Shield while Fly/Noclip were ON below terrain; the body remained paused. The same input implementation is retained in the current build. Its separate Save and Load regressions are recorded below.

**Renderer regression:** a production software-renderer fixture reproduced the 10-bit format becoming opaque because two-bit alpha rounded 85% to 100%. Software coverage now stays at eight bits until blending into the native format. All six scales, four formats, menu/confirmation states, buffer guards, zero-fade background preservation, target-format encoding and 85% color checks pass. These are software checks, not actual 1080p/1440p/4K or HDR display acceptance. Genuine current captures remain 1280×720.

**Ammunition ownership audit:** the game's private item parameters explicitly grant `Item_NutBag_Small` / `Item_StickBag_Small` on first acquisition; all small/medium/large actor names are present in the verified executable. The refill policy now checks these owning bags for empty nut/stick stacks. Production-policy fixtures cover both ages, empty owning bags, upgrades, invalid counts/caps and never-unlock behavior. On current module `5e7ac638…`, 20 ordinary native nut uses depleted the stack to zero with all cheats OFF. Normal Save completed; its new 09:24 manual checkpoint reloaded with nuts 0, owned small nut bag 1, all toggles OFF and speed 1×. Enabling only Items while HCM was paused restored nuts to 20/20; beans stayed 2 and the child-inaccessible arrow stack stayed 17. Three subsequent native nut uses refilled to 20, and OFF retained 20. The zero-stick-at-load and both-age upgrade cases still need real coverage. Bombchus share bomb capacity; treating a Bomb Bag as proof of Bombchu acquisition would unlock them incorrectly. Zero-at-load Bombchus therefore remain conservatively unchanged until ownership has been observed during that session. That requirement is not marked complete.

**Manual-save protection investigation:** a real test on the superseded `5f584af3…` module showed that manual Save bypassed the attempted shared policy guard and wrote the below-floor position to the disposable profile (slot 04 progress/caption and option files changed). Owner saves were untouched; the copied slot was restored from its closed-emulator backup. That ineffective hook was removed. The current implementation reads the verified non-persistent FullScreenUIType and redirects Confirm to native Back only on Save while movement/travel holds are active. Load and other menus retain their input. Production input-policy fixtures pass. On module `5e7ac638…`, three separate native Save confirmations from below the Desert Colossus floor at y=88.670959 followed Cancel, with all 15 copied `.sav` file hashes unchanged. The native UI returned to System without a false Saved message; body position and mask 175 / normal 3× were retained. R3 displayed the in-menu explanation. Native Load of the 07:53 Desert Colossus autosave then restored safe ground at y=95.379532, all toggles OFF / normal 1× / Return cleared, with Favorites/flight preference retained. A subsequent ordinary grounded Save with all toggles OFF succeeded and changed the normal slot-04 progress/caption and option files. This is evidence for the tested configured-controller path, not every scripted save request or external UI.

**Earlier integrated regression:** module `5e7ac638…` repeated the combined movement/travel smoke test from Adult Kakariko at `(-41.199543, 109.934311, -16.000063)`. Noclip plus Fly descended to y=88.581894 and held bit-identical coordinates through 90 presented idle frames. Noclip OFF restored the exact ground point while keeping Fly ON. Re-enabling Noclip and travelling from below terrain to Young Lost Woods completed in about 42.50 seconds, clearing Fly/Noclip and retaining all four core toggles and normal 3×. Life, rupees and magic read 40, 500 and 240. Cross-age Return completed in about 53.89 seconds and restored the safe source within 0.000004 world units. The unsupported Young river entry also remained readable and rejected activation without a travel prompt/request. These are selected regressions on the current binary, not exhaustive gameplay acceptance.

**Final integrated regression:** current module `2755cfb7…` repeated the combined smoke from Adult Kakariko at `(-41.199997, 109.934258, -15.999073)`. Noclip plus Fly reached y=88.047127 and remained bit-identical through 90 presented idle frames. Noclip OFF restored the exact dry point with Fly retained. Clipped-source travel reached Young Lost Woods in about 42.55 seconds and cleared Fly/Noclip while retaining mask 15 and normal 3×. Life/rupees/magic read 40/500/240. Cross-age Return took about 51.00 seconds and restored the safe source within 0.000004 world units. Startup was all OFF / normal 1× while Favorites and preferred flight 10× survived.

**Gerudo edge-case fixes:** both Young and Adult valley river arrivals were independently repeated and immediately auto-exited; their exact IDs remain visible/favoritable but activation is blocked. The Adult route changed from Gerudo Valley at about `(9.142208, 13.660275, 68.112610)` to Hyrule Field and settled at `(-236.904648, 125.722427, 421.944641)`. The Training Ground exit instead triggered native trainer dialogue and imprisonment in the unqualified copied save. After its ordinary capture dialogue, normal menu access resumed. The corrected build reads the existing `Item_Token_Gerudo` through the pinned generic pouch query and locks only that entrance until membership is confirmed. With Items OFF, native card stock stayed 0; A created no travel prompt/request, and adding/removing its Favorite preserved focus and the original Favorites. Owned-card admission is covered by host policy assertions, not a real progressed-save arrival.

The Adult wasteland-edge vector was separately repeated: it naturally exits into Gerudo Fortress and settles at its closed gate. A 90-present idle held the exact point `(-31.015961, 104.923958, -104.680466)`, core states stayed selected and R3 reopened. The new label explicitly says it exits the wasteland; original request parameters remain unchanged. The bounded mismatch handler released both old-build transactions cleanly. The final arrival rerun is recorded in the CSV.

<!-- ARRIVAL_SUMMARY -->
Current RC3 arrival record: **211 arrival smoke passed**, **2 unsupported**, **1 needs manual test** out of 214 retained entries. The CSV identifies each tested module. A registry or menu test is not an arrival test. An arrival smoke pass records native expected-scene completion, finite actual Link coordinates and camera basis, settled ground/water, successful menu reopening and preserved core states; it does not prove every progression, entrance trigger, combat or return path.
<!-- /ARRIVAL_SUMMARY -->

**Live minigame pause on the final binary:** normal controller movement reached the Zora diving host and started the native 10-collectible diving challenge through its dialogue. The visible countdown and the float read by native `QueryMiniGameTimer` were observed together. During ordinary gameplay the counter moved from about 79.45 to 76.86 seconds. On reopening HCM it read exactly `4493.65087890625` native units (74.894181 seconds) and stayed bit-identical through three four-second measurement intervals; Link's coordinates also stayed exact. Closing HCM resumed the countdown to about 72.09 seconds. The native timeout/failure dialogue then ran normally. God/Rupees/Items/Magic remained ON and normal speed stayed 3×. This verifies that selected active minigame timer, not every minigame, audio path or enemy/projectile class. See the genuine pause capture in the README.

**Live boss pause on the final binary:** after the native Volvagia intro, read-only actor discovery identified the live `Enemy_DungeonBoss_Volvagia` root. It moved from `(-81.300003, 102.580002, 13.799999)` to `(-74.900002, 102.580002, -7.400001)` before opening HCM. The position and its 64-byte transform remained bit-identical through three four-second paused intervals. After closing, it resumed and reached `(-85.5, 102.580002, -5.400001)`. The four core toggles and normal 3× remained selected. This verifies that boss-root movement during the tested fight; it does not independently verify every attack, projectile, animation bone or audio bus.

## Final RC3 performance and removal

Current module `2755cfb7…` was measured at the manual 09:24 Child Desert Colossus checkpoint. The installed and absent runs used separate fresh boots of the same isolated emulator/configuration, the same manual save and warmed neutral input. All five conditions held the exact coordinates `(-155.041855, 95.379532, 10.319995)`. Three four-second frame samples were taken per condition. The emulator's built-in FPS path remained enabled (`oot_30fps=false`); no external add-on other than HCM was installed.

| Condition | Median FPS | Median p50 (ms) | p99 range (ms) | Process CPU s/s |
|---|---:|---:|---:|---:|
| Mod absent | 59.04 | 16.57 | 21.4–22.0 | 4.98 |
| Closed, all OFF | 59.93 | 16.56 | 19.4–23.7 | 4.91 |
| Closed, core ON / normal 3× | 57.97 | 17.03 | 18.4–21.9 | 5.03 |
| Closed, core + Fly + Noclip | 59.42 | 16.50 | 19.9–21.6 | 4.93 |
| Menu open, same toggles | 60.00 | 16.62 | 17.6–20.8 | 3.86 |

CPU s/s is the whole emulator process's accumulated CPU seconds divided by the full measurement command's wall time, across all its threads. It includes game/emulator work and diagnostic reads; it is not isolated mod CPU cost. The core-enabled run had a 54.9 ms maximum hitch. Short samples, a roughly 60 FPS ceiling, dynamic scenery, separate boots, shader/OS cache and host scheduling limit comparison. The menu pauses simulation and therefore changes the workload. These results do not establish an FPS improvement, zero overhead, or globally safe high-speed movement.

Whole-process private-memory snapshots ranged from about 10,550 to 10,859 MiB and working sets from about 13,692 to 17,850 MiB. Game/renderer/emulator caches dominate those totals; they do not measure the mod's allocation or establish a leak delta. Maximum mod display staging remains about 81.4 MiB by the allocation bound. An earlier `5e7ac638…` process observation recorded 132.9 minutes of uptime during the travel/interaction run without an observed crash; that includes idle/tool time and is not a controlled movement soak or leak study.

With HCM's folder moved out while the isolated emulator was closed, the same copied checkpoint loaded normally. R3 displayed no HCM panel, and read-only comparison of all 20 hook locations matched the exact original instruction bytes. The folder was restored while closed; SD preferences were retained. Only the isolated test profile was changed.

## RC3 package validation

The installable folder and deterministic ZIP contain exactly 17 files: the standalone module, Build-ID-specific IPS, six English review documents, destination coverage, seven genuine screenshots and the registry's MIT notice. The private-validated and dump-free builds produced identical module and ZIP bytes. NSO segment hashes, ZIP CRC/membership/document bytes, unsupported Build ID/code rejection, warnings-as-errors compilation and all host/native-fixture assertions passed. The actual private native constructor was rerun: its last store at +0x1b1b remained within the corrected 0x1b20 object and both external guards stayed intact.

Source and a dump-free GitHub Actions workflow are prepared locally. GitHub CI has not run; no remote push, public release or final tag has been made. No game binary, key, save, reference module, private research client or external runtime program is distributed. A SHA-256 sidecar accompanies the ZIP.

## Remaining acceptance gates

Noclip is mandatory for the requested final release. Selected real walls/floors/ceiling/underwater, camera, safe shutdown and active-save-reset proofs now exist; broad geometry/equipment, stairs/jumps, both ages, transitions and high-speed streaming remain incomplete. It is not a disabled placeholder and is not marked universally Verified. A bounded native full-player-shape nearby search and walking floor verification are implemented and exercised above; their fallback remains the captured safe anchor. No predictive streaming-boundary API has been verified. Other core damage/purchase/upgrade and boss/minigame/pause/audio tests still require expanded coverage. The visible focus/logical reconnect check above does not replace physical Xbox/PlayStation/Nintendo, background-physics, display/aspect or long-session acceptance. Savestate restoration notification remains unsupported.

## Preserved baseline movement measurements

These RC1 world-units/second measurements used the built-in FPS unlocker; they are historical evidence, not a repeat against every RC3 binary. Short intervals include acceleration and host presentation variance.

| Motion | 1× | 1.5× | 2× | 3× | Return to 1× |
|---|---:|---:|---:|---:|---:|
| Walk | 1.66 | 2.62 | 3.16 | 4.88 | 1.73 |
| Run | 3.60 | 5.88 | 8.34 | 9.60 | Area transition interrupted sample |
| Ladder climb | 1.11 | 2.05 | 2.17 | 3.35 | 0.80 |

The preserved baseline also measured swimming at every setting; broader Child/Adult, wall-climbing and FPS-off coverage remains required. Further RC2 gameplay and package evidence follows with its original revision clearly identified.

## Preserved RC2 evidence — 11 November 2026

Test date: 11 November 2026. Target: Windows, Lichii Build 130 (`3dae119aeac255a9334386cdf10ca707ef96867d`), game 1.1.1. Tests used an isolated portable emulator and disposable copied saves. The owner installation and the supplied Fast Travel reference were not edited.

## Evidence levels

Host assertions test state and file handling. The optional Unicorn check (`tests/check_private_request.py --main <private dump>`, with Python unicorn installed) executes an actual constructor from the owner's private executable. Neither proves gameplay. Emulator checks below used real game execution, logical Player 1 input, screenshots, and read-only coordinate/stat inspection. Physical-controller and owner acceptance tests remain separate. No test utility or debug server is required by the shipped mod.

## Automated checks

| Check | Result |
|---|---|
| ARM64 compilation and link, warnings treated as errors, no undefined imports | Passed |
| Private executable Build ID, text hash, source signatures, and free bridge padding | Passed |
| Default build without proprietary inputs | Passed locally; GitHub runner not executed |
| Menu navigation, wrap/repeat, reconnect edges, EXP consent, unsupported activation, tabs, save reset, speed endpoints | Passed host assertions |
| 214 unique stable destination IDs; finite coordinates; both age lists; all 214 Favorites and list capacity | Passed host assertions; not 214 arrival tests |
| Every truncated and single-byte-corrupt full preference payload | Rejected; caller state preserved |
| Unknown/duplicate IDs, checksummed future schema | Unknown/duplicate IDs removed; future schema rejected |
| Missing SD, partial writes, failed creation, corrupt newest generation | Passed simulated filesystem fault tests; previous valid generation retained |
| Real native request constructor against guarded memory | Passed: final store at +0x1b1b fits the corrected 0x1b20 buffer |
| ZIP CRC, exact members, IPS bytes, NSO segment hashes, malformed executable rejection | Passed |

## Real emulator results

- **Load and rendering:** standalone subsdk8/IPS loaded with matching signatures. The original panel, two tabs, sections, EXP consent, scrolling, Favorites, and native pause worked at 1280×720 captures. Startup initially failed with oversized static graphics storage; SDK-backed staging fixed that failure. It allocates 85,389,312 bytes (about 81.4 MiB) for maximum-size display staging.
- **Core regression:** four toggles and 1.5× normal speed remained selected through Adult → Young travel and Young → Adult Return. Wallet read 500, magic 240/240, life 40/40. Reloading the same Kakariko save reset all toggles, including Fly Mode, and normal speed to 1×; saved rupees returned to 106. Flight Speed stayed at its preferred 10× without activating flight.
- **Flight:** an earlier integrated RC2 build ascended from y=96.615776 to y=110.510124, then held the exact coordinates through a 90-present idle input. Earlier isolated prototype tests also exercised horizontal motion and descent blocked by ground geometry. The six-setting sweep below exercised ascent, descent, and stationary hover before the final underwater correction.
- **Safe deactivation:** midair OFF initiated a native reload and returned exactly to `(-119.7829895, 96.6157761, 58.6927338)`. Life stayed 40/40 with God Mode OFF. Repeated recovery completed in about seven seconds in the tested Kakariko scene.
- **Travel:** Adult Kakariko → Death Mountain entrance arrival was exercised in an earlier RC2 development build. The corrected-buffer build completed Adult Kakariko → Young Hyrule Field, From Kakariko (ID `6f7d5ff8`), then Return restored Adult Link and the exact original grounded coordinates. Each cross-age transaction used three native events and took about 56 seconds. No copied `.sav` file hashes changed during the first cross-age transaction; broader progression safety remains unverified.
- **Manual save:** the stock save action completed in the isolated profile without resetting God Mode. A grounded Child Zora checkpoint was saved through the normal controller UI for repeatable water testing; its directory was copied only after stopping the test emulator. This is a developer fixture, not a shipped automatic-backup feature.
- **Additional travel/safety:** Adult Lake Hylia, Serenade warp, and Young Zora's Fountain, Fountain (story spot), arrivals completed. An airborne travel confirmation was refused without dropping Fly Mode or unpausing the menu. One scripted destination navigation overshot because a seven-frame press crossed the real-time repeat delay at low FPS; the confirmation named the actual destination before that deliberate test warp. Subsequent scripted navigation uses shorter presses.
- **Final-binary travel/reset:** Young Kokiri Forest, From Hyrule Field (ID `2c15fea0`), arrived through the native named spawn. Four toggles and normal 2× speed stayed selected, Fly stayed OFF, and the camera initialized. Five selected registry arrivals now have evidence across RC2 revisions; the other 209 remain untested. After enabling Fly at the destination, the stock Load menu restored the saved Zora checkpoint: all toggles were OFF, normal speed was 1×, rupees were the saved 106, and preferred Flight Speed remained 10×. Native autosaving resumed after travel; the copied profile created a normal Kokiri autosave. Save hashes are not expected to stay unchanged after normal gameplay resumes.
- **Preferences:** startup loaded a previously written Favorite. Y added a Young destination; repeated speed changes alternated and verified generations on the emulated SD card. A preference write after native recovery succeeded after the request-buffer correction.

### Underwater correction and collision

The first flight hook was overridden by later native buoyancy modifiers and automatically resurfaced Link. An attempted special-motion hook did not resolve that failure and was removed. The working implementation also applies the scoped velocity after the native velocity-modifier job, retaining the native collision solver.

At Child Zora's Fountain, 1× flight moved about 10 world units horizontally in 2.505 seconds. LT submerged Link to `(51.751286, 97.437126, -0.561763)`, with the native wet flag set and reported depth 4.412. A 6.801-second idle had exactly unchanged coordinates. RT then ascended through the surface. At 10×, underwater horizontal movement and a 5.081-second zero-drift hover passed at depth 4.974. Right-stick camera input changed the native camera matrix without moving Link. Midair OFF returned exactly to the dry source platform in about 5.39 seconds; four core toggles and normal 2× speed survived.

Native geometry blocked upward movement beneath Jabu-Jabu and constrained descent against underwater floor geometry. These are selected collision observations, not proof of every wall, speed, or scene boundary. Fly does not suppress ordinary underwater damage: an independent God-OFF test remained submerged for about 14.65 seconds with only 0.000004 world units of horizontal variation, while native health loss still occurred. The native controller considered that underwater floor grounded, exposing the need to reject submerged recovery anchors. The correction uses the wet bit and body path verified in the private executable's water-depth query, with both code signatures pinned.

After the dry-ground correction, the final binary repeated the God-OFF test at `(53.024460, 100.493324, 4.293661)`, wet flags 285, depth 1.355. Four idle intervals totaling 14.184 seconds had exactly unchanged coordinates. OFF then queued native recovery and returned exactly to the original dry platform `(53.498299, 104.549698, -6.266593)`; the log recorded recovery completion after about 5.14 seconds. Life read 32/40 afterward, reflecting the unprotected underwater test. Fly is independent of God Mode.

No verified streaming/altitude-boundary query or predictive speed limiter has been implemented. Native animations remain in use; a dedicated flight-pose override, menu sounds, and vibration are absent. Broad water/equipment, world-boundary, animation, boss, and minigame acceptance remains outstanding.

### Flight sweep

All six settings were selected through the menu again on the final post-modifier/dry-ground binary at Zora's Fountain. The four core toggles were ON and normal Movement Speed was 3× throughout, exercising their coexistence with independent flight speeds. Each sample used 18 presented frames for ascent, idle, descent, and stopped input. These short, variably paced samples establish response, not high-speed streaming safety. All idle and stopped samples had zero change on all three axes. Native ground contact introduced a small horizontal correction during the 2× descent.

| Flight Speed | Ascent (world units) | Descent (world units) | Ascent wall time (s) |
|---|---:|---:|---:|
| 1× | 2.169 | -2.118 | 0.582 |
| 1.5× | 3.106 | -3.111 | 0.532 |
| 2× | 4.217 | -4.274 | 0.545 |
| 3× | 7.057 | -6.244 | 1.146 |
| 5× | 10.512 | -9.372 | 0.567 |
| 10× | 19.380 | -19.746 | 0.493 |

The 3× interval included a host pacing hitch. Presented frames, simulation steps, and wall time are not interchangeable; these samples do not establish stable real-time throughput under host stalls. Wallet, magic, and health read 500, 240/240, and 40/40 after the sweep. Safe deactivation restored the dry source platform.

Normal speed measurements from RC1 are preserved in the RC2 safety snapshot: walking, running, swimming, and ladder climbing all changed at each setting. That evidence does not establish every Child Link, wall-climbing, or FPS-off path in RC2.

### Resolved defects

1. Favorite insertion shifted selection: reproduced in host tests, corrected, retested.
2. Static overlay buffers exhausted Program memory: reproduced at boot; moved display allocation to the verified SDK allocator and confirmed successful boot.
3. Flight cleared after menu close: reproduced in the integrated build; retained the scoped zero-velocity publication across the first post-pause identification gap. Subsequent integrated ascent/hover succeeded.
4. Travel followed by saving a speed preference crashed: the native constructor's final byte write exceeded an undersized buffer and cleared a byte of the cached filesystem close pointer. Corrected the size, pinned the tail signature, added guards, reproduced the old boundary in actual ARM64 constructor execution, then repeated recovery and preference writes successfully. Pre-fix packages are not release artifacts.
5. Native buoyancy overrode underwater flight: traced the later modifier job, applied velocity only for Link's matching native job, and retested underwater hover, horizontal movement, camera input, and ascent.
6. Underwater floors were accepted as recovery ground: reject the native wet flag before accepting an anchor or treating flight shutdown as grounded.

## Preserved baseline checks

RC1 real tests covered a lit bomb paused for 12 seconds, protected versus unprotected blasts with recoil, ammo refill, Din's Fire consumption/refill, ordinary save reload, another save, Game Over → Retry, install/removal, and coexistence with standalone Fast Travel and the built-in FPS unlocker. Values from the preserved baseline include life 40→38 on an unprotected blast, unchanged 40/40 plus recoil when protected, bombs 27→30, arrows 17→30, and a 60-unit Din's Fire cost when magic protection was OFF. These were not all repeated against every new RC2 build.

## Performance and stress

Thirty complete menu open/close cycles (1,920 presented frames in about 69 seconds) and ten rapid R3 reversals completed with all four native pause masks returning to zero. A first monolithic stress request exceeded the debug client timeout; it was rerun successfully in six measured batches. This is a short stress test, not a long-duration soak.

Three four-second samples per condition at the same recorded Kakariko coordinates gave the following results on an earlier corrected-buffer RC2 revision, before the final post-modifier hook:

| Condition | Median FPS | Median p50 interval (ms) | p99 range (ms) |
|---|---:|---:|---:|
| Hyrule mod absent | 28.67 | 32.85 | 46.2–101.6 |
| RC2 closed, cheats OFF | 29.99 | 33.13 | 38.1–43.2 |
| RC2 closed, four toggles ON / speed 3× | 30.89 | 32.12 | 37.2–41.7 |
| RC2 menu open | 31.16 | 31.64 | 35.8–42.9 |

The mod-free run had warm-up hitches. Separate boots, host/renderer variance, dynamic scenery, and paused simulation limit comparison. These samples do not establish an FPS improvement, zero overhead, or isolated hook CPU cost. Long runs and high-resolution memory/performance checks remain required. Removing the mod folder also allowed the copied save to load normally without a Hyrule module.

The final post-modifier/dry-ground binary was sampled again at the fixed Child Zora platform, three four-second intervals per condition:

| Final binary condition | Median FPS | Median p50 interval (ms) | p99 range (ms) |
|---|---:|---:|---:|
| Closed, all cheats OFF | 37.15 | 26.62 | 31.2–36.2 |
| Closed, four toggles ON / normal speed 3× | 37.18 | 26.53 | 30.4–36.1 |
| Menu open | 38.69 | 25.77 | 28.6–32.6 |

One enabled sample had a 148.8 ms maximum hitch. This final smoke test has no same-scene mod-absent control, so it does not quantify mod overhead. It confirms that the new job hook ran through these short measurements without an observed persistent slowdown or crash.

### RC2 binary identity and package

The tested module's SHA-256 is `023394e8e58dcdc87ebfada759dc7c7d5a56165bcc0b38493eda81330bcba623`. Read-only inspection of all 34,996 bytes of its loaded RX text matched the NSO text hash `189d617cb3f88f84789a173af4933e9d5da2968dcd87e71a803fe119154b9da5`. The package build is required to match this tested module; documentation and screenshot changes do not alter it.

The RC2 review ZIP had 14 files: the module, Build-ID-specific IPS, six release documents, destination coverage, four genuine PNG captures, and the registry's MIT notice. No private executable, emulator, debug client, reference module, keys, or save fixtures are shipped. Final checks include a private-dump-validated build, dump-free rebuild comparison, host assertions, native constructor bounds, ZIP integrity, and staged-file whitespace validation. GitHub CI remains unexecuted.

## Owner acceptance checklist

Use a copied save, then record the exact build, scene, age, other mods, and result for each failure.

1. **Release blockers first:** exercise independent Noclip and combined Fly through varied walls/ceilings/floors and water, safe OFF with and without Fly, all six flight speeds, high-altitude/streaming boundaries, recovery over moving/unsafe surfaces, and a rejected airborne travel request. Confirm no residual flight after area changes or save loads.
2. Test every destination in [destination-coverage.csv](docs/destination-coverage.csv), including both ages, Return, progression-sensitive entrances, camera initialization, and repeated travel. The CSV records 211 smoke passes across RC3 revisions, two blocked river exits and the later production trainer arrival from a controlled native progression fixture. Repeat final-build coverage where an earlier module is recorded; do not force blocked entries.
3. Test every cheat alone and together: enemy/environment/boss damage and safe falls, shop spending, wallet/ammo upgrades, zero-quantity owned items, Bombchus, child-only Deku items, locked and upgraded magic. Verify normal behavior after disabling.
4. Pause bosses, projectiles, and minigame timers. Hold the menu open, disconnect/reconnect the physical controller, alt-tab, and rapidly reverse R3. Check BGM, sound effects, input isolation, and return to gameplay.
5. Verify 1080p, 1440p, 4K, 16:10, and ultrawide views; non-Xbox mappings; FPS unlocker on/off; repeated same/different save loads; and a long session with frequent travel and preference writes.
6. Compare a mod-free run in the same scene/configuration. Test clean removal and reinstall without deleting SD preferences. Savestate restore notification is unsupported; reload a normal save to establish reset state.

A passing host build does not satisfy these runtime acceptance gates.


</details>
