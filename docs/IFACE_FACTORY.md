# Original control factory boundary — 2026-10-25

The staged interface now compiles 12 original factory/lifecycle methods: four CreateControl overloads, two Activate overloads, two Deactivate overloads, FindByName, RootWindow, OverlaysWindow and GameWindow. Their registry, flags and root references use the existing scoped interface state. Root pointers retain the original ICRoot type. A separate treeReady flag starts FALSE; it is not set by registry initialization or these tests. A real bootstrap must construct and own the roots before marking the tree ready. This increment does not implement that bootstrap.

The new generated factory translation unit belongs to dr2_iface_events. Function/data sections allow narrow tests to link the actual root getters while discarding unreachable factory code. The full-control link probe uses no section collection and still fails. No factory, drawing or lifecycle dependency has been replaced with a success stub.

## Executed original configuration behavior

CreateRegData and FindRegData now use the session's original BinTree<FScope>. The internal load_registry bridge invokes the original static loader and rejects a null input. Stored scopes are original deep copies and are disposed at session shutdown. Original duplicate handling logs an error and retains the first definition. FindRegData resets scope iterators and optionally returns the first child scope, preserving menu-template lookup semantics.

The registry suite passes 38 checks (previously 23), including copied scope lifetime, repeated reads, duplicate retention, absent entries, empty bodies, session isolation and uninitialized root getters. It does not invoke a live control factory or simulate a menu with replacement controls.

## Failures and verification

- Initial factory compilation failed because a generic IControl root has no FindByName method; retaining ICRoot fixed the type boundary. The new loader also required the staged control_stdload declarations.
- Three old generated executables lacked execute permissions; tokenizer_smoke and installation_probe were empty, while frontend_config_smoke retained ELF magic. The originals were moved aside, all three were re-linked, and the full suite then passed 31/31. No cause is asserted and no source test was bypassed.
- Full frontend linkage fails with 38 unique unresolved symbols, down from 47. Remaining symbols include fonts, drawing, texture/skin conversion, variable expressions, input, sound, timing, message boxes and IFace data. This count is not a progress percentage.
- Host ASan/UBSan tests passed; local LeakSanitizer is disabled for the known procfs restriction. CI results are recorded separately after completion.
- No live control tree, main-menu APK, new Android runtime execution or gameplay is claimed.

Reproduce with the existing CMake host build, then `ctest --test-dir build-host --output-on-failure`. The exact full-link command and unresolved list are in [result.json](logs/iface-factory/result.json). [Build](logs/iface-factory/build.log), [registry execution](logs/iface-factory/test.log), [suite](logs/iface-factory/suite.log), [initial compile failure](logs/iface-factory/initial-failure.log), [artifact failure](logs/iface-factory/permission-failure.log), [relink](logs/iface-factory/relink.log), [full link errors](logs/iface-factory/link.log).

Next: unify the original frontend's Font/Bitmap interfaces with the already ported font/texture implementations, implement drawing/state services, and link a genuine ICRoot instance before attempting menu construction.
