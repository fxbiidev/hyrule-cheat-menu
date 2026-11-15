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
