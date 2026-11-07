# Original configuration/runtime slice and GLES bring-up

## Implemented and verified locally

The source port now builds original PTree, FScope, VNode, StdParse, TBuf, BlockFile, NList/BinTree utilities, RunCodes, and Main::Exec/ProcessCmdScope (the latter two extracted into an external compilation unit from hash-verified maincmd.cpp). Original game implementations remain in a temporary external overlay. Only original bridge/tool/test code and source hashes/edit recipes are committed.

Original configuration behavior tested: enums, signed integers, hexadecimal/binary integers, floating-point/percentage values, quoted and empty strings, includes, nested scopes, optional config files and text/binary round trips. Tests reject missing includes, overwide integers and truncated binary containers. Live PTree object packing is removed for pointer alignment; disk BlockHeader/BlockInfo/BinaryInfo remain 44/8/12 bytes. Source failures and resolutions:

- Forced Windows file header: replaced at this isolated boundary by bounded DataFile, native File writer and scoped resource loader interfaces.
- Debug allocator header on BinTree: standard allocation used in the slice; no Win32 allocator/debug dependency.
- `-2147483648` sign multiplication: UBSan reproduced signed overflow. The overlay now accumulates unsigned 32-bit bits and reconstructs the signed result. Regression vectors include INT_MIN, high-bit hexadecimal and all-one 32-bit binary.
- Original fatal errors now throw at the bridge: session ownership and BlockFile unwind cleanup prevent open-file/double-exception failures. Numeric parser nodes gain scoped exception ownership. This is not a claim of complete parser fuzz hardening.
- String constants in a diagnostic lookup become const; variable diagnostics are bounded; malformed binary symbol lengths are checked.
- AtoF follows the inspected original conversion: FE_TONEAREST, double `atof`, then F32 conversion. Whole-simulation floating-point parity remains unproven.
- The original binary writer accepts function-scope trees, not raw root enum/variable definitions. The first unsuitable round-trip fixture exposed this existing limitation; supported round trips now use fully resolved function values. The parser's enum behavior is tested independently.

Resource sources preserve first-source-wins lookup, filename CRC matching, Windows separators and ASCII-insensitive physical-path discovery. Nested streams reject cycles; direct paths reject root escape. The bridge is currently read-oriented and excludes player-profile inputs; this is not the full original FileSys replacement or savegame service.

Original RunCodes tests execute initialization, optional post-init config callbacks, post-init callback, processing, CanLeave rejection/acceptance, run-once callbacks, Quit bypass, shutdown and cleanup. Main::Exec and ProcessCmdScope execute original code and dispatch to a required host callback; unsupported commands do not silently report success.

## Supplied J01 configuration evidence

Using the supplied installation, J01 mission archive, Sprawl world pack, base pack and client side packs, the original parser resolves includes and constructs real scopes:

| Input | Scopes | Arguments |
|---|---:|---:|
| startup.cfg | 25 | 33 |
| formation.cfg | 332 | 332 |
| types_messages.cfg | 709 | 523 |
| types_common.cfg | 68 | 57 |
| types_physics.cfg | 87 | 64 |
| types_traction.cfg | 99 | 176 |
| J01 game.cfg | 281 | 365 |
| J01 types.cfg include chain | 20,561 | 26,631 |
| J01 objects.cfg | 7,144 | 11,425 |
| J01 strategic.cfg | 2,180 | 2,205 |
| J01 mission_exec.cfg | 0 | 0 |

These are parsed data, not instantiated units, initialized AI or executed mission gameplay. The configuration probe explicitly mounts the supplied context; it is not a new game/config interpreter.

## GLES rendering boundary

The inspected Vid backend submits Direct3D7 pre-transformed/lit vertices and render states. The new original GLES3 adapter supports the corresponding initial 32-byte TL record, indexed triangle lists, screen coordinates with explicit pixel-center conversion, reciprocal homogeneous W, ARGB/BGRA color conversion, diffuse/textured color operations, alpha blending/test, zero-to-one depth mapping, specular RGB and specular-alpha fog. Unsupported vertex layouts, multistage state, fixed-function lighting, Bitmap surface/cache/device management and general Vid integration remain work. There is no permissive success stub for a complete D3D device.

Offscreen software EGL/GLES tests run real shaders, draws and readbacks. They pass color/half-pixel coverage, top-left texture orientation, source-alpha blending, alpha reference rejection, depth occlusion, vertex fog, specular addition, reciprocal W and invalid-index rejection. Contexts are destroyed/recreated and the checks repeated. These fixtures establish the implemented contract; they are not Windows capture parity or whole-engine rendering validation.

A native standard true-color TGA bridge supports 24/32-bit raw/RLE images and origin conversion, with corruption/bounds tests. The supplied `cursors.tga` is 128x128 RGBA. Native decoding matches Pillow byte-for-byte: 65,536 bytes, SHA-256 `c876a69feada6e14e77a7d70109773ec7fde650c691328642a51e2c32e7a497a`. This bridge does not replace the original Bitmap/PIC/mesh asset pipeline.

## Android deliverable and remaining limits

Renderer Test v0.3 contains original porting code only. It runs the GLES pixel fixtures in an actual Android surface and can preview the original cursor atlas loaded from the owner's selected base.zwp. It contains no original game implementation or proprietary asset. File selection/import and graphics preview cooperate across activity pause/resume; actual device verification is still needed.

The upstream-derived compilation targets build separately for ARM64. Full Main::CoreSystemInit/GameGod::Start, original UI, Vid/Bitmap integration, audio, input and simulation are not yet running. M3–M5 remain unachieved. Do not mistake the renderer test or parsed mission data for a playable APK.

## Reproduction

```bash
cmake -S . -B build-host -DREIGN_UPSTREAM=/tmp/dr2-reviewed -DREIGN_SANITIZE=ON
cmake --build build-host
ctest --test-dir build-host -V
build-host/configuration_probe /path/to/installation missions/jda/J01.zwp
python3 tools/stage_khronos_headers.py "$ANDROID_NDK_HOME" /tmp/reign-khronos
cmake -S . -B build-render-host -DREIGN_GLES_BACKEND=ON -DREIGN_KHRONOS_HEADERS=/tmp/reign-khronos
cmake --build build-render-host --target gles_smoke
LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build-render-host -R gles_legacy_pixels -V
bash tools/package_asset_probe.sh
```

Use `ASAN_OPTIONS=detect_leaks=0` only for the previously documented local sandbox limitation. CI retains normal leak checks. SDK headers are staged outside the repository. Portability edits are reproduced by tools/prepare_config_slice.py and validated by cmake/config-source-hashes.json. Logs: docs/logs/config-render-slice/.
