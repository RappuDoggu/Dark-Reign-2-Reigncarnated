# Native bitmap loading checkpoint — 2026-10-24

The bounded native TGA/PIC readers now feed the original `Pix`/`Bitmap::CopyLine` conversion. Source files remain in the separately pinned upstream checkout; no upstream implementations or image assets are committed. These readers are original porting code, not a claim that the full original `Bitmap::ReadTGA`, `ReadPIC`, manager, DirectDraw surface or mipmap implementation has been ported.

## Inspected implementation and supported path

Reference: pinned `graphics/bitmap.cpp`, `Bitmap::ReadTGA` at line1719 and `Bitmap::ReadPIC` at line2245; conversion provenance/hashes remain in `cmake/pixel-source-hashes.json` and `tools/prepare_pixel_slice.py`.

- TGA: raw/RLE indexed8, packed16, BGR24 and BGRA32; image IDs, bounded palettes, both horizontal and vertical origins. RLE is bounded against total image pixels and may span rows. Indexed palette indices must lie within the declared range. Palette alpha is ignored as in the original conversion. The inspected original 16-bit palette extraction's channel order and 5-bit left shifts are retained explicitly.
- PIC: big-endian Softimage header, RGB channel with optional separate alpha channel, raw and mixed RLE streams per scanline. Long runs use big-endian counts without unaligned loads. Zero-length and overlong runs fail before writes. Unknown encodings/channel chains are rejected rather than taking the original reader's generic uncompressed fallback.
- Both readers enforce nonzero dimensions at most4096 per axis, complete payload bounds and owned buffers. File format parsing performs no GPU work. Final rows go through the original conversion, retaining packed1555 alpha128 and multiples-of-eight color expansion. TGA grayscale/interleaving and other PIC channel layouts remain unsupported.
- TGA RLE true-color and horizontal mirroring are bounded extensions: the inspected original ReadTGA rejects true-color RLE and right-origin images. PIC raw and mixed RGB/alpha follow the inspected original layout. Windows framebuffer/image parity for the real images has not been captured.

## Reproduction and evidence

Configure with `REIGN_UPSTREAM` pointing outside the repository; build `bitmap_loader_smoke`, `bitmap_conversion_probe` and the ordinary CTest suite. `bitmap_tga_loading` is automatically included in host CI and its executable cross-compiles in the Android job. Run `bitmap_conversion_probe /path/to/packs/base.zwp` privately against a legally owned installation.

Actual supplied archive result: 106 true-color TGA images independently compared against the existing native true-color decoder, with zero differences; five formerly skipped indexed/16-bit TGA images now load; all453 PIC images load. Total564 images /11,605,888 pixels. The PIC/extended-format real-image result establishes successful bounded loading, not an independent Windows pixel oracle. Synthetic golden vectors cover those format/color/alpha rules.

`bitmap_tga_loading` passes288 checks, including golden raw/RLE colors, indexed/16-bit alpha rules, all four TGA origins, all truncated prefixes of representative TGA/PIC payloads, image IDs, palette ranges, malformed streams, raw/mixed PIC RGB/alpha and long runs. The existing18504 pixel-conversion golden comparisons remain unchanged. GPU tests now include TGA framing before the four original CopyLine/bucket submissions, plus one PIC alpha test, repeated in two EGL contexts.

## Device evidence and remaining gate

The owner's v0.4 report is retained in `logs/bitmap-loading/fold-v04-owner-report.txt`. Fold5 SM-F946U/API36/Adreno740 passed all12 public adapter GPU checks with context generation2, surface1692x666, archive entries2074 and cursor atlas decoded. The1073ms archive diagnostic is a single observation, not a performance benchmark. This report does not execute the new source-derived bitmap loading path or original engine startup.

Next: original Bitmap registry/lifetime/reload rules, CPU-to-GPU texture ownership and mipmaps; remaining BMP support; FontSys aliases/config registration and IFace startup/frame/control orchestration. Full Main/GameGod initialization, input/audio, menu and simulation are still unported. No new APK is needed just to repeat the unchanged public adapter checks. Source-derived engine binaries remain private pending redistribution authority. M3/M4 remain unachieved.

Original manager constraints for the next implementation: `Bitmap::Manager::Find` expands the path and keys the tree by the case-folded CRC of the filename without extension, rather than the full path. `FindCreate` first tries the requested filename, then `.pic`, `.tga`, `.bmp`; it also discovers numbered animation frames and releases an existing lower-resolution surface when a smaller reduction is requested. A generic full-path texture cache would change these rules and should not replace this manager silently. FontSys registration additionally shares loaded files between named aliases, accepts `CreateFont`/`File` scopes and reloads atlases on mode changes; the current per-font native facade does not implement that registry.
