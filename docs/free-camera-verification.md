# Free Camera — implementation and verification

## Scope

The implementation is for the pinned `E0F3CCFAF50065AE4EA01EB5AEB13B1A45DA3348`
main executable. It replaces only the current gameplay view's 3×4 matrix after
the game constructs it. It never moves Link, changes collision, edits the
camera's source eye/target/FOV, or writes inventory or save data.

The native world continues to run. Input isolation prevents camera controls
from controlling Link; ordinary gravity, platforms, enemies and existing
God/Fly/Noclip policies retain their own behavior.

## Native evidence and bridge

- `0x804E48C0` constructs camera views. The guarded inline site `0x804E5068`
  follows the final matrix store, with `x20` holding the view. The original
  `LDR X8,[X19,#0x20]` is replayed by the existing hook trampoline before native
  execution continues at `0x804E506C`.
- The manager chain is `[[0x84CA6BC0]]`, count at `+0x20`, camera array at
  `+0x28`, first camera's view at `+0x78`. The view must match vtable
  `0x84C18A88`. Pointer ranges, bounded camera count and matrix writability are
  checked before use.
- Native instructions at `0x804E4E58` write the double-precision eye/target
  fields; instructions at `0x804E4FE0` and through `0x804E5064` construct the
  matrix. These layouts and hook bytes are separately signature-guarded.
- Existing native camera-baseline evidence for the same executable identifies
  the manager, gameplay view/vtable and view matrix independently of the earlier
  flight camera matrix. Binary disassembly was rechecked for this implementation.
- A control-flow/register-liveness audit covered 310 reachable instructions from
  the inline join until the first ABI call or return. `x16`/`x17` are not read
  before being overwritten or invalidated by a call. The dispatch/resume target
  may therefore use `x16` without losing native state.
- The original assembly bridge preserves caller GPRs, all 32 complete vector
  registers, `NZCV`, `FPCR`, `FPSR`, stack and the original link register around
  the C helper. No external mod implementation was copied.

## Controls and restoration

Left stick translates, right stick rotates, logical ZR/RT rises and ZL/LT
descends. Translation is 8 units/s; rotation is 2.4 radians/s with a 15% stick
dead zone. Steps are capped at 50 ms, diagonal stick travel is normalized and
pitch is bounded near 80 degrees. Native menu/pause transitions freeze camera
integration. Position, axes and native source fields must remain finite and
bounded.

The root input integration consumes gameplay buttons and both sticks while the
camera is active. R3 still opens the menu; R3+B exits directly even with the menu
closed and is latched until the combination is released. Existing flight keeps
its own hover/recovery update with zero camera-control inputs. A separate
camera-input drain also gates native controls and the custom Fly input path
after camera exit, until the controls return to neutral. Normal Movement Speed
is suspended while the camera or its drain owns input.

The native life job may sleep while Link remains idle. An active camera therefore
retains its independently revalidated Link owner instead of depending on a
450-ms resource heartbeat. Activation captures the native GameData `SceneID`
using the existing checked text API/key `0x1D6189DA`. Retention requires the same
save epoch, safe world, matching SceneID, current camera view, fully mapped life
object and successful `is_link_life` validation. A replacement actor is still
rejected by the backend. Resource/inventory identification is not broadened.
R3 menu opening can use this validated active-camera owner even when the life
job is asleep; emergency exit remains independent of menu availability.

Likewise, a recent native pose observation is needed to activate, but ongoing
ownership validates the current view directly. A long menu pause or an in-progress
observation copy must not disable an already owned, valid view when play resumes.

Scene/view replacement, save epochs, native loading, cutscenes, travel, missing
Link or controller, and incompatible recovery disable the camera. Interruptions
are latched so finishing an event does not silently reactivate it. Save hooks
invalidate the atomic publication immediately.

Ordinary disable leaves the next newly constructed native matrix untouched.
The exact native `eye == target` branch skips matrix construction: this case
restores the last native matrix only if the currently identified gameplay view,
save epoch and still-owned matrix all match. Changed native values or a reused
pointer in a new save never authorize restoration. Reset never dereferences a
cached view pointer.

## Completed checks

- The production C backend's host fixture passes with warnings as errors and
  assertions explicitly retained at `-O2`. It covers all movement controls,
  pause freeze, dead zone/time bounds, 2,000 rotation/pitch steps, ordinary and
  degenerate restoration, other-view isolation, malformed pointers/count/vtable,
  nonfinite data, world/loading interruption, actor/view replacement, save
  epochs and explicit-reselection requirements. Additional regressions cover a
  60-second pause followed by input before a new view callback, an in-progress
  observation copy, and retained native failure reasons after deactivation.
- ARM64 compilation of the production C implementation and assembly bridge
  passes with warnings as errors.
- The actual ARM64 bridge was executed in Unicorn with a deliberately
  register-clobbering helper. All 32 full vector registers, expected outcomes
  for 30 GPRs, SP, original X30, NZCV, FPCR and FPSR matched. X16 is the audited
  scratch exclusion; X8 receives the replayed native load. The native load and
  resume branch were executed. This is an ABI fixture, not emulator acceptance.
- The combat bridge was independently exercised with the same transparency
  checks and its real LSE `LDADD`: queued damage 11 plus replacement 21 became
  32, and W8 received the prior queued value 11.

## Native development checkpoints

Native checks on the preceding integrated module
`cf17aea84ae33e314ea8a1171572b33e39650cb9aafb2b385d262fea04e45657`
demonstrated forward camera translation (about 15.96 units), right-stick rotation,
ZR ascent (about 5.368 units), ZL descent (about 5.288 units) and exactly unchanged
Link coordinates. R3+B restored the native view with matrix-derived eye error
about `2.09e-5`; the main menu reopened. Combined Fly+Noclip+Free Camera also
kept Link's coordinates exactly unchanged while camera controls were used.

That checkpoint exposed a separate input regression: continuing to hold a stick
after emergency exit reached the custom Fly path and moved Link about 3.439 units.
The corrected integration gates custom Fly and normal speed through the camera
input drain as well as consuming the native pad. The subsequent module
`557ef38bc4fa26cc8094a6b2e4c8ca8b379f00a31b6d27bd5290dee38490ffe9`
passed the following root-accepted native retests:

| Check | Measured result on `557ef38…` |
|---|---|
| Menu close/unpause | Free Camera remained ON and usable. |
| Forward translation | Camera moved about 15.656 units; all three Link coordinates remained exactly unchanged. |
| Right-stick rotation | Camera basis rotated; Link coordinates remained exactly unchanged. |
| ZR / ZL | Camera rose about 5.312 units and descended about 5.672 units; Link coordinates remained exactly unchanged. |
| R3+B exit | Camera cleared with the main menu closed; native matrix-derived eye error was about `1.12e-5`. |
| Menu reopening | R3 reopened the main menu after exit. |
| Fly + Noclip + Free Camera | Camera controls produced exactly zero Link displacement. |
| Continuously held stick after emergency exit | With Fly/Noclip still selected, Link displacement remained exactly zero until neutral. The earlier 3.439-unit leak was not reproduced. |
| Menu toggle OFF | Native matrix-derived eye error was about `1.71e-5`, with camera OFF. |
| Fast Travel while camera ON | Camera deactivated; native arrival in Adult Kakariko completed in about 16.672 seconds with Return stored. |

These checks do not cover normal-save reload, every cutscene, every map or other
controller/display configurations. A subsequent audit identified an additional
release-boundary case: centering sticks while still holding R3+B must not clear
the input drain before the exit combination is released. The integration now
requires the exit latch to be released before clearing that drain.

This exact case passed on module
`7ec57f4d212db1dac58de80b7dacd9492a5928db51d7540fd1774a8df28d32a2`,
whose complete 82,728-byte running RX matched SHA-256
`1011d3d0639737155446b1312634c6c6868f7e075cb7bb1b7bcdeef92693a120`.
One uninterrupted input sequence held R3+B with a forward stick for 8 frames,
centered the stick for 12 frames while keeping the combination held, then held
forward again for 30 frames before release. Link's three coordinates remained
exactly unchanged (zero tolerance). After explicit neutral, both drain and latch
were false, and a short ZR probe resumed Fly with a rise of about 3.093 units.

## Current integrated native acceptance

The final runtime checkpoint
`ab2a5a1e6a44b90ebb5698cbbb64c667bcf620addc40709dc9e7e930abe230d5`
was retested independently. All 82,728 running RX bytes matched SHA-256
`8d9592d6c54982efb573cb331000a7a8b33553a6fa10678d7eb2cd5f0b133922`.
The root accepted these measured cases on that exact binary:

- Camera survived menu unpause. Forward translation was about 28.968 units,
  ZR rise about 7.936 units and ZL descent about 7.736 units; right-stick rotation
  changed the camera basis. Link's three coordinates remained exactly unchanged
  through all four controls.
- R3+B exited with the menu closed and restored the native view with
  matrix-derived eye error about `3.55e-5`; the main menu reopened.
- Fly+Noclip+Free Camera and continuous held-stick emergency exit both retained
  exactly zero Link displacement. Disabling through the main menu restored the
  native view with error about `1.81e-5`.
- The uninterrupted held-combination/center/reapply-stick regression passed
  again with exactly zero Link displacement and zero tolerance. Explicit neutral
  cleared both latch and drain; subsequent ZR resumed Fly with about 6.101 units
  of ascent, with Free Camera OFF.
- Fast Travel automatically disabled Free Camera and arrived in Adult Kakariko
  in about 6.360 seconds with Return stored.
- A subsequent God-OFF Fly-OFF descent from about 24.935 units above the starting
  ground retained life 40 before/high/after landing. The final state had movement
  cheats OFF and no recovery/Return notice; measured horizontal descent drift
  was about 0.0405 units. This is one landing, not all terrain/heights.

These are new native results, not a promotion of the earlier checkpoints.
Sanitized measured records and separate binary identities are in
[native results](v1.1.0-native-results.json).

## Coverage bounds and artifact checks

Host and ABI fixtures do not prove gameplay/display compatibility. The
targeted native checks above passed on the named integrated binary. Other maps,
cutscenes, progression states, physical controllers and display configurations
remain unverified. Camera interruption by save-epoch change is covered by the
production host fixture; the recorded native normal-save reload activated
God/Fly/One Hit Kill, not Free Camera. Final build/packages and published assets
must preserve the tested binary identity. Final scope is recorded in TEST_REPORT.md.
