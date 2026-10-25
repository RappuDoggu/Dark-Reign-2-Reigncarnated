# Original interface registry — 2026-10-25

The scoped interface session now owns the original BinTree<ICClass> and runs four more original methods: RegisterControlClass, UnregisterControlClass, FindControlClass and DefineControlType. This supports built-in registration and script-defined class templates, including original FScope duplication and factory-pointer inheritance. The same session owns event/focus state and the registry; there is no parallel global registry. The original ICClass header is hash-checked too.

The registry's readiness guard describes this component only. Full IFace initialization, control instantiation, root windows and rendering remain incomplete. The test callback deliberately throws if invoked; tests do not claim to exercise a working control factory. The factory methods themselves remain unresolved in the full frontend link.

## Integration defect found and fixed

The first sanitizer execution caught an FScope allocation/deletion size mismatch: configuration code allocated 176 bytes while the interface consumer expected 184. DEVELOPMENT adds a field to this original class. Existing preludes enabled it in some translation units but configuration compilation did not. CMake now defines DEVELOPMENT=1 consistently for all targets, rebuilding both producers and consumers. This also brings the original FScope::Dirty implementation into the linked configuration library. No sanitizer check was disabled to resolve the mismatch.

DefineControlType originally discovers duplicate names after BinTree::Add has inserted another node. Since fatal errors become exceptions in this port, a preflight duplicate check now rejects the definition before mutation. Valid definitions retain original behavior. Registry destruction disposes all retained templates and their scopes. RegisterControlClass's original return convention is preserved: FALSE indicates insertion without a duplicate, not failure.

## Verification

- 23 registry checks pass: registration/lookup, duplicate rejection, independent sessions, copied scopes surviving their input's destruction, nested template metadata, missing base rejection, unregister, and shutdown ownership.
- All 31 host tests pass with ASan/UBSan. Local LeakSanitizer is disabled only for the existing procfs restriction; CI runs normal sanitizer settings.
- Full frontend executable linkage still fails with 47 unresolved symbols, down from 49. This count is not a completion percentage.
- No live window, main menu, new device execution or gameplay is claimed.

Reproduce using the existing CMake host setup and `ctest --test-dir build-host --output-on-failure`; build `interface_registry_smoke` for the focused test. Source remains separately staged and hash checked. Upstream code/assets and source-derived executables are excluded from repository artifacts.

Evidence: [test](logs/iface-registry/test.log), [suite](logs/iface-registry/suite.log), [initial sanitizer failure](logs/iface-registry/layout-failure.log), [build](logs/iface-registry/build.log), [full link failure](logs/iface-registry/link.log), [link command and status](logs/iface-registry/result.json).

Next integration work: original CreateControl/root lifecycle, shared Font/Bitmap types and rendering services. The registry is a prerequisite, not a substitute for those implementations.

## CI result

Both jobs passed for commit `6ac355cdcb95b98ed9125fab6af098e97d70acc8` in [run 40212350443](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/actions/runs/40212350443). Host tests passed 31/31 with the normal sanitizer environment, including the 23 registry checks. Android NDK built the registry test for ARM64; GLES checks and existing diagnostic APK packaging passed. No ARM64 registry runtime execution or menu launch is claimed. [Provider output and artifacts](logs/iface-registry/ci-result.json).
