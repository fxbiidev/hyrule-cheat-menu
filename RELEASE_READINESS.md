# Release readiness — Hyrule Cheat Menu v1.1.0

15 November 2026. **GO for local v1.1.0 release on the supported target.** Runtime, host, package and clean-source reproduction acceptance have passed. Live CI, tag/publication and downloaded assets are verified separately after commit. Design approval is complete; another design approval is not required. This is the current v1.1.0 decision; historical v1.0.0 GO decisions apply only to their own binaries.

## Supported target and frozen runtime identity

Windows Lichii **Build 177**, displayed update **2.0.0**, title `0400486024DEC000`, main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`. Compressed main SHA-256: `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`.

- Module SHA-256: **`ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`**.
- Running RX: **82,728 bytes**, SHA-256 **`8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922`**.
- Exact results and prior-module provenance: [TEST_REPORT.md](TEST_REPORT.md), [sanitized native evidence](docs/v1.1.0-native-results.json).

## Acceptance status

| Gate | Current evidence / remaining work |
|---|---|
| Owner-approved design | **PASS.** CHEATS / FAST TRAVEL / ITEM MANAGER, uppercase subtitle `STANDALONE CHEAT MENU FOR OOTR`, exact credit `Created by fxbii.dev`. |
| Combat | Current module passes accepted **2→3** Master Sword at 1.5×, **2→4** Fairy Bow at 2×, One Hit Kill **2→3** against life 3 despite stored 3×, then immediate stored 3× resumption **2→6**. **PASS for all four factors and priority/OFF.** At 1× a fresh Blue HP3 became HP1 with modification count unchanged; God-OFF incoming damage at outgoing 3× remained native 2, Link HP40→38. |
| Free Camera | **PASS in current native scope.** Translation 28.968 units, rotation and both triggers left Link exactly stationary. R3+B/menu OFF restored native eye within 3.55e-5/1.81e-5. Fly+Noclip combination and strict held-exit edge kept zero Link movement, then resumed Fly after neutral. Fast Travel deactivated camera. |
| Safe Item Manager | **PASS in current native scope.** Arrow cancel/decrement/increment/empty/refill **5/4/5/0/30**, verified notices, nuts/bombs/Bombchus decrement/refill and gear/age/Infinite Items refusals. Young Seeds **40→39→40**. Stock Save/load restores **7→3 unsaved→7 loaded**. |
| Reset / upgrade compatibility | **PASS in measured scope.** Current native Load clears active controls and effective speed/damage while preserving stored damage, Flight Speed and four Favorites. Frozen v1→v2 migration and all-Favorite/corrupt-storage fixtures pass. Active-camera save interruption is covered by host fixtures, not a separately recorded native load with camera ON. |
| Existing functions / UI | **PASS for targeted checks.** God-OFF natural Fly OFF from 24.935 units retained health 40. Native Young travel, settled Adult Return and Kakariko follow-up passed. Favorites **4→5→4** restored original IDs exactly; both three-tab wrap directions restored item selection/viewport. Original resource/movement/recovery/Save/travel fixtures pass; earlier broad sweeps retain their historical scope. |
| ARM64 / host / graphics | **PASS for the completed checks:** warnings-as-errors ARM64 build, 21 C fixtures, 744 renderer cases and executed bridge ABI checks. |
| Install/source ZIP | **PASS.** Full build/runner validates install and source ZIP membership, checksums, canonical metadata and exact maintained bytes. Packaged NSO is byte-identical to native-tested `ab2a5a1e…`. Clean extracted source reproduces both ZIPs and SHA sidecars byte for byte. Revalidate the final document-only refresh before publication. |
| CI / publication / download | **Delivery verification after commit.** Confirm successful CI, exact tag target, release assets and downloaded checksums; no future pass or release URL is claimed by this pre-publication source document. |

Release blockers are reproducible crashes/save corruption, game-breaking softlocks, unsafe mandatory features, serious camera/controller failures and major v1.0.0 regressions. Minor cosmetic or additional unsupported-combination coverage can be deferred. All required local gates pass within the documented scope; no unresolved release blocker is known there. CI/publication and downloaded assets are verified against the exact committed artifacts as delivery proceeds.

## Supported limitations

Combat is restricted to eight exact ordinary-enemy variants with checked player attribution; Link, friendly NPCs, bosses, minibosses and unknown targets remain native. Native caps and phases remain in force. Inventory has 20 real-ID views, six acquired-stack quantity editors and fourteen read-only gear entries; it cannot grant unsafe quest/equipment progression. Every mutation requires confirmation and Infinite Items OFF. Pending native updates hold new edits, Save/autosave and travel; timeout or ambiguous results make the save epoch read only until normal reload, without retry or rollback.

Free Camera leaves the world running and has a direct R3+B exit; input remains drained through neutral release. If native Fly recovery is refused while airborne/clipped, Fly/menu stay visibly ON. Descend under Fly, or use Fly+Noclip to ascend clear of a floor, restore collision and disable Fly. Automatic anchor teleportation is not guaranteed. Savestates require a normal save reload. Additional maps, equipment/progression states, physical controllers, displays and soak coverage retain their disclosed scope.

## Cleanup and authorized publication

The isolated emulator is stopped. All **15 copied save hashes**, isolated preferences and the baseline ExeFS module match their pretest state. The owner installation and saves were not modified.

## Rights and delivery

Publish only to the existing public [fxbiidev/hyrule-cheat-menu](https://github.com/fxbiidev/hyrule-cheat-menu), whose identity and write/admin access were verified. Preserve history, visibility, licenses/notices and published v1.0.0 assets. The prepared workflow requires a successful build from main with exact commit message `Release v1.1.0`, refuses an existing v1.1.0 tag/release and publishes the tested archives. Final tag target, release URL, assets and downloaded SHA-256 still require verification.

The supplied Clay Mullis Fast Travel registry MIT notice is retained; no project-wide license is added. Game assets/binaries, keys, owner saves, reference compiled modules and private QA clients are excluded. Discord support remains `DISCORD_INVITE_URL_TO_BE_ADDED_BY_OWNER`.
