# Original language database prerequisites

Original VarFile::Load, StdLoad::TypeBinary, Utils::CharDecode and its lookup table
are now hash-staged externally. They run against the original FScope/VarSys ports.
The engine stores binary bytes as low nibble followed by high nibble: text `21`
decodes to byte 0x12. A conventional hexadecimal decoder is incompatible.

Native guards reject odd/oversized strings, invalid digits, paths exceeding the
original VarSys limit and binary declarations above the native 64 MiB resource
limit. The original table maps lowercase hex to zero; lowercase now fails instead
of silently changing data. Original generated databases use uppercase. Valid
input still executes the original decoder and database load/update algorithms.
Malformed-load exceptions can leave partial original VarSys mutations; discard
the affected startup/database rather than continue with partial localization.

12 focused checks pass for encoding, bounds, nested typed values, replacement,
size limits and path limits. The host sanitizer suite passes 42 tests. A first
language probe link missed reign_platform; adding that existing dependency fixed
Archive/name_crc resolution. No stub archive implementation was added.

Actual installation verification: original PTree parses english.cfg, original
VarFile loads it into VarSys, and the probe checks each version-prefixed UTF-16
record. Result: 1,515 binary entries / 30,072 UTF-16 units, valid sizes and surrogate
pair structure. It does not render text or test Babel substitutions. Reproduce:

```
ASAN_OPTIONS=detect_leaks=0 /external/build/language_database_probe /external/install/packs/base.zwp
```

Remaining localization gate: scoped lookup and bounded UTF-16 Babel formatting.
The inspected Babel formatter ignores its output-size parameter, has fixed
substitution buffers and Windows-wide printf assumptions. These must be adapted
and tested before routing real menu strings through it. Database loading alone
does not resolve Babel::Translate or initialize the original main menu.
