# Release notes — Hyrule Cheat Menu v1.1.0

**Created by fxbii.dev** · Windows · Lichii Build 177 · displayed update 2.0.0

## What changed

- **Approved Emerald & Gold redesign:** a wider contextual layout, clear status/selection, controller hints and three independently remembered tabs: **CHEATS | FAST TRAVEL | ITEM MANAGER**. Subtitle: `STANDALONE CHEAT MENU FOR OOTR`. Player, Movement, Combat and Camera & World stay collapsible; Damage Multiplier is directly below One Hit Kill.
- **One Hit Kill and Damage Multiplier:** checked player-originated damage on a conservative eight-variant ordinary-enemy allowlist. Values are 1×, 1.5×, 2× and 3×; fractional halves round upward. One Hit Kill takes priority and the stored multiplier resumes immediately when it is disabled. Bosses, minibosses, Link, friendly NPCs and unsupported targets retain native handling.
- **Free Camera:** left-stick movement, right-stick rotation and RT/LT altitude, with native restoration, unsafe-transition shutdown and direct **R3+B** exit. The world continues to run. Camera controls are isolated from Link/Fly and drained until neutral after exit.
- **Safe Item Manager:** real ownership views for 20 native item IDs. Six previously acquired ammunition/consumable stacks support bounded quantities, emptying and refill; fourteen weapon/shield/equipment entries remain read only. Every change needs a separate confirmation and Infinite Items OFF. Native queued changes are acknowledged on later frames; timeout or ambiguous results require normal reload before further edits.
- **Upgrade and reset safety:** preference schema 2 reads existing v1.0.0 speeds/Favorites and stores the damage selection without enabling it on load. Full save reloads reset active cheats and effective speed/damage while retaining safe preferences.
- **v1.0.0 features retained:** native menu pause, all resource toggles, normal/flight speed ranges, Fly/Noclip recovery and Fast Travel's 214 records, 37 groups, Favorites, ages, Return and confirmations.

## Release verification

Current module SHA-256: `ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`. All **82,728 running RX bytes** match SHA-256 `8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922`. The supported game main Build ID remains `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`; compressed main SHA-256 is `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`. Matching version labels alone do not establish compatibility.

The current module's native checks pass 1.5× Master Sword damage **2→3**, 2× Fairy Bow damage **2→4**, One Hit Kill **2→3** against a fresh life-3 Blue Tektite while 3× stays stored, and immediate 3× resumption **2→6** after disabling One Hit Kill. The 1×/OFF Fairy Bow control leaves fresh Blue HP3 at HP1 with no mod modification; God-OFF Link receives ordinary damage 2 at outgoing 3× (HP40→38). Stock Save/Load restores saved arrows **7→3 unsaved→7 loaded**, resets active controls/effective speed/damage and preserves stored damage, Flight Speed and four Favorites.

All **21 portable C fixtures**, **744 renderer cases**, warnings-as-errors ARM64 compilation and executed assembly ABI checks pass. Earlier inventory/camera/exit-edge native results retain their recorded module identities. Current-module camera, inventory, Fly landing, cross-age travel/Return, Favorites and three-tab navigation checks also pass in the recorded scope. The complete build/test runner passes install and source ZIP checks. The packaged module matches the native-tested module exactly, and clean extracted source reproduces both ZIPs and both SHA-256 sidecars byte for byte. CI/tag/publication and downloaded assets are verified separately after the release commit. [TEST_REPORT.md](TEST_REPORT.md) gives exact scope and failure/fix history.

Current native Free Camera checks moved the camera 28.968 units, exercised rotation and both altitude triggers, and kept Link exactly stationary. Both exits restored the native view; combined Fly/Noclip and the held-R3+B edge had zero Link displacement until neutral, then normal Fly resumed. A God-OFF natural Fly landing from 24.935 units retained health 40. Item Manager passed confirmed quantity/empty/refill operations and safety refusals, including Young Deku Seeds 40→39→40. Young travel/settled Adult Return passed; Favorites 4→5→4 restored original IDs exactly, and both tab-wrap directions preserved selection/viewport. [Sanitized native evidence](docs/v1.1.0-native-results.json) records these exact cases. Genuine final screenshots are linked from [README.md](README.md).

## Scope and limitations

The combat allowlist covers Red/Blue Tektites, ordinary/Fire/Ice Keese, ordinary/large Stalchildren and Octoroks. Other enemies, bosses and scripted phases are not modified. Native caps and invulnerability remain enforced. The current native tests cover the stated Tektite/melee/projectile cases; they do not establish every enemy, boss phase, NPC, reward, weapon or progression combination. Save-epoch interruption with Free Camera active is additionally covered by the production host fixture, not a separately recorded native load with camera ON.

Equipment is view only. Quest/trade items, bottles, beans, keys, progression flags and arbitrary save writes are excluded. Pending inventory acknowledgement briefly holds Save/autosave/new travel; an uncertain result releases that temporary hold and keeps further edits read only until normal reload. No live-save backup or automatic retry/rollback is performed.

If native Fly recovery is refused while airborne or clipped, Fly/menu remain ON. Descend under Fly or, below a floor, use Fly+Noclip to ascend into clear space, restore collision and disable Fly. Savestate notification is unsupported; reload a normal save after restoration. Broader geometry, physical-controller/display, equipment/progression and long-session coverage remains disclosed follow-up.

## Install and support

Close the game, back up its save directory, extract `Hyrule-Cheat-Menu-v1.1.0.zip` into Lichii's **Open Mod Data Location** and enable the add-on. Replace the entire mod folder, including both ExeFS files. Preferences remain in `sdmc/Hyrule-Cheat-Menu/`. Full controls and compatibility are in [README.md](README.md). `Hyrule-Cheat-Menu-v1.1.0-source.zip` is the separate build/review source archive; SHA-256 sidecars accompany both archives.

The isolated emulator is stopped; all 15 copied save hashes, isolated preferences and baseline ExeFS module were restored to their pretest state. The owner installation/saves were untouched.

The project remains [fxbiidev/hyrule-cheat-menu](https://github.com/fxbiidev/hyrule-cheat-menu); v1.0.0 and older history/releases are preserved. Publication and downloaded assets are checked against the exact tested commit and archive sidecars. The supplied registry MIT notice and existing rights are retained; no project-wide license is added. Proprietary inputs, saves and private fixture clients are excluded. Issues remain disabled. Discord support: `DISCORD_INVITE_URL_TO_BE_ADDED_BY_OWNER`.
