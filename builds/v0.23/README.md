# APK29: cutscene speech and music

Update to the existing **Reigncarnated IA City Test** app. Application ID
`org.reigncarnated.instantcity`, version 29 / `0.23-cutscene-audio`, same private
signer as APK22–28. Imported data is retained; no game data or key is included.

Source: `b2737565d21ec4529736a04b3a3ddc36261e0289` on
`recovery/native-arm64-v012`. Build instructions: `docs/RECOVERY_BUILD.md`;
progress: `docs/RECOVERY_PROGRESS.md`; receipt:
`docs/builds/cutscene-audio-v29.json`. Source revision, library hashes, instructions
and progress are also embedded in the APK's `assets/build/` directory.

Adds bounded native MP3 dialogue decoding and protected Reserved speech priority.
Pauses Android background music during loading, cutscenes, movies, speech and
campaign transitions; resumes the same track after these conditions clear.
Keeps the original simulation, ABI, touch controls and rendering behavior.

Validated on host: 68 general and 13 engine tests; original Reserved MP3 queue
under combat saturation; combat effect lifetime; original cinematic/raw-mesh and
campaign navigation twice; 55 touch, 13 report and 14 music policy checks.
APK signer, ARM64, 16 KiB ELF/ZIP alignment, embedded provenance and privacy checks
pass. Retail Sprawler opening dialogue and new performance still need phone
verification. Missing voice resources and unsupported compressed WAV are reported
explicitly. See the receipt for exact acceptance limits.

Please replay the Sprawler opening, then check music resumption and unit/combat
sounds. If speech remains absent, collect the all-in-one diagnostics ZIP after
the opening scene; voice resource names and failure counters are now included.
