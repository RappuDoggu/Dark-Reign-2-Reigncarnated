# APK 25 diagnostic review: campaign cinematics

The owner confirms Instant Action combat sounds work on the phone. This is a
user-reported device result; no claim is made that every unit/audio asset was
exercised. The owner also reports Sprawler's introduction stays black with
in-game cinematic sounds, and JDA shows Chapter 1 before an error.

The private ZIP's main receipt matches APK 25 source
`7250200d75073ae0691bda0dbb5d5fc9259647af` and its exact native hashes. Its
nested previous-mission-last.zip is still an APK 24 collection. The current
last-load journal is therefore more useful for the newest JDA failure than the
older nested report. Private device identifiers, images and raw traces remain
outside public Git and the APK.

## Findings

The latest JDA launch reaches a real simulation cycle at about 263 ms. At
3,271 ms the journal records `Attempt to use an invalid VarPtr (not setup or
target has been deleted)`. The earlier empty-path failure is no longer the
recorded stop. The supplied current collection is from the menu; it has no
current mission caller stack naming the invalid variable. Its historical crash
traces also predate the latest launches. A root-cause fix for this JDA variable
error is not established by the archive.

The native DX7 adapter only accepted a full-target viewport and full-target
clear rectangles. The original cinematic Letterbox primitive changes the camera
to a smaller viewport and issues clears inside it. An authored timeline through
the real original cinematic path reproduces rejected Clear calls; the targeted
viewport API check also fails against the earlier implementation. This is a
confirmed adapter gap relevant to cutscenes. The report lacks a current Sprawler
mission screenshot/native snapshot, so fixing this gap is not proof that it is
the sole cause of the owner's black screen.

## APK 26 changes

The adapter stores and reports valid sub-viewports, clips pretransformed TL
screen coordinates with GLES scissoring, and clears clipped color/depth regions
without altering pixels outside the viewport. Partial clears and multiple clear
rectangles are supported. Invalid rectangles, unsupported stencil/depth-range
operations and other unsupported calls still return explicit failures and
record diagnostic sites. Full-screen rendering retains its existing coordinate
mapping, full-clear path and fenced append-only stream; there is no per-draw
GPU query or readback.

Before original cinematic command dispatch, the loading journal records command
name and timeline cycle. Invalid VarPtr access captures a bounded operation/
module-relative caller stack before unwinding, saved with native diagnostics.
It preserves the original exception and does not manufacture variable defaults
or swallow failed script commands. VarPtr and original engine object layouts do
not change. Native startup clears previous variable-failure evidence.

On caught frame failure and normal mission pause/exit, the app saves a bounded
native mission report with source receipt, mission, timestamp, camera/cinematic/
movie flags, audio and rendering diagnostics. The menu ZIP includes the latest
report and previous distinct mission report (128 KiB each), so testing Sprawler
then JDA can preserve both even without exporting from the mission screen.
An abrupt fault before a snapshot still relies on the persistent stage journal
and available OS traces. Older manual ZIPs retain their own source/time receipt.

## Validation and remaining checks

68 host tests and 12 engine foundation tests pass with ASan/UBSan and the
sandbox's established leak-detection setting. Real GLES pixels verify nonzero
viewport offsets, partial and multiple clear rectangles, preservation outside
the clear region, TL clipping and restoration to the full target. The original
Vid/GLES attachment/presentation/context lifecycle passes twice. An authored
cinematic timeline tests fade/letterbox pixels, nonblack scene pixels, shroud/
input/HUD restoration, end of timeline and invalid-variable stack capture. The
synthetic campaign and reference Instant Action load/simulation/pause/resume/
shutdown paths each pass twice. SDK 35 Java compilation and 48 touch checks pass.

These are source/host checks, not retail story playback or Android lifecycle
validation. Retest Sprawler's intro, return to menu, then JDA Chapter 1 and collect
one new menu diagnostics ZIP. If JDA stops again, its saved caller stack should
identify the real failing variable access. Combat audio is retained and phone
verified by the owner; new cinematic phone behavior, JDA fix and FPS measurement
remain acceptance checks.
