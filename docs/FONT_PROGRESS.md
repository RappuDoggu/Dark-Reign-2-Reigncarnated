# Original font/IFace rendering slice — 2026-10-24

The original FFF reader, glyph atlas builder, text measurement, Font::Draw and
IFace rectangle batching now compile through a SHA-pinned external source
overlay. Original FontSys atlas allocation/lookup functions are retained with
scoped native storage. This does not initialize the complete FontSys alias
registry, UI controls, original Bitmap manager or Main/GameGod.

## Corrected source finding

The earlier plan inferred triangle-fan use from a DOTRIFAN define/comment.
Inspection of the complete current Font::Draw shows that define is unused there:
the function calls IFace::GetVerts/SetIndex and emits indexed glyph rectangles.
The implementation follows that actual path. No fan support was added or claimed.

## Implemented boundaries

- Native CPU-backed RGBA8 BitmapSurface implements real allocation, clear,
  lock/unlock, bounded pixel writes, half-texel UV offsets and publication to a
  required consumer. Unbound or locked publication fails. This is not a fake
  DirectDraw surface or the full Bitmap class.
- A private font_detail namespace retains selected original Font and IFace code
  without colliding with future engine-wide types. The FontSys atlas table is
  scoped to a NativeFont instance, with the original 1,000-atlas limit.
- File bytes are validated before original Font::Read obtains raw pointers:
  identifiers, v1/v2 header/table extents, two-byte glyph image records, source
  100-pixel glyph limits, payload extents and a 128 MiB input limit.
- Unaligned 16-/32-bit loads use memcpy. Original multichar constants become
  their inspected fixed-width identifiers. Removing the zero-length pixel-array
  extension preserves the two-byte on-disk image header; record sizes are
  asserted. Temporary arrays use unique_ptr for fatal-error exception cleanup.
- Original LoadVideo calls at atlas completion are explicitly replaced by CPU
  atlas Publish calls. GPU upload happens only through a caller-supplied upload
  consumer, which can replay retained pixels after context recreation. The
  adapter does not claim to have ported the original Bitmap::LoadVideo.
- Original IFace heap requests receive bounded native vertex/index arrays;
  original capacity/state-change flushes, index offsets and font draw commands
  remain. The required draw consumer receives actual original batches. Sampler
  and material boundaries reject unsupported uses. Thread-local batch state and
  exception cleanup prevent a failed consumer from contaminating the next draw;
  reentrant rendering is rejected.
- The original Utils::FtoL x86 implementation temporarily selects truncation
  mode and restores its control word. The font adapter uses a C-compatible
  truncating cast; opacity 255*0.5 becomes 127, with caller rounding mode retained.
- Original glyph advances, missing-character behavior, half-texel offsets,
  alpha truncation, shadow order/opacity and horizontal early-return clipping are
  preserved. Full interface clipping, FontSys registration and localization
  integration remain outstanding.

## Raster coverage failure and fix

The first font GPU fixture failed despite correct CPU atlas bytes: its top row
was missing. Raw readback is retained in gles-font-failure.log. The old adapter
inverted Y before triangle rasterization; on tested Mesa this changed coverage
at integer-coordinate glyph edges. The backend now renders in logical game
coordinates into a reusable RGBA8/depth16 framebuffer and flips the completed
color image once in present(). Original glyph coordinates and UVs are unchanged.
No epsilon/bias was added to game vertices.

Call present() once after the frame's draws and before EGL swap. Readback retains
its previous bottom-up display-row API. JNI preview calls present(), and pixel
checks verify both presentation orientation and target resize/reallocation.
Lost-context cleanup explicitly abandons old GL names on onSurfaceCreated rather
than relying on opaque EGL handle equality. Driver/device/surface/context data
are included in the v0.4 report.

Reference for integer pixel centers and top-left filling of legacy TL drawing:
[Microsoft rasterization rules](https://learn.microsoft.com/en-us/windows/win32/direct3d9/rasterization-rules)
and [texels/pixels](https://learn.microsoft.com/en-us/windows/win32/direct3d9/directly-mapping-texels-to-pixels).
These D3D9 compatibility references are not a captured D3D7 game-render oracle.
The actual coverage fix is verified by our retained readbacks and the new GPU
fixtures. The Fold's updated driver path still needs owner testing.

## Observed verification

- Six real v1 fonts from supplied base.zwp load through original Font::Read:
  font_allcaps (225 glyphs/2 atlases), font_console (225/1), font_movie (226/4),
  font_system (226/2), font_tiny (224/1), font_windowtitle (225/2).
  Total: 1,351 glyphs, 12 atlases; sample Draw emits 504 vertices without failure.
  Only metadata/logs are committed, never font files or atlas pixels.
- Original font fixtures cover v1/v2, atlas alpha/gutters, metrics, missing
  glyphs, UTF-16 codes, geometry/UVs, capacity and texture-change flushes,
  alpha/shadow rules, clipping, atlas replay, consumer failure/recovery and
  reentrancy rejection. Every truncated prefix of the small fixtures rejects.
- Sixteen host CTest targets pass with ASan/UBSan; local LeakSanitizer retains
  the previously documented environment restriction. Optimized component checks
  are recorded separately. CI uses default leak checks.
- Software GLES: 39 checks per context in two fresh contexts (12 original
  porting adapter checks, 13 original Vid, 5 original buckets, 4 original pixel
  conversion, 5 original font/IFace rendering). The same retained CPU font is
  uploaded into both contexts. Font checks cover alpha/coverage, tint/alpha
  scale, source shadow order, clipping and multi-atlas texture changes.
- Original font/IFace targets and probes compile for NDK r27c/API26 ARM64.
  They have not run on a physical Android device.
- Renderer Test v0.4 packages only original porting adapters and runtime notices,
  with no original game implementation or assets. Its 12 GPU checks, especially
  integer coverage, presentation and resize, are the owner's next device test.
  The local debug certificate matches the delivered v0.3 APK.

Actual failures/resolutions: initial staging selected a class-name comment
instead of its declaration (marker fixed); an isolated GPU test used an absent
U16 alias (std::uint16_t fixed); Make needed explicit reconfiguration before new
multiple targets; a local archive-smoke executable lost its execute bit (restored
and the full suite reran). These are retained separately from the raster bug.

## Remaining path to original menu launch

1. Validate v0.4 framebuffer presentation/resize/context recreation on Fold.
2. Port original Bitmap file loading/registry/lifetime, including PIC and the
   five still-unsupported TGA formats. Connect user installation resource streams.
3. Integrate original FontSys aliases/configuration and IFace startup/frame
   orchestration, controls, clipping, native input and remaining Main/GameGod
   prerequisites. No substitute menu is created by this font test.
4. Boot an engine-initializing APK, then exercise original menu/mission loading.

M3–M5 remain unachieved. Touch/pen and dual-screen work stay deferred. No additional
installation files or disc copy are currently needed.
