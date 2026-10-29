# Native BMP loading checkpoint — 2026-10-24

The bounded native image loader and original bitmap manager now support BMP, closing the format gap found in both base assets and mission previews. Original Pix/Bitmap::CopyLine remains the final pixel conversion. This is file framing plus the scoped manager, not the original complete Bitmap::ReadBMP/Create/DirectDraw/mipmap implementation.

## Inspected source and supported semantics

Primary reference: pinned graphics/bitmap.cpp, Bitmap::ReadBMP at line1183, already hashed by cmake/pixel-source-hashes.json and cmake/bitmap-manager-hashes.json. It reads a14-byte file header and40-byte info header, seeks to bfOffBits, handles DWORD-aligned rows and signed-height orientation, then calls CopyLine. It accepts BI_BITFIELDS but leaves its16-bit mask reading unfinished. The new reader explicitly rejects bitfield compression instead of guessing mask interpretation.

Native support: Windows DIB40, BI_RGB, one plane,8/16/24/32-bit rows, top-down/bottom-up orientation and DWORD row padding. Dimensions must be nonzero and at most4096 per axis. Header/palette/pixel ranges and declared nonzero file size are checked before reads. Signed32-bit dimensions are decoded without unaligned loads, signed shifts or abs(INT_MIN). A zero bfSize is accepted; pixel extents must still fit the actual buffer.

Indexed images use a declared palette or the default256 entries. Palette reserved alpha is ignored, as in original CopyLine. Partial palettes are a bounded extension beyond the original reader's unconditional256-entry read; undeclared indices fail. Packed16 retains the original CopyLine1555 conversion (5-bit channels left-shifted, high-bit alpha128). BGRA32 retains source alpha, including zero; it is not normalized to a conventional BMP decoder's opaque output. Full original destination-format/transparency/color-key policies still require integration.

## Actual installation evidence

- base.zwp has three24-bit/uncompressed BMPs: meshview_ground.bmp, meshview_sky.bmp and engine_turtle.bmp. All now load through the original conversion. The complete base image corpus is567 TGA/PIC/BMP requests. Original manager cache semantics produce566 entries, with smoke.tga reusing smoke.pic, and all566 entries reload successfully.
- Inventory of all88 installation archives found63 BMPs, all DIB40/24-bit/BI_RGB, and no loose BMP files. Sixty are named terrain.bmp. Inspected coregame/game_preview.cpp defines that filename as the game-preview bitmap; graphics/terrain.cpp also exposes a terrain-map export command. Decoding these files is not mission or terrain simulation/rendering.
- Every one of the63 BMPs matches Pillow12.3.0 RGBA output exactly:1,118,208 pixels, zero mismatches, zero unsupported-depth skips. This is an independent24-bit BMP pixel oracle, not Windows framebuffer parity or a16/32-bit alpha oracle. Those other depths use original-conversion golden vectors.

Reproduce privately:

```sh
ASAN_OPTIONS=detect_leaks=0 python tools/verify_bmp_archive.py build-host/bmp_rgba_probe /path/to/installation
ASAN_OPTIONS=detect_leaks=0 build-host/bitmap_conversion_probe /path/to/packs/base.zwp
ASAN_OPTIONS=detect_leaks=0 build-host/bitmap_manager_probe /path/to/packs/base.zwp
```

The comparison script uses the existing metadata inspector, reads BMP data in memory and invokes the compiled original-conversion probe over stdin/stdout. It writes no images or extracted assets. Pillow is an optional local verification dependency, not a game/Android dependency. Engine-derived probe binaries and proprietary image bytes are not uploaded.

## Compilation and tests

Host ASan/UBSan:21 CTest targets PASS. New bitmap_bmp_loading has213 checks: golden colors/alpha, padding/orientation, partial/full palettes, all truncated prefixes of representative raw/paletted files, bad offsets/declared sizes/dimensions/planes/depth/compression and palette range errors. Existing malformed-BMP manager rollback remains covered.

Software GLES:51 checks PASS in each of two recreated contexts. Four added BMP fixtures run native framing -> original CopyLine -> original bucket/Vid -> GPU pixels; a fifth verifies actual manager BMP fallback/upload/drawing and restoration with the retained manager in the second EGL context. No game images enter these GPU fixtures. Local NDK r27c/API26 compilation and private package verification pass; the new code has not executed on Android. No unexpected compilation/runtime failure occurred; malformed-input rejections are intentional tests.

Logs: docs/logs/bmp-loading. CI evidence is recorded in BUILD_STATUS. The public v0.4 adapter APK is unchanged and does not include the original pixel conversion/manager implementation.

## Next control-startup dependency gate

Inspected IFace::Init registers EventSys handlers for IFACE and Input, installs Main root-scope callbacks, initializes colors/metrics, Controls, Sound, command and cursor systems, then constructs/activates ICRoot and initializes key bindings/VarSys substitution. IControl construction calls TrackSys::RegisterConstruction; destruction unregisters tracking and deletes VarSys items. Existing metric/font and image-manager slices do not satisfy those services.

Original util/event.h additionally packs an event union containing native IFace pointers. Its32-bit layout cannot simply be assumed on ARM64; queue/pointer alignment and any serialization consumers must be audited before changing it. The source queue/handlers are inspected, not ported in this checkpoint. Next implement those actual event/tracking/color/control prerequisites and the remaining Bitmap allocation/UV/mipmap/transparency policies. M3 and a playable mission remain unachieved. No additional installation or disc files are required at present.
