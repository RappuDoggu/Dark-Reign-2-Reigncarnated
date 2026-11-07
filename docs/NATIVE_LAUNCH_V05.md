# Native launch APK v0.5

## Download

- [Reigncarnated-Native-Launch-v05.apk](https://chatgpt.com/library?file_id=libfile_5f648094538881919a2f673a676bc6d2&account_id=personal)
- [Decoder source and relink kit](https://chatgpt.com/library?file_id=libfile_4a6e42e50cfc8191bb150cc0789b9863&account_id=personal)
- [Existing companion game-data ZIP](https://chatgpt.com/library?file_id=libfile_3c8c19944d0c8191a7f5290d28569740&account_id=personal)

Private owner links. APK contains no proprietary game data. No new data ZIP is
required; existing imported v0.4 data is sufficient. Package remains
org.reigncarnated.menu.v02, versionCode 5, ARM64, minimum API 26, target API 35.
The recovered v0.2-v0.4 development signing key is reused for an in-place update.

## What changed

Original intro movies/audio, sky/clouds, lighting/fog, rain and ambient sound,
sun/moon sprites, animated water, rain impacts, shell smoke/flame/glow/drop
emitters, lightning/thunder and billboard playback. Launch pause checkpoints
preserve intro or shell progress while the Activity survives. Menu destinations
remain disabled. See LAUNCH_PROGRESS.md for the remaining parity limitations.

## Validation

- 65 host tests pass; 13 ZIP checks including actual data; 28 folder fixture checks.
- All three intros decode fully and restore matching frames/subsequent PCM.
- Seven scene/UI frames match across two recreated software GLES contexts.
- 19 emitter layers, 23 water clusters, ground/water splats and thunder exercised.
- Java-imported companion ZIP renders byte-identically to the source layout.
- ARM64 JNI, Java, DEX packaging, APK ZIP checks, v2/v3 signing, 16 KB ZIP alignment
  and 0x4000 ELF LOAD alignment verified.
- Decoder source/relink kit reproduces libreign_menu.so byte for byte.
- Final rendered menu inspected; fixed an opaque-roof transparency regression.

No phone or emulator was connected. This is a host-tested APK, not a claim of
successful Android execution or 1:1 desktop parity. On the Fold 5, install over
v0.4, launch using existing imported data (or select the existing ZIP), check the
three intros and their sound, then the native menu. Tap skips one intro. Check
Home/return once during an intro and once at the menu; use Report / data to share
any error. Process destruction starts a new launch and menu music restarts on
return. The menu remains a launch proof of concept, not gameplay.

## Reproduce

Use tools/install_android_sdk.sh, tools/build_bink_decoder.sh and
 tools/package_menu.sh with the documented external dependency paths. Existing
matching CMake configurations can use REIGN_REUSE_CONFIG=1; the package script
checks the cached ABI, platform, menu target, upstream, decoder and toolchain.
Then run tools/package_relink.py BUILD FFMPEG_ROOT NDK OUTPUT.zip. Keep keys and
owned assets outside the repository.

SHA-256:

- APK: 1d0fa253b52d5d7200caa3d356a1f6b58f9d2fa7084f1cd61940581ef04bc598
- Relink ZIP: 9131c9727fe0f4ef6d0eb505c60c431a412054d4c3b86f217ea734648105a490
