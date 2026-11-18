# Release readiness — Hyrule Cheat Menu v1.2.0

The pinned ARM64 build and host regressions passed. The isolated Build 177 run verified normal placed and procedural cuts, stock special drops, the same-scene OFF control, Infinite Rupees, a scene transition and a stock save reload. The report records the module used for each result. [Test report](TEST_REPORT.md)

Isolated QA cleanup passed. The publication job builds and tests the release commit, checks the tag is available and attaches the verified archives. Verify the published release and download afterward.

---

# Release readiness — Hyrule Cheat Menu v1.1.3

15 November 2026. **Local release GO: native functionality, host regressions and cleanup PASS within the documented scope.** Deku Seeds support confirmed first acquisition through the required native base bag and preserve existing 40/50 capacities. Magic Beans use native capacity 10 and need no previous purchase. Young Link edits, confirmations, deferred acknowledgement, Save/travel holds and uncertainty fences remain. The approved UI, navigation, unrelated inventory and existing features are preserved. [Inventory contract](docs/v1.1.3-inventory-verification.md) records the mechanisms.

Final module `b247a9ac86c52b3ae033c9367e55bff0d67d68a94437ca5d170cbe22b46f81ba`; loaded RX 83,672 bytes, SHA-256 `61ac1ef8cca49a2984eb2d1babcf8c27dcde891a57e980088eb0ddfbef1d705e`. RX/RW match gameplay-tested `70aef1a94c5f9fc996acf638531ce541261919cc1b2ced3e802c9505467a70a1`; only two read-only footer version characters differ. Final loaded identity and V1.1.3 visual footer smoke pass. Individual native records keep their actual binary scope.

All 48 native observations validate. Corrected-logic tests establish never-purchased beans, exact first seed grant with the related base bag, normal Slingshot consumption and bean planting with Infinite Items ON, native Save/Load quantity and removal persistence, and measured progression preservation. Final-binary tests establish Adult/tool/Infinite Items refusals, zero-stock grant/cancel/remove/increase/decrease/maxima for both stacks, retained acquired 40-seed capacity and preservation of purchase counter 4/two existing plants. [Test report](TEST_REPORT.md) and [native evidence](docs/v1.1.3-native-results.json) retain exact measurements and limits, including no separate native 50-capacity run.

The final ARM64 build and complete host runner pass: 19 C fixtures, 1,176 renderer cases and installation/source ZIP checksum/layout/profile checks. The frozen module matches the release binary. Isolated emulator cleanup passes: all 15 copied saves, two preferences, two ExeFS files and three configs match their pretest baseline; owner data is untouched. No critical crash, softlock, save corruption or major regression was observed in the completed scope.

Publication is authorized to existing `fxbiidev/hyrule-cheat-menu`. Repackage the finalized evidence/documentation and verify exact archive bytes before the release commit. CI must then pass; the workflow publishes only exact `Release v1.1.3` on main, refuses existing tags/releases and attaches the tested artifacts. Verify the published tag, release URL and downloaded bytes afterward. These external delivery steps are not claimed complete here. Preserve previous history/releases, visibility and licensing.

---

# Release readiness — Hyrule Cheat Menu v1.1.2

15 November 2026. Follow-up to v1.1.1 commit `822ba40`: both creator/version labels move three logical pixels upward to clear the lower inner border. ZIP creator-platform metadata is pinned for reproducible Windows/Linux archives. The approved layout, controls, inventory logic and save-safety mechanisms otherwise remain unchanged.

Local ARM64/host/renderer/archive validation passes for module `35e338333455b091d9b1208bcd9c5a1d3b301db5163ade208e15d46b9fe1c942`. The complete runner includes 19 C fixtures and 936 renderer cases; exact archive members, checksums, source/document bytes and pinned ZIP creator-platform metadata pass. Native ownership/persistence evidence remains explicitly scoped to the v1.1.1 module below; no new ownership gameplay test is claimed for this visual adjustment.

Native footer acceptance also passes: the reviewed in-game capture shows the creator/version labels clear of the lower inner border. [Footer evidence](docs/v1.1.2-footer-results.json) retains its exact scope. Final documentation/evidence packaging and clean-source reproduction remain delivery gates before commit.

Publication requires passing local build/regressions/package checks, successful CI and a separate `Release v1.1.2` main-branch commit. Verify the new tag and downloaded assets while preserving the existing v1.1.1 release and older history, visibility and licensing.

The v1.1.1 readiness record below is retained unchanged for its own binary and scope.

---

# Release readiness — Hyrule Cheat Menu v1.1.1

15 November 2026. **Native and host acceptance PASS. Local release GO within the scope below; frozen-source package and external delivery checks are mandatory before publication.** Published v1.1.0 is preserved at `fbfb5cb4e9ebff0987fc2d5f353abbd547bd3351`; this is a separate backward-compatible patch.

## Exact supported runtime

Windows Lichii Build 177, displayed update 2.0.0, title `0400486024DEC000`, main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`.

- Module SHA-256: `5a88f46f2ad013c2881085d81c063a483fd51e00c37b44410c37233aac425443`.
- Loaded RX: 82,728 bytes; SHA-256 `09346ed8625397d0856986ac42bc37aec05e54ce2990e99254a49338eff044ef`.
- All loaded RX bytes matched the independently retained module model before native actions.

## Acceptance gates

| Gate | Evidence |
|---|---|
| Goron Tunic ownership and native inventory | PASS: genuinely absent0 → confirmed add1 → stock equipment screen and Adult equip. |
| Granted ownership persistence | PASS: save equipped Goron 1 → unsaved removal0 → stock Load restores Goron 1 and equipped `Armor_Adult_Goron`. |
| Removal persistence / equipped safety | PASS: equipped removal0 restores `Armor_Adult_Normal`; save removal0 → unsaved add1 → stock Load restores absent0 / normal tunic. |
| Other equipment categories | PASS: Mirror Shield and Iron Boots granted, equipped through stock UI, removed with saved references cleared; unowned Megaton Hammer0→1→0. |
| Stack / dependency policy | PASS for measured Adult nuts/arrows/bombs/Bombchus0→1→capacity; Bow-with-arrows and Hookshot/Longshot conflicts refused. Young sticks/seeds0→1→20/40 pass; both-age refusals pass. Infinite ON does not recreate removed Bow/arrows; ordinary restored Bow firing30→29 works. |
| Host / ARM64 | PASS: warnings-as-errors module build,19 C fixtures, all 20 ID ownership cases, age/capacity/dependency/confirmation/epoch tests, renderer 936 cases and original feature regressions. |
| Frozen installation/source archives | The complete build/runner validates exact maintained bytes, checksums, NSO hashes and archive membership; final documents/evidence are rebuilt and clean-source reproduced before commit. |
| Isolated test cleanup | PASS: emulator stopped; all 15 copied saves, two preferences, two ExeFS files and three configs equal baseline bytes. Owner data untouched; cleanup is recorded in public native evidence. |
| CI / publication / downloaded assets | Pending exact release commit; verify successful branch CI before main, main publication job, tag target and downloaded asset bytes. |

The approved Emerald & Gold design and three tabs/controller navigation are preserved. Preference schema remains 2 with existing migration. Runtime changes are confined to Item Manager and the required Infinite Items ownership interaction; movement, Free Camera, combat, travel and save-reset implementations remain intact and retain host regressions. Historical native results for those features keep their own version/module scope in TEST_REPORT.md.

## Boundaries and rights

The 20-ID catalog excludes quest/trade items, bottles, dungeon keys, beans, upgrades and arbitrary save addresses. Current age, native capacity, shared Hookshot/Longshot slot and Bow/Slingshot ammunition prerequisites are enforced. Stock removal empties ammunition without deleting its tool/bag. Pending updates keep existing Save/travel holds; uncertain updates require normal save reload. Broad maps/progression/peripheral/soak testing and native timeout fault injection are not claimed.

Publish only after these local gates pass and no critical crash, softlock, save-corruption or major regression remains. No such blocker was observed in the completed native/host scope. User publication authorization covers the existing public repository. The workflow publishes only exact `Release v1.1.1` on main after its build, refuses existing v1.1.1 tag/release and uploads the tested artifacts. Preserve previous history/releases/visibility and third-party notices. No game files, keys, copied saves or private QA clients ship.
