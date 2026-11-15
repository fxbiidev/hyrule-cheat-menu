# Changelog

## v1.0.0 — Full Release

- Replaced ordinary midair Fly OFF recovery with current-position native descent and one actor/save-scoped native fall-damage guard. Final-module ordinary God-OFF descent retained full health and retired its landing guard.
- Fixed a failed native recovery releasing Fly into an unsafe fall. Refused extreme/below-floor recovery now keeps zero hover and visible Fly/menu control. Noclip's same-scene anchor stays available so the player can ascend out of clipped geometry.
- Added a short settled-ground/water grace period, an unpaused landing watchdog and protection across rejected airborne travel. Reproduced and fixed a narrow actor-identification gap in pending recovery.
- Recorded the owner's report of extended play without observed bugs separately from instrumented tests. Release decisions now use practical P0/P1 defects and regression checks; broader unexercised combinations are disclosed P2 follow-up unless a concrete failure is found.
- Final supported-target UI shows `V1.0.0` without experimental labels or consent; signature and availability checks remain active. The owner authorized publication; repository Issues are disabled and existing history/tags/releases are preserved.

Final module: `d2a061f7b31071e7f536f64f5d3d1885ef2ffd9ed0130d422de850ce935d0772`. Exact native identity, scoped checks and earlier-build measurements are recorded in TEST_REPORT.md. The final artifact/document refresh and CI publication are separate from gameplay verification.

## v1.0.0 development — local owner-test build, 2026-11-15

- Changed the current target to Windows Lichii Build 177 with the owner's library displaying update 2.0.0. Verified the current title1 main is byte-identical to the earlier private executable (compressed SHA-256 `7db9ddfda088c22d219ef13496decd7077c68cd67d4b37f53f4ac46a082768f2`); Build ID and hook profile remain unchanged. This does not claim all 2.0.0 executables or emulator builds are compatible.
- Bound Fly/Noclip publications to the originating save generation, preventing stale custom movement from being reused between the native save hook and the next input poll.
- Added save-generation checks around health, magic, normal speed and ammunition operations, including resets inside native getters. Item ownership evidence is committed only while the originating save is still current.
- Verify preference readback against both speed values and every Favorite ID, rather than accepting a checksum-valid payload solely because its generation matches. Reject and remove a mismatched inactive candidate while retaining the previous active file.
- Keep failed preference writes pending for retry on a later menu close.
- Use the requested unnumbered `Hyrule-Cheat-Menu-v1.0.0.zip` for local owner testing; the menu shows `V1.0.0 DEV` and gameplay EXP labels remain. Added optional custom ZIP names and a separate source archive.
- Strengthened package checks for duplicate/unexpected paths, stale install-folder files, exact maintained source/document bytes, deterministic ZIP metadata and matching checksum sidecars. Updated the prepared CI artifact paths.

Targeted host regressions reproduce the stale-generation and storage-readback defects against the earlier code and pass with the fixes. Historical RC2–RC5 screenshots and gameplay evidence remain explicitly scoped to their original modules. This checkpoint predates the natural-descent Fly OFF change; see the current practical checks in RELEASE_READINESS.md. No final release or public publication of this revision is claimed.

## v1.0.0-rc5 — local review candidate, 2026-11-12–13

- Fixed A-held Noclip ground movement ignoring the selected normal multiplier. The bypass now applies the shared factor once without double-scaling ordinary walking.
- Retained pre-cheat ground anchors during flight/clipping/recovery, avoiding lagging contact flags capturing an airborne or embedded point. Scene changes now clear movement state before new player identification.
- Fixed wallet refill during ordinary purchase dialogue while retaining native transactions and save-load invalidation.
- Added native full-player-shape nearby recovery within two metres and fresh floor-ray acknowledgement for normal collision settling. Walking retains floor support; safe anchors remain the fallback.
- Added generation/owner fences and a verified native LoadingPause hold for custom movement.
- Fixed safe pause/resume identification gaps discarding Noclip's inactive anchor, allowing Noclip activation while already flying.
- Replaced the trainer's card-only lock with the actual live guard quest order plus existing card/receipt checks. No progression or inventory grants ship.
- Expanded real flight, travel/Return and 100-cycle recovery regressions; preserved exact build provenance. ZIP checksums now regenerate and validate automatically.
- Added signature-checked native consumption guards at ordinary ammunition-use calls. OFF, unknown flags, alternate inventories, other ages and non-unit removals retain native behavior.
- Traced normal Deku nut use through the shared inventory event and limited its extra guard to single nuts in safe gameplay. Broader quest/rental cases remain EXP.
- Shared the age/item whitelist between refill and consumption policies; added production fixtures for both ages, all six ammunition types, exclusions, flags, invalid pointers and transition/reset fallback.
- Used Wayfarer only as a technical lead, independently checked against the pinned executable. No Wayfarer source, graphics or fonts were imported. The existing camera transform was confirmed; predictive streaming remains unresolved; later full-player-shape queries were independently traced.

## v1.0.0-rc4 — local review candidate, 2026-11-12

- Remember existing Bombchu stock while Infinite Items is OFF, without granting or refilling anything until enabled.
- Recover depleted Bombchus through the signature-checked native saved guide getter, independently of positive stock this session. Unknown ownership remains conservative; no save flags are written.
- Added production policy regressions for OFF observation, empty saved acquisition, missing archive state, different-save isolation and never-unlock behavior.
- Real copied-save tests passed: native depletion to zero, paused refill, OFF retention, fresh-process zero-stock refill, new-game empty inventory, locked magic, new-game reset and ordinary load reset.
- Kept EXP labels and historical destination-test provenance. Streaming, rental/progression cases, physical devices and the remaining broad acceptance gates are not claimed complete.

## v1.0.0-rc3 — local review candidate, 2026-11-12

- Replaced broad flat travel lists with 37 collapsible groups, including dungeon and special-location subgroups. Preserved every byte of the 214 destination records.
- Added concise, distinct entrance labels, destination counts, disclosure arrows, nested indentation and a scroll marker.
- Preserved expansion, focus and viewport separately for Young/Adult lists and each tab. Back now collapses the current region or parent; R3 always closes.
- Fixed Favorite insertion selecting its duplicate in the Favorites list; immediate add/remove feedback preserves the selected region entry.
- Added independent scoped Noclip: grounded wall bypass, full bypass with Fly, conservative floor-loss recovery, safe deactivation and save/transition invalidation.
- Added swimming-source travel while retaining the last safe dry Return; unsafe airborne positions remain rejected.
- Corrected 10-bit panel opacity without reducing software coverage precision; added production renderer buffer/format tests.
- Added owned bag checks for empty Deku nuts/sticks and restored native stock-menu analog controls while Fly is active.
- Added acknowledged ground restoration before travel from Noclip, safe Return source capture and overlapping native autosave holds.
- Added a Save-screen-only Confirm-to-Back guard alongside autosave gating; it reads the verified native UI type and preserves Load/other stock inputs. Save-file regressions are recorded separately from compilation.
- Detect stable unexpected arrival scenes and release failed transactions promptly. Preserve verified castle-door and wasteland-gate follow-ons with explicit labels; retain both unsupported river-exit records without allowing activation.
- Lock the exact Training Ground exit until the existing Gerudo membership card is verified, preventing its native guard capture in an unqualified save. Ownership is read only; other destinations remain independent.
- Added exhaustive grouping/label tests and a native CCT fixture exercising the actual Noclip implementation. Expanded genuine emulator regression tests and release evidence.

Noclip and gameplay remain EXP. Final v1.0.0 is NO-GO while mandatory coverage remains incomplete. No public publication.

## v1.0.0-rc2 — local review candidate, 2026-11-11

- Added integrated Fast Travel with the reference registry's 214 destinations, five categories, Young/Adult selection, confirmation, Favorites, and Return.
- Added Fly Mode through Link's native collision calculation, camera-relative controls, six independent speeds, and native recovery to a captured grounded position.
- Added versioned preferences outside game saves, two validated generations, bounded local diagnostic output, and feature-specific compatibility checks.
- Expanded the original green/gold menu to two tabs and collapsible cheat sections; retained the original five cheats and native pause.
- Fixed a favorite insertion changing selection, oversized static graphics storage exhausting the Program heap, and flight clearing during post-pause actor identification.
- Fixed travel request underallocation corrupting a filesystem pointer. Added constructor-tail validation, guarded storage, and an optional test executing the real private-dump constructor.
- Fixed native buoyancy overriding underwater flight by applying scoped velocity after the native modifier job. Excluded submerged floors from safe recovery anchors using verified water-state layout and signatures.
- Added host failure tests, legal dump-free build workflow, and local owner-review documentation.

This candidate retains EXP status. See TEST_REPORT.md and RELEASE_READINESS.md before use. No public release has been published.

## v1.0.0-rc1 — preserved baseline

Original five-cheat menu, native pause, save-reset hooks, signature checks, and measured ordinary Movement Speed. Baseline evidence is summarized in TEST_REPORT.md and preserved in the RC2 safety snapshot; it does not automatically verify later paths.
