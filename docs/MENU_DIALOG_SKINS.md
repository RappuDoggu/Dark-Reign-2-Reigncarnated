# Original menu dialog events and window skins

External hash-checked staging now compiles the complete original messagebox_event
translation unit and the original ExpandDerefVar method. The latter is compiled
and linked only; live IControl-relative dereferencing is not yet runtime-tested.

The original TextureSkin::Render nine-piece layout submits through the existing
frontend batch adapter. A bounds guard rejects an invalid color state before
indexing the original state array. The tests check all nine rectangles, missing
state behavior and invalid state rejection: 33 quad/skin checks total.

Seven message-box event checks verify caption copying/ownership, event identifier,
callback dispatch and context, dead notification recipient and null callback
rejection. This does not implement the message-box window/factory itself. The
focused event executable discards unreachable controls with section GC; the
separate complete link probe uses no GC and still reports 13 missing symbols.

Host sanitizer suite: 38/38 passed after regenerating three damaged cached
executables (one zero-length, others lacked execute permissions). No cause for
these workspace artifact changes has been established. Evidence is in
logs/menu-dialog-skins; original sources/assets remain external.

CI for the previous expression commit passed host tests but failed Android Clang
compilation: relationaloperator.h accessed incomplete FScope. The expression
stager now includes fscope.h explicitly before the template. The fresh CI run
must confirm ARM64 and GLES results; no APK/menu success is claimed here.
