# Original menu configuration dispatch

Implemented hash-pinned external staging of original Main command-scope dispatch,
IFace configuration/root-control construction and KeyBind scan-code/modifier
configuration. Upstream code and installation assets remain outside this repository.
Native adapters scope registry ownership and reject unimplemented services explicitly.
Original language initialization now uses the loaded English database. FontRegistry
and InterfaceMetrics accept original parsed scopes.

Host validation: 50/50 CTest tests pass with ASan/UBSan and local LeakSanitizer
disabled because of the workspace procfs limitation. Two tests initially could not
start because their executable bits were absent; restoring those bits allowed the
suite to run. Initial compiler failures and test logs are retained under docs/logs.
This is host evidence; new ARM64 CI results must be checked separately.

The real installation base.zwp (SHA256
f28ea56cecea135ee52c00c5f63fab975dd882778b549fc814954f3cf7d02fe9)
was tested using menu_configuration_probe. Original PTree/Main/IFace processing
now loads 128 key names and 12 modifiers from the Login configuration before
stopping at CreateCursor. Three interface statements were attempted; four control
classes were registered; no fonts or bitmaps had yet loaded at that stop.

Reproduce after the existing host build setup:

```sh
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir "$BUILD" --output-on-failure
ASAN_OPTIONS=detect_leaks=0 "$BUILD/menu_configuration_probe" /path/to/base.zwp
```

The probe intentionally returns 2 for an unimplemented dependency. It is not a
menu runtime. Original Login/Shell lifecycle, display of the original menu and a
main-menu Android APK remain unverified. No new device test is requested yet.

Next: port CursorSys ownership, original bitmap/derived cursor configuration and
standard cursor selection. Inspection of interface/cursor.cpp shows that
ProcessCreateCursor constructs Null, Bitmap, Geometric or derived cursor objects,
configures them and inserts them into the owned registry. This cannot safely be
replaced with a no-op. Then advance the real config probe to the next dependency;
complete remaining widget factories and Login user-profile services before
connecting the original menu lifecycle to the Android rendering host.

Safety fixes in staged paths include scan index 256 rejection, duplicate and
oversized key-name rejection, scoped Switch cleanup, bounded dispatch recursion,
and explicit rejection of unbound or unsupported configuration services. These
are build-time transformations of the external pinned source, not redistributed
source copies. Key configuration does not yet supply Android keyboard events.
