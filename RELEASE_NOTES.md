# Hyrule Cheat Menu v1.1.1

Fixes Item Manager ownership editing while retaining the approved Emerald & Gold interface and existing controls.

- Add supported items that have never been acquired, including Goron Tunic; remove real owned weapons, shields, tunics and boots.
- Display OWNED / NOT OWNED and ADD ITEM / REMOVE ITEM, with a separate confirmation for every change.
- Adjust six consumable/ammunition stacks within native capacity without prior-acquisition gating. Empty stock preserves its bag/tool.
- Use native equipment cleanup, age checks, Hookshot/Longshot slot protection and Bow/Slingshot ammunition dependencies. Infinite Items cannot refill ammunition for a removed tool from cached ownership.
- Retain deferred acknowledgement, Save/travel holds, save-epoch fences and reload-required handling of uncertain native updates. Quest-critical items and arbitrary save writes remain excluded.

Install `Hyrule-Cheat-Menu-v1.1.1.zip` by replacing the complete existing mod folder while the game is closed. Back up saves first. Preferences remain compatible with v1.1.0. The optional source ZIP is for review/building.

Supported target: Windows Lichii Build 177, displayed update 2.0.0, title `0400486024DEC000`, exact main Build ID `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`. Other builds are unverified. See [test report](TEST_REPORT.md), [release readiness](RELEASE_READINESS.md) and [inventory verification](docs/inventory-verification.md) for actual verification and limitations.

Native checks verify Goron Tunic add/equip/remove and persistence of both ownership states through stock Save/Load, representative shield/boots/weapon edits, all six stacks across both ages, and Infinite Items/normal ammunition use. Portable fixtures cover all 20 IDs and existing feature regressions; 936 renderer cases pass. Final package/CI/download verification is a separate publication gate. Existing v1.1.0 and v1.0.0 releases, Git history, repository visibility and third-party notices are preserved. No game files, keys, saves or private QA clients are distributed.
