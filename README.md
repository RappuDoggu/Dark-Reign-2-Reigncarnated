# Reigncarnated

**Dark Reign 2 preservation and native Android source-port research.**

## Objective
Produce a **native ARM64 Android** build of Dark Reign 2 from recoverable C++ source, preserving original gameplay while adapting rendering, platform services and controls. Target AYN Thor dual-screen tactical UI **after** a stable single-display port.

## Status
**Phase 0 — source and licensing audit.** No Android gameplay, successful engine compilation or APK has been demonstrated yet.

## Existing reference
- Source repository: https://github.com/grasmanek94/darkreign2 (master)
- Original game is confirmed by project owner to run under Winlator on Android. That establishes a behavioral reference, **not** native Android compatibility.
- Public source availability does not necessarily grant permission to redistribute source, third-party libraries or game assets. Do not copy upstream source or bundled binaries here until provenance and license rights are clarified.

## Planned milestones
1. Inventory C++ source, project files, third-party dependencies and provenance.
2. Reproduce an original Windows build or isolate a small, independently buildable subsystem.
3. Identify/untangle Win32, Direct3D and legacy compiler dependencies.
4. Add a portable platform abstraction and ARM64 Android toolchain; validate with automated CI.
5. Bring up engine initialization and rendering on one Android display.
6. Load user-provided game data and play one mission with gamepad/touch support.
7. Investigate AYN Thor secondary-display APIs; implement tactical controls on the lower screen.

## Architecture notes
See [docs/PORTING_AUDIT.md](docs/PORTING_AUDIT.md). Avoid reimplementing gameplay unless a source component is genuinely unavailable.

## Assets
The repository should not contain proprietary game data. The eventual app should load files obtained from the user's legally owned copy, subject to rights and compatibility review.
