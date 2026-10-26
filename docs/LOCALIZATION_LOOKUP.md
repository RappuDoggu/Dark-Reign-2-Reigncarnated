# Original localization lookup and Babel port

The source checkout remains external. `prepare_localization.py` and
`prepare_babel.py` verify SHA-256 hashes before staging the original language
lookup and Babel substitution implementations. No upstream bodies are committed.

`reign::Localization` binds a language name to an already initialized VarSys
session. Original MakePath/GetItem/Lookup resolve `multilanguage.<name>.<key>`.
The native boundary checks path length, binary type, version-header size, even
UTF-16 byte count, surrogate pairs and Babel's 16,384-code-unit buffer limit.
A returned view borrows the original VarSys binary value; do not modify/delete
that value while using the view. Scope restoration works after exceptions and
nested language selection. This does not implement Windows font metadata.

Babel retains its original scan, index ordering, BinTree substitutions, variadic
argument consumption and five-result-buffer rotation. Native helpers implement
bounded UTF-16 copies and integer/float/string/character formatting without
casting Linux wchar_t buffers to CH. Format validation rejects malformed,
duplicate, missing and out-of-range indices, dynamic widths, length modifiers,
unsupported conversions and excessive widths/precision before consuming args.
Pointer and `%n` formatting are explicitly unsupported; they are not required by
the inspected menu controls. String formatting counts UTF-16 code units, as the
original Windows engine did. Numeric decimal formatting uses the C locale
provided by the process; callers must not change LC_NUMERIC during a session.

Original bugs addressed in staging:

- Doubled opening braces failed to advance either scan and could loop forever.
  Scans now skip the pair, leaving the literal pair unchanged in output.
- Destination size was unused; substitution and final copies now enforce it.
- Format strings could overflow a 512-unit Sub. Preflight validation bounds them.
- Exceptions could leak substitutions or skip va_end. Cleanup guards cover both.
- L"Error" and missing-key formatting depended on Windows 16-bit wchar_t.
  Explicit CH text and bounded ANSI conversion replace these boundaries.

Babel's result buffers are per thread, not per language. A pointer is valid until
the fifth subsequent translation on that thread. Callers must supply arguments
with the types required by each specifier, as for the original variadic API.
Rejected formatting can leave a partial destination; callers must discard it.
ANSI conversion remains the existing explicitly selected ASCII/Windows-1252
path. This increment does not establish an encoding for every installation.

Reproduce with the normal sanitized CMake host build and CTest. Tests cover
language selection and record validation, reordered arguments, signed/unsigned
values, float precision, Unicode strings, nested translations, padding,
buffer rotation, bounded output, malformed formats and missing keys. Tests use
synthetic database records; asset-backed Babel rendering is not yet demonstrated.
The main menu still requires command dispatch, message-box/control construction,
sound/movie decisions and actual Login/Shell lifecycle integration.
