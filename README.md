<div align="center">

# HYRULE CHEAT MENU

**A controller-operated cheat menu and integrated Fast Travel for Ocarina of Time remake.**

`v1.0.0-rc5` 

**Created by fxbii.dev**

[Features](#features) · [Installation](#installation) · [Controls](#controls) · [Support](#support--bug-reports)

</div>

> **Local review candidate.** Gameplay features are marked **EXP** while verification is incomplete. This is not the final v1.0.0 release. See [release readiness](RELEASE_READINESS.md) and the [test report](TEST_REPORT.md) for evidence and outstanding checks.

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

The first experimental activation asks for consent for the current game session. Unsupported features remain readable but cannot be activated. Noclip is implemented and remains EXP while geometry, transitions and equipment coverage are incomplete. Each cheat can be disabled independently. Grounded Noclip uses the same normal multiplier once, including A-held movement; flight keeps its separate speed.

The arrival sweep records **211 smoke passes across RC3 builds**, **two blocked river exits**, and **a later membership-qualified trainer arrival on the integrated production build**. These checks do not establish every progression state. See the [per-destination evidence](docs/destination-coverage.csv).

## Screenshots

<p align="center">
  <img src="docs/screenshots/cheats.png" width="49%" alt="Cheats tab">
  <img src="docs/screenshots/fast-travel.png" width="49%" alt="Fast Travel tab">
</p>

<details>
<summary>View regions, flight, Noclip and pause captures</summary>

![Compact region overview](docs/screenshots/regions.png)

![Noclip active above a tested ceiling in the Deku Tree](docs/screenshots/noclip.png)

![Active diving-game timer paused by the menu](docs/screenshots/pause.png)

![Fly Mode hovering above Kakariko](docs/screenshots/flight.png)

![Underwater flight in Zora's Fountain](docs/screenshots/underwater.png)

</details>

Genuine emulator captures. Cheats shows the refreshed RC5 integration with all four core toggles, selected normal 3x, Fly 10x and Noclip active. Regions and ceiling Noclip show RC3 development revisions; the diving pause image shows the tested RC3 baseline. The flight and underwater images retain their RC2 provenance. Screenshots demonstrate the pictured state, not all destination or display combinations. See the test report for exact coverage.

## Compatibility

| Component | Target |
|---|---|
| Emulator | Lichii Build 130 on Windows |
| Game update | 1.1.1 |
| Title ID | `0400486024DEC000` |
| Main Build ID | `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348` |

The IPS filename gates loading by Build ID. Runtime title/layout and function signatures guard activation, with feature-specific failures. Other builds, regions, forks, and arbitrary hook mods are unverified. An existing mod using `subsdk8` conflicts with this installation. Built-in FPS Unlock and Dynamic FPS coexistence and disabled controls have been exercised; that does not establish universal mod compatibility.

## Installation

1. Close the game. Back up your saves using the procedure below.
2. In Lichii, right-click the supported game and choose **Open Mod Data Location**.
3. Extract `Hyrule-Cheat-Menu-v1.0.0-rc5.zip` there. Replace the complete mod folder when updating, including both ExeFS files. The result must be:

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

The mod reads the game's Player 1 input, with handheld fallback. Navigation inputs are consumed while the menu is open. Flight consumes the left stick and altitude triggers; other gameplay inputs retain their native behavior. The visible standalone emulator retained the menu through a focus roundtrip and a logical Player 1 disconnect/reconnect; navigation and R3 worked afterward. Physical Xbox, PlayStation and Nintendo controllers still need acceptance testing.

## Fly Mode

Activation requires a dry, grounded recovery position captured in the current supported scene. Release flight controls to hover. Normal Movement Speed is temporarily suspended during flight. Solid geometry remains handled by the native controller; Fly Mode does not enable Noclip. Underwater control has been exercised at Zora's Fountain; flight alone does not prevent native underwater damage.

When Fly Mode is turned off in midair, this candidate uses a native scene reload to the last recorded safe grounded position. The menu closes for that recovery. It does not attempt an unverified downward raycast or blind landing. Autosave requests are withheld during flight, Noclip, safe recovery and travel, then normal behavior resumes. Fly Mode is cleared during loading, scripted unsafe states, travel, and save resets. Flight Speed is retained as a preference.

High-speed streaming boundaries, equipment-specific water states, animation transitions, boss/minigame use, and broad landing coverage remain release gates. The verified native LoadingPause condition temporarily holds custom movement at zero and resumes the selected speed after loading. This does not predict unloaded geometry ahead of Link or establish arbitrary altitude safety. Native collision, recognized scenes, bounded per-step displacement and finite recovery coordinates remain necessary. Existing native animations are retained without a dedicated flight animation override. See the test report before using this candidate on an important save.

## Noclip

**Fly OFF:** Link bypasses supported walls using native requested movement while the game retains floor collision and gravity. Airborne and swimming motion retain native handling. If floor support disappears after a wall bypass, a conservative guard returns to the last clear point; release the stick or turn away before continuing. This does not establish support for every obstacle or terrain boundary.

**Fly ON:** flight controls position through supported walls, floors, ceilings and underwater geometry. Release controls to hover. Noclip remains independent of the selected normal and flight speeds.

**Turning Noclip OFF:** the menu closes so Link's movement job can check his full native collision shape. A bounded search within two metres finds a checked nearby position; walking recovery also requires upward-facing floor support. Fly stays enabled when selected. If no nearby point is confirmed, flying recovery uses the last clear position; walking recovery reloads the same scene at the captured dry ground anchor. Failed or interrupted queries never authorize an unchecked position.

Fly and Noclip retain their pre-cheat ground anchors while either movement cheat owns the body; a lagging ground-contact flag cannot overwrite them during ascent or clipping. Normal scene changes clear movement state even before the new player is identified. Normal scene transitions and genuine save loads clear Noclip. Travel first restores and confirms the captured ground position, then clears Fly/Noclip and dispatches the native event. The pre-Noclip ground position becomes Return's source. No global world collision filters are changed.

Noclip remains **EXP**. Broad ceiling, water, stairs, equipment, high-speed and camera tests are release gates; a passing selected wall test is not universal collision verification.

## Fast Travel

Open **FAST TRAVEL** with LB/RB. Start from a compact region overview. A opens a region or dungeon subgroup; indented rows show entrances without repeating the region name. Choose an entrance and press A. A separate confirmation shows the destination and selected age; press A again to travel or B to cancel. Travel begins after the menu releases its pause. Requests are not repeated by holding A. B returns to the parent region; R3 closes from anywhere. Counts, a scroll marker, and the full confirmation name keep long lists readable. Expanded groups, selection and viewport are remembered separately for each age during the session and restored when returning from CHEATS.

**X** switches between Young and Adult destination lists. A cross-age trip uses the game's native age-change event and intermediate Market scene. This path has separate verification requirements and can change in-memory story state; do not assume every progression combination is safe.

**Y** toggles a stable destination ID in Favorites immediately, with a small in-menu message. Favorites always appears first; an empty list stays compact. All 214 destinations can be favorited; the list is filtered by selected age and sorted alphabetically by area, then destination. Closing the menu flushes changed preferences. **Return to Previous Location** records a recognized source scene, grounded position, and age. It becomes unavailable after a save load. Flying from an unsafe source does not create a return point.

Normal swimming sources can travel too. Water positions do not replace a dry Return point: the previous safe Return is retained, or Return remains unavailable if none has been captured.

The Young and Adult Gerudo Valley river exits (`887aed4f`, `834153be`) immediately trigger another native scene exit in the tested save. Their original records and Favorites remain visible with **UNSUP**, an explanation, and activation blocked. **From the Training Ground** stays **LOCKED** until the existing Gerudo card, receipt and live fortress quest order match the native guard admission condition. Archive-only progress is insufficient. The mod only reads these conditions; it grants no card or quest progress. Some story entrances can start native cutscenes, dialogue or song lessons; finish those using the normal game controls before reopening the menu. Travel does not skip them or silently complete their progression flags.

**Castle door (enters castle)** and **Fortress gate (exits wasteland)** keep their original coordinates and recognize their verified native follow-on scenes. Their labels explicitly describe the resulting transition. Unexpected stable scene changes end the mod's pending transaction with an explanation, allowing the menu to reopen instead of waiting for the full loading timeout.

The four resource/health toggles and selected normal speed survive ordinary travel, with unsafe writes suspended during loading. Fly and Noclip deactivate. Actual save loads take precedence and reset all cheats OFF and effective Movement Speed to 1×.

## Preferences and saves

Preferences use a versioned checksum-protected pair of files, `preferences-a.bin` and `preferences-b.bin`, in `sdmc/Hyrule-Cheat-Menu/`. The inactive generation is written, flushed, and read back before being accepted. A previous valid generation survives a failed write. Missing/corrupt files use safe defaults; unknown favorite IDs are discarded. Future schema versions are rejected safely. No older public schema needs migration yet.

Stored speeds never reactivate cheats on save load. A filesystem failure leaves Favorites usable for the session and exposes a status inside the menu.

**Saving during movement cheats:** while Fly or Noclip is active, or a travel/recovery hold is pending, confirming the native Save screen follows Back/Cancel instead. Recover and turn movement cheats OFF before saving. Load and the other stock menus keep their normal controls. A short explanation is retained inside the cheat menu. This controller-path guard does not replace a backup.

**Manual backup:** with the game closed, use Lichii's **Open Save Data Location**, then copy that entire save directory to a separate dated folder. Restore only while the game is closed. Automatic copying of live saves is not implemented because a consistent snapshot has not been established. Native gameplay can save changed resources or progression even though the mod does not write save files directly.

## Troubleshooting and limitations

- **R3 does nothing:** check update/Build ID, add-on enablement, folder nesting, Player 1 mapping, and `subsdk8` conflicts. Dialogs, cutscenes, stock menus, and loading intentionally block opening.
- **Grey entry:** read its description. Magic requires an unlocked meter; Fly and Noclip need verified travel support and a safe ground anchor. Signature mismatches disable affected features.
- **Travel refused:** stand on stable ground or swim in a recognized scene and wait until ordinary gameplay resumes. Some underwater arrival states reject native events; swim away or surface before trying again. A timeout ends the mod's pending transaction; let native dialogue, cutscenes or loading finish before trying again. Reload a copied save if ordinary gameplay cannot resume. Native event refusals are reported without blindly repeating the request.
- **Depleted consumables:** empty nut and stick stacks use the corresponding owned bag. Bombchus use positive stock observed this session (including while Items is OFF) or the native saved guide-opened flag. A fresh zero-stock save refilled after prior native use; a new-game empty inventory stayed empty. Unknown zero-stock Bombchus remain unchanged, and a Bomb Bag alone never proves acquisition. Rental-item and broader progression cases still need coverage.
- **Wallet prompt:** filling the wallet can trigger the game's normal wallet-full Navi message.
- **Diagnostics:** search Lichii's local log for `[hyrule]`. The mod limits its own contribution to 64 KiB per process; the emulator controls the overall log's retention. Nothing is uploaded.
- **Verification gaps:** progression across destinations, every damage source/upgrade, the full boss/minigame/audio matrix, physical controllers, savestate restores, display modes and other forks are not yet fully verified. EXP labels remain until those gates pass.
- **Memory and sound:** maximum-size display staging uses about 81.4 MiB. Menu sounds and vibration are not implemented; gameplay audio remains under the native pause profile and needs broader checks.

## Building from source

Python 3.9+ and Zig 0.17.1 are required. The tested compiler distribution is `ziglang==0.17.1`.

```powershell
python build.py --zig "C:\path\to\zig.exe"
python tests/run_tests.py --zig "C:\path\to\zig.exe"
```

An optional `--main "C:\private\main-dump.nso"` additionally checks the owner's uncompressed executable against the verified profile. No game dump, ROM, key, or extracted asset is needed for the default build or prepared CI. The workflow compiles and tests menu, registry, Noclip state/movement contracts, storage and package behavior; it does not run gameplay tests or publish a release.

## Support & Bug Reports

Discord support will be added by the owner before publication:

`DISCORD_INVITE_URL_TO_BE_ADDED_BY_OWNER`

Include the mod version, emulator build, game update, other enabled mods, reproduction steps, and a relevant local log excerpt. Do not submit game files, keys, or saves containing personal information. GitHub Issues are intended to be disabled when the repository is created; this local project has no remote repository settings.

## Credits and rights

**Created by fxbii.dev.** Original runtime integration, menu graphics, and bitmap lettering are included without extracted game assets. No additional general open-source license is granted for the owner's original code.

The destination registry was recovered from the supplied Fast Travel reference attributed to Clay Mullis; its [MIT notice](third_party/Fast-Travel-LICENSE.txt) is retained. That reference's compiled module and exlaunch code are not included or linked. Native integrations were written independently. Nintendo and The Legend of Zelda belong to their respective owners; this project is an unofficial mod.
