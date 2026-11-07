# Uploaded community installation — 2026-10-24

## Input and provenance

The owner supplied `Dark Reign 2.7z` as a community-updated installation. It was extracted into a separate temporary directory, never into versioned source. Native 7-Zip completed extraction successfully: 50 directories, 496 files, 469,820,640 uncompressed bytes. The Python extractor initially stopped at unsupported BCJ2; native 7-Zip supports it. This was an extraction-tool limitation, not evidence of corruption.

`installation-inventory.json` contains input/archive/executable SHA-256 hashes, pack sizes/counts and selected PE metadata. No game files, player saves, raw configurations, user names or serials are included. Player-profile/save configuration contents were excluded from inspection.

| Component | Observed metadata | Consequence |
|---|---|---|
| dr2.exe | PE i386 (32-bit x86), FileVersion `1,458`, embedded build date `09/08/20` | Community-era Windows baseline; exact correspondence with the reviewed source revision is unproven |
| DDraw.dll / D3DImm.dll | dgVoodoo 2.64 | Windows graphics compatibility wrappers; not Android dependencies |
| mss32.dll | Miles 6.0f | Matches the source SDK version; requires a native audio replacement or separately licensed backend |
| binkw32.dll | Bink 1.0i | Matches source header version; Android decoder rights/implementation remain unresolved |
| startup.cfg | Original engine stream configuration structure; references base pack and dynamic streams | Structurally compatible with the inspected startup path |
| settings.cfg | Saved dgVoodoo device/mode identifiers | Existing Windows video preferences must not become native Android defaults |

The executable imports Win32, DDRAW, DINPUT8, Bink, Miles, Winsock and multimedia/version APIs, consistent with the source audit. The Windows executable and bundled installers were not executed. No claim is made that this installation exactly reproduces an original disc build or that all community changes have been identified.

## Verified data checks

- Python metadata inspector validates all **88 ZWP archives**, totaling **3,763 entries**.
- New C++ archive adapter independently opens all 88, validates bounded metadata/directory records, rejects duplicate CRC keys, and reads every payload: **389,882,093 bytes**.
- Each filename CRC matches the **original engine Crc::CalcStr**; uppercase lookups resolve the same entry for all 3,763 names.
- The **original TBuf implementation**, compiled from the external source checkout with a small compatibility prelude, tokenizes **1,165 archived text configs**, producing **2,005,790 tokens**.
- It also tokenizes **16 loose non-player configuration files** (1,291 tokens). The required startup file yields 152 tokens and contains the expected startup/game configuration scopes.
- No archive/table/read/CRC/tokenizer errors occurred in this run. Binary-config skip support exists in the inspection CLI, but this installation's archived configs were all text.

These checks establish archive access and lexical compatibility. They do not execute PTree directives, resolve all includes, construct the complete game configuration, validate all mesh/audio/image semantics, or run missions.

`packs/base.zwp` baseline for Android testing:

| Field | Expected result |
|---|---:|
| Archive file bytes | 99,711,663 |
| Entries | 2,074 |
| Payload bytes | 99,631,656 |
| Configuration files | 465 |
| GOD meshes | 334 |
| PIC/TGA/BMP textures | 567 |

SHA-256: `f28ea56cecea135ee52c00c5f63fab975dd882778b549fc814954f3cf7d02fe9`.

## Implemented continuation

- Original TBuf and original StrBuf header now compile on host and Android ARM64, with content hashes verified. No upstream implementation is copied into this repository or modified in place.
- The compatibility prelude supplies 32-bit Bool and original utility/calling-convention/error contracts. Errors become exceptions at this isolated diagnostic boundary rather than silently succeeding.
- New native Archive adapter reads the original format on demand through portable file streams. Its table-free filename CRC is an original implementation of the documented polynomial and is verified against the original engine for real names. This adapter is not yet wired into original FileSys or its stream priority rules.
- Original tokenizer adapter groups string tokens using the original ReadConstant behavior. It is not a replacement parser or simulation.
- `installation_probe` combines native reads, original CRC and original tokenizer for repeatable installation testing. `tools/audit_installation.py` produces metadata only (optional pefile module for PE versions/imports).
- A separate Android **asset-test APK** contains only original porting code and standard runtime libraries. It has a system file picker, bounded app-private temporary copy, native archive validation/payload reads and a shareable report. No original engine implementation, game data, Windows wrapper, or proprietary middleware is linked or bundled. It is eligible for diagnostic artifact publication independently of unresolved upstream-engine distribution rights.

## Result and next gate

No data inconsistency found here requires a disc reinstall at this stage. Keep the disc installation as a comparison source if later version/asset/mission discrepancies emerge.

The next runtime evidence needed is the Android asset-test report on the owner's Fold 5 or Thor. This tests the native file-picking/copy/JNI/archive path. It is **not** the original-engine startup or gameplay milestone. The original PTree/FScope/FileSys dependency slice and Vid/Bitmap rendering migration remain engineering work after this test; a successful asset probe alone does not validate them.

Reproduction:

```bash
python3 tools/audit_installation.py /path/to/installation --source-archive /path/to/input.7z --output metadata.json
cmake -S . -B build-host -DREIGN_UPSTREAM=/tmp/dr2-reviewed -DREIGN_SANITIZE=ON
cmake --build build-host
ctest --test-dir build-host -V
# This sandbox only: ASAN_OPTIONS=detect_leaks=0
build-host/installation_probe /path/to/installation
```

Actual local logs are in `docs/logs/install-slice/`. Proprietary input is required to reproduce the real-installation run and is never downloaded by CI. CI instead runs a small original synthetic installation fixture, plus original tokenizer/archive regression tests.
