# Original interface expressions and text utilities

The hash-checked external staging recipe compiles original Operation, Comparison,
Switch and relational operators against the existing FScope and VarSys ports.
No upstream source or engine binary is stored in this repository.

Runtime verification: 16 checks cover integer/float/string operations, variable
operands, comparisons, typed switches, missing variables/arguments and invalid
operators. One inspected upstream defect is patched during staging: Switch's
float branch creates an empty relational operator and always errors. The recipe
requires exactly one matching statement and changes it to equality, consistent
with the integer/string branches. This is an explicit behavior correction.

Native text support adds bounded UTF-16 conversion with ASCII as the default and
an explicit scoped Windows-1252 selection. Other high-byte input fails until the
installation encoding is selected; UTF-8/DBCS support is not claimed. Legacy
hexadecimal formatting preserves width, padding, prefix and suffix semantics
without constructing pointers before the destination buffer. 14 checks pass.

Host ASan/UBSan suite: 37/37 tests pass (local leak detection disabled because of
procfs restrictions). Full six-control link: 17 unresolved symbols; no garbage
collection used to conceal missing methods. See logs/interface-expressions.
Two zero-byte cached build artifacts were preserved externally and regenerated.
An earlier test invocation overlapped rebuilding and was discarded; the recorded
final suite ran after build completion.

Quads CI run 40223005932: host passed and ARM64 compilation passed, but the GLES
host-test compilation failed from conflicting legacy headers. The frontend GLES
checks now compile in their own library with UI include ordering. Pixel success
for this correction must be verified by the subsequent CI run.

These components do not yet initialize the original menu or create a new menu APK.
