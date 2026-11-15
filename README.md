<div align="center">

# HYRULE CHEAT MENU

**A controller-operated cheat menu and integrated Fast Travel for Ocarina of Time remake.**

`v1.0.0` · `Lichii Build 177` · `Displayed update 2.0.0` · `Windows`

**Created by fxbii.dev**

[Features](#features) · [Installation](#installation) · [Controls](#controls) · [Support](#support--bug-reports)

</div>

**Hyrule Cheat Menu v1.0.0** provides eight cheat controls and integrated Fast Travel for the exact target below. The release UI shows **V1.0.0**, with runtime signature and availability checks. See [release readiness](RELEASE_READINESS.md) and the [test report](TEST_REPORT.md) for the practical release decision, exact binary evidence and disclosed limitations.

## What it is

Hyrule Cheat Menu runs inside the game as an ExeFS add-on. Press **R3** to open a centered green-and-gold panel, change cheats, or select a travel destination. The game uses its native gameplay pause while the menu is open. The mod adds no persistent HUD, telemetry, updater, or external trainer.

## Features

| Feature | Behavior |
|---|---|
| God Mode | Restores unlocked hearts and guards the native health decrement while keeping hit reactions. |
| Infinite Rupees | Fills the unlocked wallet and maintains its current capacity. |
| Infinite Items | Refills eligible owned arrows, bombs, Bombchus, and Deku ammunition; guards standard single-ammo use with age and capacity checks. Excludes bottles, beans, keys, and quest items. |
| Infinite Magic | Refills an existing meter. Unavailable before magic is unlocked. |
| Movement Speed | 1× / 1.5× / 2× / 3× for ordinary walking, running, swimming, and climbing requests. |
| Fly Mode | Camera-relative horizontal movement, vertical controls, and hovering through Link's native collision controller. |
| Flight Speed | Independent 1× / 1.5× / 2× / 3× / 5× / 10× setting. |
| Noclip | Grounded wall bypass with native floor support; combine with Fly for walls, floors and ceilings. Conservative safe-position recovery. |
| Fast Travel | All 214 reference destinations in 37 collapsible groups, including dungeon subgroups, concise entrance labels, age selection, confirmation, Favorites and Return. |

Supported features activate directly. Unavailable features remain readable but cannot be activated. Each cheat can be disabled independently. Grounded Noclip uses the same normal multiplier once, including A-held movement; flight keeps its separate speed.

The historical arrival sweep records **211 smoke passes across RC3 builds**, **two blocked river exits**, and **a later membership-qualified trainer arrival on the earlier integrated build**. These checks do not verify the changed development module or every progression state. See the [per-destination evidence](docs/destination-coverage.csv).

## Screenshots

<p align="center">
  <img src="docs/screenshots/release-cheats.png" width="49%" alt="v1.0.0 Cheats tab">
  <img src="docs/screenshots/release-travel.png" width="49%" alt="v1.0.0 Fast Travel tab">
</p>

Genuine release captures retain the green/gold style, English controls, golden pointer and exact credit. Current landing/recovery evidence is recorded in [TEST_REPORT.md](TEST_REPORT.md) and [landing results](docs/fly-landing-verification.json). Earlier six-speed, 100-cycle and destination measurements retain their original binary provenance.

<details>
<summary>Landing and earlier Build 177 Noclip captures</summary>

![Release Fly OFF landing](docs/screenshots/release-landing.png)

![Fly with Noclip below the tested Kakariko floor](docs/screenshots/build177-noclip.png)

![Independent grounded Noclip inside the ranch fence, without Fly](docs/screenshots/build177-grounded-noclip.png)

</details>

<details>
<summary>Historical regions, flight, Noclip and pause captures</summary>

![Compact region overview](docs/screenshots/regions.png)

![Noclip active above a tested ceiling in the Deku Tree](docs/screenshots/noclip.png)

![Active diving-game timer paused by the menu](docs/screenshots/pause.png)

![Fly Mode hovering above Kakariko](docs/screenshots/flight.png)

![Underwater flight in Zora's Fountain](docs/screenshots/underwater.png)

</details>

The captures inside the historical section retain their original provenance: regions, ceiling Noclip and diving pause show RC3 revisions; flight and underwater show RC2. Earlier RC5 Cheats and RC3 Fast Travel captures also remain in the source archive. They are not new Build 177 validation. Screenshots demonstrate only the pictured state; see the test report for exact coverage.

## Compatibility

| Component | Target |
|---|---|
| Current emulator target | Lichii Build 177 on Windows |
| Current library update label | 2.0.0 |
| Title ID | `0400486024DEC000` |
| Main Build ID | `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348` |
| Pinned compressed main SHA-256 | `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2` |

The owner's current game container displays update **2.0.0**, but its title1 main executable is byte-identical to the private executable used for the earlier 1.1.1-profile testing. Its Build ID is unchanged, so this exact executable needs no hook-address port. This is an identity check, not a claim that every update labelled 2.0.0 is compatible or that all gameplay tests have been rerun on Build 177. Earlier gameplay evidence used Build 130; consult the current test report for subsequent results.

The IPS filename gates loading by Build ID. Runtime title/layout and function signatures guard activation, with feature-specific failures. Other executables, regions, forks, and arbitrary hook mods are unverified. An existing mod using `subsdk8` conflicts with this installation. Historical built-in FPS Unlock and Dynamic FPS checks do not establish universal mod compatibility.

## Installation

1. Close the game. Back up your saves using the procedure below.
2. In Lichii, right-click the supported game and choose **Open Mod Data Location**.
3. Extract `Hyrule-Cheat-Menu-v1.0.0.zip` there. Replace the complete mod folder when updating, including both ExeFS files. The result must be:

```text
<game's mod data location>/
└── Hyrule-Cheat-Menu/
    ├── exefs/
    │   ├── subsdk8
    │   └── E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348.ips
    ├── third_party/
    │   └── Fast-Travel-LICENSE.txt
    └── README.md
```

4. Enable the add-on in the game's properties if necessary, start the game, and load a save.
5. During ordinary gameplay, press **R3**. No separate executable or runtime service is required.

Avoid an extra nested `Hyrule-Cheat-Menu/Hyrule-Cheat-Menu` folder. Disable a separate Fast Travel overlay during integrated-travel testing so its controls cannot interfere.

The optional `Hyrule-Cheat-Menu-v1.0.0-source.zip` is the review/build source archive. Install the add-on ZIP above. Its sibling `.sha256` file records the exact archive checksum.

### Updates and removal

Close the game and replace the installed `Hyrule-Cheat-Menu` folder. Preferences live separately on the emulator's SD card and survive replacement. Remove that mod folder to uninstall; base game files are not patched on disk. Optionally remove `sdmc/Hyrule-Cheat-Menu/` under Lichii's user directory to erase preferences as well.

## Controls

Labels refer to logical game inputs after Lichii's configured controller mapping; physical button labels can differ.

| Input | Action |
|---|---|
| R3 | Open / close; reverses the current transition |
| D-pad Up / Down | Select; wraps and repeats when held |
| A / confirm | Toggle, expand, select, or confirm |
| B / back | Cancel confirmation; collapse the current region or its parent; close from a collapsed root header |
| D-pad Left / Right | Adjust speed once per press; on Fast Travel, collapse / open a region |
| LB / RB | Switch CHEATS / FAST TRAVEL |
| X on Fast Travel | Select Young / Adult destinations |
| Y on a destination | Add / remove Favorite |
| Left stick while flying | Horizontal flight relative to the camera |
| Right stick while flying | Normal camera controls |
| RT / logical ZR | Ascend |
| LT / logical ZL | Descend |

The mod reads the game's Player 1 input, with handheld fallback. Navigation inputs are consumed while the menu is open. Flight consumes the left stick and altitude triggers; other gameplay inputs retain their native behavior. The earlier standalone Build 130 test retained the menu through a focus roundtrip and a logical Player 1 disconnect/reconnect. That result is historical. Physical Xbox, PlayStation and Nintendo controllers still need acceptance testing.

## Fly Mode

Activation requires a dry, grounded recovery position captured in the current supported scene. Release flight controls to hover. Normal Movement Speed is temporarily suspended during flight. Solid geometry remains handled by the native controller; Fly Mode does not enable Noclip. Underwater control has been exercised at Zora's Fountain; flight alone does not prevent native underwater damage.

Turning Fly OFF releases Link at his current ordinary position to native gravity and collision, protecting one landing from native fall damage. It does not rewind an ordinary descent to the takeoff point. Finite positions within 80 metres above or below the captured safe ground are eligible; God Mode permits higher finite descents. This landing guard does not provide enemy, fire or other general damage immunity.

**Extreme or below-floor recovery:** invalid/extreme positions and known below-floor Noclip history request checked-ground recovery. The game can refuse that native event while airborne. Fly then remains visibly ON with the menu open and zero hover drift; descend under Fly to safe ground. Below a floor, keep Fly on, enable Noclip, ascend into clear space, restore Noclip collision, then turn Fly off. Automatic anchor teleportation is not guaranteed in every native state.

Autosave requests stay held during flight, its protected descent, Noclip, recovery and travel. A missing landing after 30 seconds of unpaused play requests recovery or restores visible Fly/menu control. Menu pause does not expire landing protection. Rejected airborne travel retains the landing guard. Scene transitions and normal save loads clear movement state; Flight Speed remains a preference.

High-speed streaming boundaries, equipment-specific water states, animations and additional boss/minigame scenarios remain follow-up coverage. The verified native LoadingPause condition holds custom movement at zero and resumes the selected speed after loading. It does not predict unloaded geometry or guarantee arbitrary altitude safety. Native animations are retained without a dedicated flight animation override. Back up saves before using movement cheats.

## Noclip

**Fly OFF:** Link bypasses supported walls using native requested movement while the game retains floor collision and gravity. Airborne and swimming motion retain native handling. If floor support disappears after a wall bypass, a conservative guard returns to the last clear point; release the stick or turn away before continuing. This does not establish support for every obstacle or terrain boundary.

**Fly ON:** flight controls position through supported walls, floors, ceilings and underwater geometry. Release controls to hover. Noclip remains independent of the selected normal and flight speeds.

**Turning Noclip OFF:** the menu closes so Link's movement job can check his full native collision shape. A bounded search within two metres finds a checked nearby position; walking recovery also requires upward-facing floor support. Fly stays enabled when selected. If no nearby point is confirmed, flying recovery uses the last clear position; walking recovery reloads the same scene at the captured dry ground anchor. Failed or interrupted queries never authorize an unchecked position.

Fly and Noclip retain their pre-cheat ground anchors while either movement cheat owns the body; a lagging ground-contact flag cannot overwrite them during ascent or clipping. Normal scene changes clear movement state even before the new player is identified. Normal scene transitions and genuine save loads clear Noclip. Travel first restores and confirms the captured ground position, then clears Fly/Noclip and dispatches the native event. The pre-Noclip ground position becomes Return's source. No global world collision filters are changed.

Additional ceiling, water, stairs, equipment, high-speed and camera combinations are disclosed follow-up coverage; the selected wall/fence tests do not establish every map condition.

## Fast Travel

Open **FAST TRAVEL** with LB/RB. Start from a compact region overview. A opens a region or dungeon subgroup; indented rows show entrances without repeating the region name. Choose an entrance and press A. A separate confirmation shows the destination and selected age; press A again to travel or B to cancel. Travel begins after the menu releases its pause. Requests are not repeated by holding A. B returns to the parent region; R3 closes from anywhere. Counts, a scroll marker, and the full confirmation name keep long lists readable. Expanded groups, selection and viewport are remembered separately for each age during the session and restored when returning from CHEATS.

**X** switches between Young and Adult destination lists. A cross-age trip uses the game's native age-change event and intermediate Market scene. This path has separate verification requirements and can change in-memory story state; do not assume every progression combination is safe.

**Y** toggles a stable destination ID in Favorites immediately, with a small in-menu message. Favorites always appears first; an empty list stays compact. All 214 destinations can be favorited; the list is filtered by selected age and sorted alphabetically by area, then destination. Closing the menu flushes changed preferences. **Return to Previous Location** records a recognized source scene, grounded position, and age. It becomes unavailable after a save load. Flying from an unsafe source does not create a return point.

Normal swimming sources can travel too. Water positions do not replace a dry Return point: the previous safe Return is retained, or Return remains unavailable if none has been captured.

The Young and Adult Gerudo Valley river exits (`887aed4f`, `834153be`) immediately trigger another native scene exit in the tested save. Their original records and Favorites remain visible with **UNSUP**, an explanation, and activation blocked. **From the Training Ground** stays **LOCKED** until the existing Gerudo card, receipt and live fortress quest order match the native guard admission condition. Archive-only progress is insufficient. The mod only reads these conditions; it grants no card or quest progress. Some story entrances can start native cutscenes, dialogue or song lessons; finish those using the normal game controls before reopening the menu. Travel does not skip them or silently complete their progression flags.

**Castle door (enters castle)** and **Fortress gate (exits wasteland)** keep their original coordinates and recognize their verified native follow-on scenes. Their labels explicitly describe the resulting transition. Unexpected stable scene changes end the mod's pending transaction with an explanation, allowing the menu to reopen instead of waiting for the full loading timeout.

The four resource/health toggles and selected normal speed survive ordinary travel, with unsafe writes suspended during loading. Fly and Noclip deactivate. Actual save loads take precedence and reset all cheats OFF and effective Movement Speed to 1×.

## Preferences and saves

Preferences use a versioned checksum-protected pair of files, `preferences-a.bin` and `preferences-b.bin`, in `sdmc/Hyrule-Cheat-Menu/`. The inactive generation is written, flushed, and read back before being accepted. Verification compares both speed settings and every Favorite ID as well as the generation and checksum. A mismatched candidate is rejected and removed; the previous active file stays untouched. Failed saves remain pending for retry on a later menu close. Missing/corrupt files use safe defaults; unknown favorite IDs are discarded. Future schema versions are rejected safely. No older public schema needs migration yet.

Stored speeds never reactivate cheats on save load. A filesystem failure leaves Favorites usable for the session and exposes a status inside the menu.

Health, magic, normal speed, ammunition and custom-movement work are bound to the save generation that began the operation. If a normal save reload starts inside a native getter or before the next controller poll, stale work is rejected instead of applying the previous save's cheats or inventory evidence. Reliable savestate notification remains unsupported; load a normal save after restoring a savestate.

**Saving during movement cheats:** while Fly or Noclip is active, or a travel/recovery hold is pending, confirming the native Save screen follows Back/Cancel instead. Recover and turn movement cheats OFF before saving. Load and the other stock menus keep their normal controls. A short explanation is retained inside the cheat menu. This controller-path guard does not replace a backup.

**Manual backup:** with the game closed, use Lichii's **Open Save Data Location**, then copy that entire save directory to a separate dated folder. Restore only while the game is closed. Automatic copying of live saves is not implemented because a consistent snapshot has not been established. Native gameplay can save changed resources or progression even though the mod does not write save files directly.

## Troubleshooting and limitations

- **R3 does nothing:** check update/Build ID, add-on enablement, folder nesting, Player 1 mapping, and `subsdk8` conflicts. Dialogs, cutscenes, stock menus, and loading intentionally block opening.
- **Grey entry:** read its description. Magic requires an unlocked meter; Fly and Noclip need verified travel support and a safe ground anchor. Signature mismatches disable affected features.
- **Travel refused:** stand on stable ground or swim in a recognized scene and wait until ordinary gameplay resumes. Some underwater arrival states reject native events; swim away or surface before trying again. A timeout ends the mod's pending transaction; let native dialogue, cutscenes or loading finish before trying again. Reload a copied save if ordinary gameplay cannot resume. Native event refusals are reported without blindly repeating the request.
- **Depleted consumables:** empty nut and stick stacks use the corresponding owned bag. Bombchus use positive stock observed this session (including while Items is OFF) or the native saved guide-opened flag. A fresh zero-stock save refilled after prior native use; a new-game empty inventory stayed empty. Unknown zero-stock Bombchus remain unchanged, and a Bomb Bag alone never proves acquisition. Rental-item and broader progression cases still need coverage.
- **Wallet prompt:** filling the wallet can trigger the game's normal wallet-full Navi message.
- **Diagnostics:** search Lichii's local log for `[hyrule]`. The mod limits its own contribution to 64 KiB per process; the emulator controls the overall log's retention. Nothing is uploaded.
- **Coverage limits:** additional progression, damage/upgrade, boss/minigame/audio, peripheral and display combinations remain follow-up work unless a concrete serious failure is found. Other executables/forks are unverified. Savestate notifications are unsupported; reload a normal save after restoring one.
- **Memory and sound:** maximum-size display staging uses about 81.4 MiB. Menu sounds and vibration are not implemented; gameplay audio remains under the native pause profile and needs broader checks.

## Building from source

Python 3.9+ and Zig 0.17.1 are required. The tested compiler distribution is `ziglang==0.17.1`.

```powershell
python build.py --zig "C:\path\to\zig.exe"
python tests/run_tests.py --zig "C:\path\to\zig.exe"
```

To include the separate source archive and validate both distributions:

```powershell
python build.py --zig "C:\path\to\zig.exe" --source-archive
python tests/run_tests.py --zig "C:\path\to\zig.exe" --source-package "dist/Hyrule-Cheat-Menu-v1.0.0-source.zip"
```

The default add-on is `dist/Hyrule-Cheat-Menu-v1.0.0.zip`, with a SHA-256 sidecar. `--release-name Hyrule-Cheat-Menu-local.zip` selects a different local ZIP basename; pass its path through the test runner's `--package` option. Source ZIPs include maintained source, tests, documentation and CI; private inputs, saves, preferences, Git internals and compiled build products are excluded. Package checks reject duplicate or unexpected paths, stale install-folder files, checksum errors and mismatched source/document bytes.

An optional `--main "C:\private\main-dump.nso"` additionally checks the owner's uncompressed executable against the pinned profile. No game dump, ROM, key, or extracted asset is needed for the default build or CI. Ordinary CI compiles/tests code and archives without running gameplay. A separate publication job runs only after a successful build from a push to `main` with the exact commit message `Release v1.0.0`; it publishes the tested artifacts and refuses an existing release/tag.

## Support & Bug Reports

Project repository: [fxbiidev/hyrule-cheat-menu](https://github.com/fxbiidev/hyrule-cheat-menu). Older tags and releases retain their original provenance.

Discord support will be added by the owner:

`DISCORD_INVITE_URL_TO_BE_ADDED_BY_OWNER`

Include the mod version, emulator build, game update, other enabled mods, reproduction steps, and a relevant local log excerpt. Do not submit game files, keys, or saves containing personal information. GitHub Issues are disabled; support uses the owner's Discord link once supplied.

## Credits and rights

**Created by fxbii.dev.** Original runtime integration, menu graphics, and bitmap lettering are included without extracted game assets. No additional general open-source license is granted for the owner's original code.

The destination registry was recovered from the supplied Fast Travel reference attributed to Clay Mullis; its [MIT notice](third_party/Fast-Travel-LICENSE.txt) is retained. That reference's compiled module and exlaunch code are not included or linked. Native integrations were written independently. Nintendo and The Legend of Zelda belong to their respective owners; this project is an unofficial mod.
