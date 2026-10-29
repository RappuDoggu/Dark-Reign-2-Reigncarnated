# REA assessment and original-menu trace — 2026-10-25

## Decision

Pursue REA as an investigation accelerator, but continue route B (standalone Ghidra plus the available original C++ source) in this workspace. The installed REA 6.8.0/Ghidra 12.1.4/JDK21 toolchain is present; REA provider launch is not working. Do not spend further porting iterations repeatedly reinstalling dependencies that already work in standalone Ghidra. No replacement engine, wholesale decompilation, paid decompiler, or new asset upload is currently justified.

REA needs a Linux execution environment whose process IDs agree with its mounted procfs and whose procps can inspect child ownership. This workspace fails that prerequisite. The reproduced doctor output is `capability_unavailable`, with `fatal library error, lookup self`; providers/decompile fail before bridge startup. Namespace repair attempts were denied. See [actual output](logs/rea-workspace/rea-doctor.json) and [setup and troubleshooting](REA_SETUP.md). Do not disable ownership validation or reset existing analysis to hide this error. A healthy separate host is an alternative; it has not been connected or tested here.

## Research: what the evidence supports

- [Touhou maintainer's reconstruction account](https://rea.tools/blog/touhou-reconstruction/), dated October 10: reports continuation from substantial existing human reconstruction work on August 13, source for 1,107 identified functions by August 19, a playable Linux commit August 24, and a native 64-bit release August 30. This is meaningful evidence of rapid agent-assisted progress. It is a maintainer report, not our independent runtime benchmark. The same account documents incorrect floating constants that escaped an insufficient comparison oracle and required later corrections. It does not establish a general eleven-day schedule for unrelated engines.
- [Official DX-Ball case study](https://rea.tools/showcase/dx-ball/), October 7 checkpoint: 55 maintained functions, 45,380 original-code comparisons, 33 byte-matching functions. The project still described inspection tools/analysis libraries, with playable-game work outstanding. Its concrete example demonstrates why callers, instructions and raw constants must supplement pseudocode.
- [DAoC developer's Reddit post](https://www.reddit.com/r/daoc/comments/1x94kna/i_made_a_video_on_reverse_engineering_daoc_with/) links [their YouTube tutorial](https://www.youtube.com/watch?v=erRoMmb5Gkg). The author describes REA/Ghidra/Claude analysis of game.dll and a companion equipment UI; explicitly unfinished. This supports usefulness for focused client investigation, not a whole-engine platform-port timing claim. YouTube retrieval was throttled here; the video itself was not independently reviewed.
- [Official morluto/rea](https://github.com/morluto/rea) describes an agent/CLI bridge to native analysis providers. Use official morluto/rea, rea.tools and rea-agents npm sources; search also returned similarly named mirrors, which were not installed.

The earlier 2–6-week allowance was unmeasured and should not be treated as a validated forecast. Days are plausible for bounded investigations, and published projects show substantial short-timescale progress. We have no evidence supporting a days-to-DR2-menu promise. Re-estimate after a full original interactive control tree links and draws in an integrated startup loop. The key work here is reconciling original Win32/DirectX dependencies with existing ARM64 adapters; most original source is already available.

## New actual results

`DR2MenuProbe.java` ran through the installed standalone Ghidra against the existing DR2Startup project using `-readOnly -noanalysis`. All 13 requested functions decompiled. The six lifecycle entry points came from the previously checked WinMain registration ordering, not guessed symbols. Seven immediate callees were inspected next. The verifier checks the actual executable SHA256, language, complete output, recorded text lengths and six exact direct-call sequences. Five deliberately corrupted reports were rejected. This is structural evidence, not proof of functional equivalence or game execution.

Pinned executable SHA256: `5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250`.

| Address | Source-correlated interpretation | Observed evidence |
|---|---|---|
| 00545620 | Login::Init | Calls 00558d20 |
| 005456d0 | Shell::Init | Calls same initializer, then logged-in check; fatal path if false |
| 00545840 / 00545890 | Login/Shell::Process | Conditional call 005e7610, then 005e7160 |
| 00545510 / 005455a0 | Login/Shell::Done | Identical ordered three-call teardown |
| 00558d20 | Vid::InitResources | Material setup and conditional resource initialization match inspected source |
| 00548360 | User::LoggedIn | Tests a global pointer for nonzero |
| 005e7610 / 005e7160 | IFace::Process / PaintAll | Input/interface processing and render begin/clear/render/end/present path |
| 00550480 / 005e56c0 / 005572e0 | GameSound::Reset / IFace::DisposeAll / Vid::DisposeAll | Ordered teardown matches Base::Done source |

Names above are source-correlated inferences, not debug symbols or dynamic execution results. Optimized/inlined calls and inferred calling conventions make decompiler signatures unreliable. Call-reference lists do not capture every indirect call. No claim of exact source/binary equivalence is made.

Inspected original source: `game/gameruncodes.cpp` Base/Login/Shell; `graphics/vid.cpp` InitResources; `interface/iface.cpp` PaintAll and DefaultRenderProc, at upstream commit `9ce9da5f21604310a997f0c41e9cd383f5e292c3`.

## Consequences for the first menu

Shell initialization requires a valid user profile. Intro routes to Login when no user exists. The first-launch acceptance test must therefore include the original profile/login interface and the transition to Shell, not bypass the check with a fabricated user.

Normal Vid resource initialization creates default/black materials and initializes lights, mesh resources and the engine turtle texture. IFace painting calls rendering begin, clear, interface render, end and presentation; its default renderer draws the real root and overlay control trees. A cursor-atlas demonstration does not exercise this path.

Next implementation gates:

1. Resolve the full IControl/IFace shared declarations and dependencies (Pix, VertexTL, Input, Font and Utils); the current full translation-unit probe still fails. Coordinate/type slices alone do not construct controls.
2. Link real root/window/button controls with original configuration, fonts, event dispatch and GLES drawing. Exercise an original interactive window.
3. Integrate Main/GameGod startup, resources, profile/login and Shell configuration; keep original run-code transitions and rendering order.
4. Package that integrated path for ARM64, then ask for Fold testing of profile creation, Shell display, input, pause/resume and surface recreation.

No new menu APK was built in this analysis increment. Previous CI results remain the evidence for existing native component compilation; no new C++ build success is claimed.

## Reproduction and preservation

Run from the project repository, with the toolchain environment loaded and the existing project closed in any GUI. `NEW_PRIVATE_OUTPUT` must not exist. Keep all decompiler text outside the repository.

```sh
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" "$PRIVATE_PROJECT_DIR" DR2Startup \
  -process dr2.exe -noanalysis -readOnly \
  -scriptPath "$PWD/tools/ghidra" \
  -postScript DR2MenuProbe.java "$NEW_PRIVATE_OUTPUT"
python3 tools/verify_ghidra_menu.py "$NEW_PRIVATE_OUTPUT" "$OWNED_DR2_EXE"
```

The script refuses an unexpected executable hash/language and existing output directory. It writes final metadata only after all functions complete. Check the verifier, not just the headless launcher exit status. Existing project changes were discarded by Ghidra; its normal transient presave cleanup is visible in the private full log. No project reset/reimport was performed. Full pseudocode, executable and analysis database remain private; committed evidence contains addresses, hashes, call references and provider status only.

Evidence: [provider output](logs/menu-trace/provider-output.log), [verified metadata](logs/menu-trace/verified.json), [negative checks](logs/menu-trace/negative-checks.json).
