# Original font registration checkpoint — 2026-10-24

Original FontSys alias/file registration now executes behind native font/file adapters. This connects original PTree/FScope parsing, font declarations, original font atlas/drawing and original IFace batching/Vid submission. It does not initialize the full interface, menu or engine.

## Inspected and retained code

`tools/prepare_font_registry.py` verifies `cmake/font-registry-hashes.json` and stages code outside the repository. The inspected original `interface/fontsys.cpp` supplies FontFile/FontItem, Init/Done, both Create overloads, DeleteAll, GetFont overloads and OnModeChange. The original `system/ntree.h` NBinTree supplies alias/file trees; `util/utiltypes.h` establishes the64-byte GameIdent limit. Existing hash-verified configuration dependencies supply StrCrc and FScope. No upstream implementation is committed here.

Original behavior retained: case-folded CRC lookup; duplicate aliases retain their first mapping and report success; aliases sharing a filename share one loaded font; Create(FScope*) requires a File child; mode changes iterate every registered file once; clear disposes registered aliases/files. Source-derived code remains in a private namespace to avoid inventing ABI compatibility with the unported full engine.

Explicit native boundaries:

- FontSys global state is selected by a scoped per-registry binding. Calls must be serialized; reentrant loader callbacks fail explicitly.
- A small native Font wrapper delegates file bytes to the already ported original Font reader/draw implementation through NativeFont. Each loaded file owns its native CPU atlases. The old global texture-table reset/release calls are removed at this boundary, not replaced by empty functions. The retained original registration methods manage file/alias entries, while native ownership releases/rebuilds actual atlas data.
- Failed reads own temporary FontFile/FontItem allocations until insertion succeeds, avoiding leaks when native fatal errors throw. Reload errors propagate; earlier successful files may already have reloaded, as the original loop is incremental.
- Native callers receive shared immutable font snapshots. After reload, re-query an alias to obtain its replacement; existing shared handles retain safe older data. Original registry alias pointers still address stable wrapper entries. Full engine pointer/ABI integration remains future work.
- The bounded configure adapter dispatches CreateFont at top level or directly inside ConfigureInterface to the original Create(FScope*) method. Other scopes, including Metrics, are ignored deliberately. This is a font-only configuration pass, not successful interpretation of full IFace configuration. Identifiers beyond63 bytes or containing NUL are rejected before fixed-size storage could truncate them.

## Verified local results

- Host ASan/UBSan: all18 CTest targets PASS. The new registration target checks20 cases including configuration routing, case folding, file sharing, duplicate mapping, reload, shared snapshots after clear, failed loads/reloads, independent registries, invalid identifiers and reentry cleanup.
- Supplied base.zwp: six real FFF files registered as12 aliases, with six initial reads and six reload reads. Actual `interface_standard_fonts.cfg` then parses through original PTree/FScope and registers all nine declarations using six files. System/Button/Subtitle share the same font.
- The first real-config probe failed an incorrect harness assumption that total reads would be seven. Inspection of PTree::AddFileToScope shows binary BlockFile probing before text loading: actual reads are two config opens plus six font loads, eight total. The probe now counts font loads separately; registration output itself was already correct (nine aliases/six files). Initial failure is retained in logs; no engine behavior was changed to fit the expectation.
- Software GLES: all41 checks PASS in each of two recreated EGL contexts. The new check runs original config/registration/reload, retrieves a shared alias and draws through original Font/IFace/Vid into checked pixels.

Logs are under `docs/logs/font-registry/`. Android cross-compilation and CI evidence are recorded in BUILD_STATUS after completion. No new source-derived Android runtime execution is claimed, and no upstream code or game assets enter public APKs.

## Next engine integration

Original Bitmap manager naming/fallback/animation/lifetime/mipmaps still needs integration with the bounded TGA/PIC loader. IFace Metrics, control class registration, frame orchestration, event/input handling and Main/GameGod startup remain missing. Font-only configuration is now exercised against actual assets, but M3 and gameplay are still unachieved. Touch/pen and dual-screen remain deferred.

CI run40154496926 passes host and Android jobs for code53027af214f5103c6eddf622ea9c9c782f8b0c7c. Exact jobs/artifacts are recorded in BUILD_STATUS and logs/font-registry/ci-result.json. No original font-registration binary was published.

Further manager inspection: Setup strips the extension from the complete supplied name before tree insertion, while Find expands the path and looks up its basename. Before implementing the manager adapter, audit real call-site paths rather than assuming those two key derivations agree for directory-qualified names.
