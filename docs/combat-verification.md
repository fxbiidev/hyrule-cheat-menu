# Combat implementation and verification

This document records the v1.1.0 combat backend's verified code contract and
portable checks. The corrected native module passed all four multiplier settings,
One Hit Kill priority and stored 3× resumption on supported Tektites. Modified
packets had exact Link attribution. A source inspection or host fixture is not an
emulator test.

## Supported policy

One Hit Kill and Damage Multiplier modify only a positive, already accepted native
external sensor-hit damage operand whose checked attacker handle resolves to the
currently identified Link, and whose verified contact channel is 2. Channel 6 and
every other channel retain stock behavior. Link, friendly actors, unrelated objects, bosses,
minibosses and unknown targets are excluded. The exact ordinary-enemy allowlist is:

- Red and Blue Tektites (`Enemy_Tektite_Red`, `Enemy_Tektite_Blue`).
- Ordinary, Fire and Ice Keese (`Enemy_Keese`, `Enemy_Keese_Fire`, `Enemy_Keese_Ice`).
- Ordinary and large Stalchildren (`Enemy_Stalchild`, `Enemy_Stalchild_Big`).
- Octoroks (`Enemy_Octorok`).

These actor packs and their ordinary enemy components were found in the supported
local game data. Exact names exclude dictionary/display actors and additional
variants. Stalfos, Gold Skulltulas, invisible Keese and every unlisted enemy remain
stock; they are not silently treated as supported. Broader coverage requires new
evidence and an explicit allowlist change. Native gameplay checks below cover Red
and Blue Tektites; the other allowed names have resource and portable-policy
coverage, without a claim of separate native combat acceptance for each species.

The multiplier is 1×, 1.5×, 2× or 3×. Integer half points round upward consistently:
1 damage becomes 2 at 1.5×; 3 becomes 5. Each accepted native operand is transformed
once; the implementation does not rewrite reusable attack parameters or already
modified damage queues. One Hit Kill substitutes at least the target's current
positive life and takes priority over the multiplier. Disabling it immediately
resumes the published multiplier. Published 1× with One Hit Kill OFF is passthrough.

Native request caps remain enforced, even with One Hit Kill. Native life caps,
invulnerability, minimum-life handling, health updates and death/reward processing
remain in the original engine. A native cap may therefore prevent a lethal result;
the mod does not bypass it. Save changes and unsafe gameplay revoke authorization.

## Verified executable boundary

The implementation targets the exact executable identified in
[the design review](v1.1.0-design-review.md), with signatures checked against its
matching flat image.

| Evidence | Verified native behavior |
|---|---|
| `0x804A99FC`, `ldadd w8,w8,[x9]` | Adds the final accepted request amount to the external damage queue. Here `x20` is the external calculator, `x19` the request, and `x9` the calculator's amount field at +`0x18`. |
| `0x804A99D0`–`0x804A99FC` | Native queue suppression flags are checked before reaching the bridge. The bridge replaces only the incoming amount and replays the original atomic addition. |
| `0x804A87AC`–`0x804A87C8` | Request byte +`0x1C` selects its signed maximum at +`0x18`. The C policy reapplies this maximum to the transformed operand. |
| `0x804A8B08`–`0x804A8B38` | Request generation +`0x30` and handle +`0x28` resolve through matching handle generation +`0x48` to actor +`0x40`. Invalid handles and engine actor states fail closed. |
| `0x804A5230`, `0x804A9140`–`0x804A9164` | External calculator is Life +`0x210`; receiver is calculator +`8`, Life is receiver components +`0x2B8`, and current life is the storage pointer at Life +`0x1C10`, offset +`8`. |
| `0x804A77A8`, `0x804AB9C0` | Constructs contact channel 2 and processes native contact/sensor records. Actual Master Sword and Bow impacts confirmed this channel with a checked Link source. |
| `0x804AA8B0`–`0x804AA8DC`, `0x804AA310` | Native contact processing selects the record's source handle at +`0x60` or +`0x78`, checks its generation and copies it into request +`0x20`. The mod requires the resulting attacker to be Link; it does not infer ownership from a projectile name. |
| `0x804BDCCC` onward | Original final health clamps, atomic health update and subsequent life/death handling remain after external aggregation. |

The accepted-dispatch function contains no `x16`/`x17` register use, so the profile
dispatcher's scratch use is compatible with this bridge. The assembly bridge must
preserve the remaining registers, vectors and status registers around the C call.
`combat_damage` itself returns an operand and writes only bounded diagnostic state;
it never writes requests, actor health, inventory or save data.

Master Sword melee and an ordinary Bow arrow have actual source-attribution
evidence. Additional melee weapons, elemental arrows, reflected projectiles,
explosives and spells have no separate native acceptance claim. An unrelated
projectile or an unknown shooter receives no multiplier.

Diagnostic reason counters and a separate last checked Link-origin packet retain
source/target/cause evidence when later enemy hits overwrite the general last
packet. Recording a cause is not authorization to modify it.

## Completed checks

- Production C fixture compiled and passed with warnings treated as errors.
- All four factors, odd/even half-point rounding, One Hit Kill priority, immediate
  OFF/1× behavior and multiplier resumption passed.
- Every allowlisted name and negative cases for Link, NPCs, bosses, minibosses,
  dictionary actors, unsafe variants and unrelated sources passed.
- Invalid/stale handles, mismatched receiver/Life relationships, unsupported
  causes, nonpositive/extreme amounts, packet caps and queue overflow passed.
- Save epoch changes, unsafe world state and publication revocation passed.
  A fixture first reproduced an epoch change during the final safety check;
  rechecking the epoch after that call fixed it, and the fixture passed on retest.
- Fixtures compare the complete simulated game memory before and after the C
  policy, confirming no actor, request or health writes. Repeated equal native
  operands return equal results without stacking.
- Freestanding ARM64 compilation passed with warnings treated as errors.

## Native source-channel verification

The diagnostic module SHA-256 was
`7ec57f4d212db1dac58de80b7dacd9492a5928db51d7540fd1774a8df28d32a2`.
Its 82,728 running RX bytes were identified by SHA-256
`1011d3d0639737155446b1312634c6c6868f7e075cb7bb1b7bcdeef92693a120`.
These checks verified the correct channel and source while the earlier provisional
channel-6 policy still forwarded channel 2 unchanged. They do not claim that this
diagnostic module applied a multiplier.

| Native action | Recorded evidence |
|---|---|
| Master Sword hit using logical ZL + Y | The checked attacker was the current Link; the target was an allowlisted Red Tektite with life 2; native amount was 2 and contact channel was 2. The Tektite died through native handling. |
| Bow shot using ZL lock-on and a held/released ZR | Arrow stock decreased from 7 to 6. A second Red Tektite died; its accepted packet had native amount 2, channel 2 and the same checked Link attacker. The retained player channel-2 count advanced from 1 to 2, without another sword input. |

Later enemy hits were separated by the retained player packet and reason counters.
This exposed the initial channel-6 assumption: native impacts occurred, but the
multiplier was correctly refusing their unverified channel. The policy was then
restricted to the observed channel 2, with channel 6 and all other channels tested
as passthrough. The corrected production C fixtures passed on retest. All native
effect tests below must use the corrected module identity.

## Corrected native combat results

The corrected module SHA-256 is
`ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`.
All 82,728 loaded RX bytes matched SHA-256
`8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922`.
Each modified packet below used contact channel 2, an exact checked current-Link
source, an allowlisted target and native queue count zero before the addition.
The separate 1× comparison used before/after target life and the unchanged
modification counter. The game performed the resulting deaths through its
original life handling.

| Corrected native check | Result |
|---|---|
| 1.5× Master Sword against Red Tektite, life 2 | Native damage 2 became 3; the target died normally. |
| 2× Fairy Bow against Red Tektite, life 2 | Native arrow damage 2 became 4. |
| One Hit Kill ON, stored multiplier 3×, Fairy Bow against fresh Blue Tektite, life 3 | Native damage 2 became 3, exactly the target's current life. It did not become 6. The target died in one valid hit. |
| One Hit Kill OFF, stored multiplier still 3×, Fairy Bow against another fresh Blue Tektite, life 3 | The next valid arrow's native damage 2 became 6 without changing the multiplier. |
| Fresh 1× comparison after normal save load, One Hit Kill OFF | A landed ordinary Bow shot reduced fresh Blue Tektite life from 3 to 1 with multiplier 1×; the modification counter remained 4. Earlier missed shots left life at 3 and were not counted as damage checks. |

These checks establish the pictured/native damage and death cases, including a
distinct One Hit Kill result where ordinary damage 2 would leave a life-3 target
alive. They do not establish every weapon, enemy, drop/reward, progression state or
scripted encounter. Fractional rounding of odd native damage remains covered by
production fixtures rather than a claimed odd-damage native hit.

Accepted hits received by Link retained their native amount 2 and were rejected
by the combat policy as `UNSUPPORTED_TARGET`. A separate final-module check with
God Mode OFF and outgoing multiplier 3× reduced Link's actual life from 40 to 38;
the recorded received amount remained 2 and the modification counter remained 4.
This confirms that the selected multiplier did not multiply damage received by
Link in the tested native case.

## Coverage limits and follow-up

1. Friendly/unrelated actors, excluded bosses and scripted enemies have portable
   rejection fixtures. An excluded boss remains entirely stock by the exact
   allowlist policy; native boss-phase encounters are not implied by the Tektite
   results.
2. The 1× comparison followed a normal save load with damage 1× and One Hit Kill
   OFF. Broader scene/travel/save-load, resource/movement and assembly-preservation
   results belong to the main test report; the C policy fixtures alone do not
   establish those integration results.

The completed host checks do not substitute for the remaining native effect tests.
