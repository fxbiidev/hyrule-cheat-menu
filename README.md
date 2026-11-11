[README.md](https://github.com/user-attachments/files/34635059/README.md)
<h1 align="center">Hyrule Cheat Menu</h1>

<p align="center">
  A standalone in-game cheat menu for<br>
  <strong>The Legend of Zelda: Ocarina of Time Remake</strong>
</p>

<p align="center">
  <kbd>v1.0.0 RC1</kbd>
</p>

<p align="center">
  <a href="#features">Features</a> ·
  <a href="#installation">Installation</a> ·
  <a href="#controls">Controls</a> ·
  <a href="#release-status">Release status</a>
</p>

---

Press **R3** during gameplay to open a translucent dark-green and gold panel over the game. The menu uses the game’s native pause system, works with controller input, and runs entirely inside Lichii without a separate trainer or an internet connection.

## Features

| Cheat | What it does |
| --- | --- |
| **God Mode** | Refills your existing hearts and protects against ordinary damage while preserving hit reactions. |
| **Infinite Rupees** | Refills your wallet to its current capacity. |
| **Infinite Items** | Refills eligible owned arrows, bombs, Bombchus, Deku Sticks, Deku Nuts, and Deku Seeds up to their current capacities. |
| **Infinite Magic** | Refills an unlocked magic meter and prevents ordinary magic costs. The option is greyed out until magic is unlocked. |
| **Movement Speed** | Switch between **1×**, **1.5×**, **2×**, and **3×** for ordinary movement. Tested while walking, running, surface swimming, and climbing a ladder. |

The four toggles and Movement Speed can be used together. Starting the game, loading a save, and choosing **Retry** after Game Over restore the default settings.

## Installation

1. Download `Hyrule-Cheat-Menu-v1.0.0-rc1.zip` from the repository’s **Releases** tab.
2. Close the game. In Lichii/Fritchii, right-click the game and select **Open Mod Data Location**.
3. Import the ZIP into the Addon Tab in Litchii/Fritchii.
4. Launch the game, load a save, and click the right stick (**R3**) during gameplay.

To **uninstall**, close the game and remove the `Hyrule-Cheat-Menu` folder from the mod location. To back up a save, close the game, open its save data location from Lichii, and copy that directory. Refills can carry over into subsequent game saves.

## Controls

| Input | Action |
| --- | --- |
| **R3** | Open or close the menu |
| **D-pad ↑ / ↓** | Select a cheat. Hold to repeat. |
| **Confirm (mapped A)** | Toggle the selected cheat |
| **Back (mapped B)** | Close the menu |
| **D-pad ← / →** | Adjust speed while **Movement Speed** is selected |

Confirm and Back follow Lichii’s configured Switch **A** and **B** mappings. The first EXP toggle requests confirmation once per game session. Speed changes take effect directly with Left or Right.

## Release status

> [!NOTE]
> **RC1 is a release candidate.** All five cheats carry the **EXP** label. Installation, removal, save-load resets, and a live bomb fuse pausing were tested on Windows with Lichii Build 130 and game version 1.1.1.

- Boss and minigame pause, Child Link transitions, wall climbing, physical controller behavior, and higher resolutions still need testing. Scripted scenes, stock menus, and some minigames can block R3.
- Savestate loads have no verified reset signal. Certain depleted Deku items and Bombchus may need to be observed with a positive count before the mod can refill them.
- Fast Travel and Lichii’s built-in FPS unlocker co-loaded in testing. Open one mod menu at a time and reserve `subsdk8` for Hyrule Cheat Menu.

See the [test report](PROGRESS.md) for movement measurements and the remaining checks.

<details>
<summary>Build and compatibility details</summary>

- **Title ID** `0400486024DEC000`
- **Executable Build ID** `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`
- **Build tools** Python 3.9+ and Zig

```text
python build.py --main <pristine-uncompressed-main-NSO-dump> --zig <path-to-zig.exe>
```

The build validates the original executable hash and hook signatures.

</details>

---

<p align="center"><sub>Created by fxbii.dev</sub></p>
