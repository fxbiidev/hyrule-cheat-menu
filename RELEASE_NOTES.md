# Release notes — Hyrule Cheat Menu v1.0.0

**Created by fxbii.dev** · Windows · Lichii Build 177 · displayed update 2.0.0

Hyrule Cheat Menu v1.0.0 combines eight cheat controls, native menu pause and controller-operated Fast Travel in the original green/gold interface. The supported-target UI displays **V1.0.0** without experimental labels or consent. Runtime signature checks and feature availability checks remain active.

## Release changes

- **Fly OFF lands naturally:** ordinary midair deactivation releases Link at his current position to native gravity and collision. A one-shot actor/save-scoped guard protects that landing from native fall damage; other damage remains native.
- **Recovery refuses safely:** invalid/extreme positions and known below-floor Noclip history request checked-ground recovery. If the engine refuses the request, Fly/menu return with zero hover drift instead of releasing an unsafe fall. Same-scene Noclip anchors remain available for escape.
- **Save/reset safety:** generation checks fence movement, health, magic, speed, ammunition and travel work, including reloads inside native getters. Stale ownership evidence cannot be committed to the next save.
- **Preferences:** both speed settings and every Favorite ID are verified after writing. Checksum-valid wrong readback is rejected; failed writes remain pending for retry while the prior active file is retained.
- **Travel and navigation:** all 214 reference records remain in 37 collapsible groups, with concise labels, stable age/tab focus, alphabetically sorted persistent Favorites and Return. Two known unstable river exits remain visibly blocked; trainer access requires existing native progression.
- **Distribution:** installable and separate source ZIPs include checksum sidecars. Package checks cover membership, duplicates, deterministic metadata and exact source/document bytes. The prepared CI publishes only after successful validation from the deliberately named release commit.

## Target and verification

Supported main Build ID: `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`; title `0400486024DEC000`. The owner's displayed update 2.0.0 contains the same compressed main bytes as the earlier private 1.1.1-profile executable: SHA-256 `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`. Other executables, regions and forks are unverified.

Final module SHA-256: `d2a061f7b31071e7f536f64f5d3d1885ef2ffd9ed0130d422de850ce935d0772`. All 60,296 running RX bytes matched the built module. Final native checks passed ordinary God-OFF landing, six natural landing cycles, rejected-airborne-travel protection, safe refusal of extreme recovery and below-floor refusal followed by successful Fly/Noclip ascent and deactivation, retaining life 40. Cross-age Kakariko travel completed in 44.985 seconds and Return in 65.203 seconds with all three source coordinates restored exactly. Relevant host fixtures, ARM64 build and package/source checks pass. The release checklist covers final post-document archive reproduction and live CI/download verification.

Earlier 25/75/100-unit cases, the 100-cycle run, six-speed sweep and historical destination arrivals retain their own module provenance. The owner also reports extended play without observed bugs; no measured duration or exhaustive scenario list is inferred. [TEST_REPORT.md](TEST_REPORT.md), [release readiness](RELEASE_READINESS.md) and [landing evidence](docs/fly-landing-verification.json) distinguish these sources.

## Known limitations

The engine may refuse automatic recovery while airborne or clipped. Fly then stays visibly **ON**: descend under Fly to safe ground. Below a floor, enable Noclip with Fly, ascend into clear space, restore Noclip collision, then turn Fly off. Automatic anchor teleportation is not guaranteed in every native state.

Savestate notifications are unsupported; load a normal save after restoring one. Additional geometry, equipment, progression, audio, display/controller and long-session combinations remain disclosed P2 follow-up unless a concrete serious defect is found. These are not exhaustive release prerequisites.

## Install and support

Close the game, back up its save directory, then extract `Hyrule-Cheat-Menu-v1.0.0.zip` into Lichii's **Open Mod Data Location** and enable the add-on. Replace the entire mod folder when updating. Preferences remain separately in `sdmc/Hyrule-Cheat-Menu/`. Normal save loads reset all cheats OFF, effective Movement Speed to 1× and Return unavailable; stored settings never reactivate cheats. Full controls and troubleshooting are in [README.md](README.md).

`Hyrule-Cheat-Menu-v1.0.0-source.zip` contains maintained build/review source. The archives exclude game binaries/assets, keys, owner saves, reference compiled modules and private fixture clients. The supplied registry MIT notice is retained; no project-wide license is added to the owner's original code.

Publication is authorized for [fxbiidev/hyrule-cheat-menu](https://github.com/fxbiidev/hyrule-cheat-menu), preserving older tags/releases. Issues are disabled. Discord support: `DISCORD_INVITE_URL_TO_BE_ADDED_BY_OWNER`. This document describes the prepared release; live publication and downloadable checksums are verified separately after CI.
