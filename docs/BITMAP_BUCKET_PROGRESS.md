# Original bitmap pixels and render buckets — 2026-10-24

Original PrimitiveDesc, Bucket, opaque BucketMan and TranBucketMan now compile
from an external source overlay and submit through original Vid calls to GLES.
Original Pix::Setup/SetName, integer Pix::MakeRGBA and Bitmap::CopyLine also
compile separately and supply native texture pixels. This is a tested rendering
dependency slice; it does not initialize the complete Bitmap manager or engine.

## Source retained and adapter boundaries

`prepare_bucket_slice.py` pins original Primitive.h/.cpp, BucketDesc.h/.cpp,
bucket.h/.cpp, TranBucket.h/.cpp and system/nlist.h. It retains original
allocation/merge searches, state grouping, index rebasing, tag ordering,
triangle depth comparison, flush logic and translucent blend/fog transitions.
The render-buffer vertex layout is the already checked 32-byte TL format.
The nonindexed TL triangle-list path creates sequential 16-bit indices and
uses the real indexed backend. Other vertex formats/topologies and active
multitexturing reject explicitly; discarding unsupported queued data remains
possible. Original Vid global initialization wrappers and mesh-lighting lock
classes remain excluded.

Platform boundaries are actual aligned allocation/free, checked floor
conversion, diagnostic formatting and the existing texture-view/draw bridge.
Original intrusive NList is retained. The render manager's shared descriptor and
sort comparator context retain the original single-render-thread assumption.
Call Init followed by Flush(FALSE) before allocating frame data, as the harness
does; this slice does not replace the original InitBuckets orchestration.

`prepare_pixel_slice.py` pins original bitmap.h/.cpp and bitmapprimitive.cpp.
It retains the integer Pix declaration/methods and original CopyLine body;
the unused Color overload is excluded. The isolated conversion types and
methods live in reign::pixel_detail so their narrow facade cannot collide with
the future full engine Bitmap/Pix types. The full DirectDraw/Bink Bitmap class
is not compiled. A narrow conversion-only Bitmap facade implements the real
MakeRGBA contract by calling original Pix. Original GameIdent uses the already
reviewed StrCrc template; the required RGB DDPIXELFORMAT fields use a checked
32-byte native declaration. Bounded formatting replaces wsprintf.

The public conversion adapter validates dimensions, exact buffer sizes,
palette availability, disjoint contiguous masks of at most eight bits and a
128 MiB per-buffer diagnostic limit before calling original code. Source
formats: indexed 8-bit, TGA-style ARGB1555, BGR24 and BGRA32. Destination formats:
validated 16-/32-bit RGB(A), or original BGR888 for the direct-copy 24-bit path.
The original ARGB1555 conversion maps a set alpha bit to 128, not 255, and expands
5-bit color to multiples of eight. These behaviors are preserved and tested.
Indexed palette alpha is ignored in the original CopyLine path; that is also
preserved. These are not claims that original ReadTGA/ReadPIC or FileSys-backed
Bitmap loading have been ported.

## Scoped source edits and observed failures

- The first original-bucket compile exposed assertion invocations without a
  trailing semicolon. The bucket-specific adapter accepts that convention.
- UBSan then caught a real misaligned U16 write with a 1,000-byte pool slice.
  Original ratio rounding placed the index region at an odd offset. The overlay
  aligns the split and reserved index/next-batch regions to four bytes. The
  original policy/search algorithm remains, with small capacity differences
  where padding is required. See alignment-before-fix.log.
- Native request/pool guards prevent size arithmetic overflow and enforce the
  16-bit vertex-index capacity. Original index rebasing and sorting now validate
  indices before changing or dereferencing them.
- The translucent fixed 444-pointer scratch array becomes a vector sized to the
  actual descriptor count. The original selection ordering is unchanged and is
  tested with 450 distinct active buckets. This is not a performance benchmark.
- MSVC-only redundant in-class qualifiers and filename case assumptions are
  corrected in the external overlay. A zero-fill NULL argument becomes zero.
- Three original 24-bit stores through an aliased structure pointer become
  three-byte memcpy operations. Original integer color packing remains intact.
- The first allocation test assumed two batches would merge even in a very
  small bucket; the original extra-three-vertex reservation correctly prevented
  that. The merge/alignment fixture now sweeps 512–1,056-byte pools in four-byte
  increments. The fixture failure is retained separately from engine failures.
- One local CTest launch failed because bitmap_pixels_smoke had mode 0644.
  Restoring its executable bit allowed all 14 tests to run and pass. The
  permission failure is retained separately; no source fix was required.
- Clang emits an empty-loop-body warning in original Pix::Setup; its deliberate
  bit-count loop is retained. It is a warning, not a failed build.

Upstream files stay unchanged. Scripts contain hashes and scoped edit recipes,
not vendored implementations. Generated source, engine objects/binaries and game
assets remain outside repository/artifact publication.

## Verification

- Fourteen host CTest targets pass with ASan/UBSan (local LeakSanitizer has the
  previously documented environment limitation; CI uses its default setting).
- The bucket and bitmap golden tests also pass in an optimized Release build
  with ASan/UBSan; release configure/build/test logs are retained.
- Original CopyLine: 18,504 independent scalar golden pixel comparisons across
  four source depths and six destination formats, including odd-width rows.
- Supplied base.zwp: bounded native TGA decode followed by original CopyLine
  conversion matches 106 images / 1,934,336 pixels exactly; five TGA files use
  formats unsupported by the current native TGA decoder and are explicitly
  skipped. This does not verify original ReadTGA execution.
- Real software-GLES readback runs 31 checks per context in two fresh contexts:
  nine baseline, 13 original Vid, five bucket/transparent/fog, four original
  CopyLine-to-bucket-to-GPU cases. No screenshot alone substitutes for these
  original-function host tests.
- The original opaque/translucent bucket and pixel-conversion targets and smoke
  executables compile for Android ARM64 with NDK r27c/API26. They are not yet
  executed on a physical device.

Raw logs: docs/logs/buckets/ and docs/logs/pixels/. CI results are recorded in
BUILD_STATUS.md after the branch build completes.

## Remaining launch dependencies

1. Port original Bitmap lifetime, file loaders (including unsupported TGA/PIC),
   texture registry/upload/mipmaps, surface locking and context restoration.
   Current texture views remain externally owned handles with no manager cache.
2. Original interface/font.cpp::Read builds 128x128 glyph textures through
   Bitmap Create/Clear/Lock/MakeRGBA/PutPixel/LoadVideo; these manager/storage
   methods are the next concrete font prerequisites. Its default DOTRIFAN
   rendering path also requires triangle-fan support (currently rejected).
   Connect the original InitBuckets/frame orchestration and frontend render
   helpers; validate real UI drawing and clipping rather than constructing a
   substitute menu.
3. Add needed texture stages/samplers and complete original VarSys, font,
   interface, Main/GameGod, timing/input/audio and scene services.
4. Exercise an engine-initializing APK on device once those services are real.

Original FlushTex resets unrelated queued bucket state in the inspected code;
that behavior has not been changed in this slice and needs call-site analysis
before native texture eviction is connected. Full Bitmap/terrain/mesh startup,
menu and mission execution remain unfinished. M3–M5 remain unachieved. No new
playable APK or additional owner test is claimed or required by this slice.
