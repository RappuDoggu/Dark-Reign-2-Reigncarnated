# Original bitmap manager checkpoint — 2026-10-24

Original manager naming, format fallback, numbered-animation construction and release/restore iteration now execute behind native bitmap ownership adapters. This is a stage-zero, unreduced, level-zero TGA/PIC texture slice, not the complete original Bitmap/DirectDraw implementation or engine startup.

## Retained original implementation

`tools/prepare_bitmap_manager.py` verifies `cmake/bitmap-manager-hashes.json` and stages original Bitmap::Manager::Setup/Create/Find/FindCreate/ReleaseDD/OnModeChange outside this repository. The original NBinTree implementation owns registered bitmap entries. Reference hashes also cover Bitmap declarations/reload behavior, Utils::FindExt, Dir::PathExpand and the256-byte BuffString declaration. No original implementation or assets are committed.

Retained behavior: first lookup by case-folded basename CRC without extension; the requested format wins if present, otherwise try .pic, .tga, .bmp; discover consecutive numbered frames and form a circular next-frame chain; release each tree entry; restore released entries through original mode-change iteration. A cache hit can return a released entry: callers must restore before drawing rather than expecting FindCreate to reload it automatically.

The inspected Setup/Find path discrepancy remains visible: Setup strips an extension from the entire supplied name before insertion, while Find expands the name to its basename. A directory-qualified creation can therefore fail a subsequent Find; the test preserves this behavior instead of silently normalizing keys. The native split-path adapter handles slash/backslash/drive components; obscure MSVCRT path edge cases have no independent Windows oracle.

## Explicit native boundaries

- Per-manager scoped state replaces globals; loads, view reads and graphics callbacks must be serialized. Loaders/callbacks must not reenter the manager.
- Native Bitmap Read/ReLoad/ReleaseDD adapters own actual bounded image decoding and optional native texture handles. Original Pix/CopyLine still performs color conversion. Full original Bitmap methods, DD surfaces, reduction, mipmaps, nonzero stages, Bink and BMP decoding are not ported. The public interface does not accept unsupported option flags. BMP fallback reaches an explicit unsupported-format error if a BMP exists.
- CPU-only mode explicitly uses no GPU callbacks/handles. GPU mode requires upload, destruction and unbinding callbacks together. Upload must produce a nonzero handle; destruction/unbinding callbacks must not throw and must run on the owning context thread. Original manager ReleaseDD unbinds first, then invokes each real native resource deletion. Direct3D global eviction is removed at this boundary, not replaced by a success stub. Bink auto-dispatch is omitted from the scoped static/numbered texture path.
- Managed views share stable record objects. Release/clear/destruction invalidate their GPU handles and residency while retained CPU pixels may remain alive through external references. Reload increments generation. Already resident entries are not re-read. Restore is incremental and propagates errors.
- On lost context, abandon_context invalidates handles without any driver/deletion/unbind calls. The owner must actually discard that context, then restore with callbacks targeting the new current context. Original restore iteration re-reads source files and uploads new handles; retained record references remain stable.
- Temporary Create ownership and per-load rollback remove newly registered animation entries if any read/upload fails. Existing entries survive. A file disappearing between existence checks and Setup also fails instead of reporting an uninitialized texture. This intentionally improves corrupt/missing-data failure handling rather than returning unusable success. Names are bounded to240 bytes, animation formatting is checked and the manager caps4096 entries.

## Actual verification

- Host ASan/UBSan:20 CTest targets PASS. New original_bitmap_manager has29 checks for cache/extension semantics, fallback priority, animation loops/gaps, real callback ownership, release/restore/no-op resident reload, lost-context invalidation, failed animation/upload/BMP loads, rollback, scoped managers, path-key discrepancy, invalid names, reentry and disappearing files.
- Supplied base.zwp:564 TGA/PIC requests produce563 tree entries/563 initial image reads. smoke.tga reuses an already loaded smoke.pic through the original extension-independent cache. Release followed by restore re-reads all563 entries. This inventory-order observation does not establish the original game's first-use ordering or Windows pixel equivalence for those two files. No installation corruption or missing disc files is inferred.
- Software GLES:46 checks PASS in each of two EGL contexts. Four new checks use original manager lookup/fallback/animation, original pixel conversion, actual upload/deletion callbacks and original Vid drawing. They verify pixels, release/restore, and retained manager records reloaded in a second EGL context. No game assets are used in GPU fixtures.
- Initial GPU fixture compilation failed because it referenced unimported U16/DP_DONOTUPDATEEXTENTS aliases; fixed with std::uint16_t and the actual draw interface. Its first execution then failed Unsupported Vid blend factor because flags0 omitted original source/destination blend bits. Fixed by supplying RS_BLEND_DEF; strict adapter checks and original engine code stayed unchanged. Both failure logs are retained.

Logs: docs/logs/bitmap-manager. Android compilation/CI evidence is recorded in BUILD_STATUS after completion. There is no new original-derived Android runtime result and no new public APK behavior.

## Next engine dependencies

BMP loading, original mipmap/reduction and transparency/color-key policies, remaining full Bitmap integration, IFace colors/skins/control registration, frame/event orchestration, input/audio and Main/GameGod startup remain unfinished. This manager slice does not initialize the original menu, scene or simulation. M3/M4 remain unachieved; pen/touch and dual-screen remain deferred.
