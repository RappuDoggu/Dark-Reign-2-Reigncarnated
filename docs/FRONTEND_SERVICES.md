# Frontend service linkage — 2026-10-25

## Implemented and executed

- Added `dr2_frontend_config`: 19 original StdLoad implementations for integer/float/string/CRC/point loading, staged externally from hash-verified source. Original inclusive range checks, defaults and signed-coordinate behavior are preserved. Modern explicit-specialization syntax is applied to original static Range definitions.
- Implemented native UTF-16 code-unit length, bounded padded copy and duplication. These avoid casting 16-bit engine strings to Linux/Android 32-bit wchar_t. Surrogate pairs remain two code units, matching the Windows engine's string storage semantics.
- Implemented original Array allocation entry points using malloc/posix_memalign/free. Zero-sized allocations return null; invalid non-power-of-two alignments are rejected; small alignments are raised to pointer alignment.
- Added `EventLoop::run_native`, a scoped binding for calls into original EventSys APIs. This is needed when original IFace routines enqueue events outside EventLoop::process. Nested bindings restore the prior loop on normal return and exceptions. Call on the owning thread; do not retain native-state references.

Host GCC13.3 build passes. All **29 CTest targets pass** with ASan/UBSan and the documented local LeakSanitizer limitation. New tests execute 24 original configuration checks, 14 native string/allocation checks and 10 native event-context checks. They do not construct a real control tree or display the menu.

The event test registers the original event type/handler, enqueues through original PutLock/PutUnlock and Put, and dispatches through EventLoop::process. It checks two-loop isolation, nested exception restoration, absence of a native binding outside the callback, and empty-callback rejection. It does not claim that original IFace::PostEvent or its Reaper lifetime path has run yet.

Configuration tests include range/type errors, signed and unsigned coordinate values, CRC of Shell, missing-required settings, optional defaults, integer-to-float conversion, and an original behavioral distinction: an absent named scope returns its default without range validation, while an absent argument in a present scope validates the default.

## Link result and next gate

The actual executable link with six original control objects and current host static libraries still fails. Unique unresolved symbols fell from **77 to 62** in this particular link probe. This is not a completion percentage: integrating IFace startup can expose further dependencies.

Remaining work includes IFace state/control factories/event routing/drawing, the global original Font/Bitmap boundary, Input production, Babel, remaining control classes, and the explicit movie adapter. Native event context is now available for that integration, rather than supplying empty implementations for unresolved services.

No menu APK, gameplay, new on-device test or original interactive window is claimed.

## Workspace recovery

Automated maintenance removed the workspace project/build directories, SDK and local installation/analysis files. The project was restored through the connected GitHub integration at commit `8108ba0c53908eec1b172eaab419bfdc726a1bb8`; the separate pinned upstream checkout survived. Direct authenticated git clone was unavailable, so the connector restored 169 project files. Existing remote logs were preserved in the base tree rather than downloaded and overwritten.

Committed analysis and reproducible scripts remain in GitHub. Private raw decompiler output and the local Ghidra database were not recreated in this increment. Asset bytes remain recoverable from the owner's uploaded installation when needed. No user-driven deletion or deliberate analysis reset is implied.

The local Android SDK was removed, so this increment's ARM64 verification uses GitHub CI rather than reporting an unperformed local cross-build. REA is still deferred; no new provider integration success is claimed.

## Reproduction and evidence

```sh
cmake -S . -B build-host -DREIGN_UPSTREAM=/absolute/external/dr2-upstream -DREIGN_SANITIZE=ON
cmake --build build-host -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-host --output-on-failure
```

Use the existing Android configuration instructions to build the same default targets with NDK r27c. New libraries/tests are included in default host and Android builds. No game implementation or assets are committed.

[Build](logs/frontend-services/build.log), [29-test result](logs/frontend-services/tests.log), [configuration tests](logs/frontend-services/config-test.log), [native support tests](logs/frontend-services/support-test.log), [event context tests](logs/frontend-services/event-context-test.log), [link errors](logs/frontend-services/link-failure.log), [machine-readable result and exact link command](logs/frontend-services/result.json). The link probe main is `int main(){return 0;}` and failed to produce an executable. CI status is recorded separately after completion.
