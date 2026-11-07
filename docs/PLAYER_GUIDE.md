# Reigncarnated player guide — 0.1

## Installation and owned data

Install the ARM64 APK on Android 8.0+ with a GLES3 GPU. For an update, install over
the existing IA City Test/Reigncarnated app. Do not uninstall or clear app data.
Keep your original game archives externally; Android app backup is disabled.

Import complete ZIP files or the original installation folder. Include `packs`,
`missions`, `worlds`, `sides`, `mods` and `library`, including `base.zwp`, the shell,
`worlds/sprawl.zwp`, JDA/Sprawler client packs and campaign group/mission files.
Music imports from the installation's music folder; a separate music ZIP can be
selected under **Report / data → Import music ZIP**. An incomplete installation
can open a menu while still lacking mission, sound or world resources.

Do not combine overlapping ZIPs with conflicting versions of the same paths.
The import validates content and retains the prior installation if a new import
fails. Export a private backup with **Report / data → Export complete game-data
ZIP**. That archive contains your game data: keep it private, separate from bug
reports and source releases. Music and other content are not supplied by the APK.

## Main menu and campaigns

**Instant Action** opens the original animated selection/setup. Choose map,
opponent/practice, difficulty and seed. **Resources** sets only your starting
stockpile: blank/map default preserves the original amount; 0–1,000,000 is allowed.
AI resources remain map-defined. Use map defaults for normal gameplay acceptance.

**Campaigns → Start JDA / Start Sprawler campaign** starts the original first entry
and follows normal transitions using the normal profile. It does not erase prior
campaign records. **Choose individual mission / cutscene (testing)** exposes all
declared imported sequence entries without unlock requirements. Each row labels
faction and mission/cutscene type. Missing data is shown with launch disabled.
Individual tests use separate progress and return to the menu on transitions.
Cinematics embedded in a mission remain part of that mission's script.

## Touch controls

| Action | Gesture/control |
| --- | --- |
| Select or issue the original left-click order | Single-finger tap |
| Select several units | Drag with one finger |
| Choose a building/build action | Tap and hold |
| Anchor a building preview | First tap while placing |
| Rotate the anchored preview | Further single taps; wait briefly for double-tap detection |
| Confirm placement | Double-tap |
| Reposition a preview | Drag |
| Move the map | Two-finger pan, with no pinch zoom or twist |
| Deselect/cancel placement | **Deselect** button |
| Manual rotation/camera arrows | Toolbar build-rotation/camera controls |
| Return to the main menu | Android Back |

Three-finger rotation and pinch zoom are not active controls in this version.
Controller customization is planned, not a supported 0.1 feature.

## Settings, audio and reports

Main-menu Configuration, in-game Options and the toolbar open the same Android
settings. Use Apply to save or Cancel to keep the previous values. Available
settings cover music/effects volume, brightness, map pan speed, keep-awake and
FPS/frame timing display. Supported base-game graphics are configured at high
quality; desktop display-driver/mode switching is not presented as functional.

Background music pauses during loading, original cinematics/movies and reserved
speech, then resumes subject to audio focus and volume. Missing dialogue should
be reported separately from working unit responses/combat audio.

For bugs, collect the all-in-one ZIP immediately afterward. Include version,
phone/Android, map/mission, campaign versus individual mode and repeatable steps.
The ZIP can retain an older failure with its own source/time receipt; distinguish
it from the current session. Avoid sharing full game archives with reports.

Full campaign completion, save/restore, multiplayer and remaining rendering
issues are tracked in the [roadmap](ROADMAP.md) and [release notes](RELEASE_0_1.md).
