# Original texture configuration connected to native bitmap loading

FScopeToTextureInfo is hash-staged from original iface_util.cpp without changing
its configuration/UV algorithm. Bitmap::Manager::FindCreate forwards to the
existing original-manager slice through a scoped FrontendTextures adapter.
The bridge owns immutable CPU snapshots and stable Bitmap wrappers; retained
TextureInfo references remain valid until the bridge is destroyed. New manager
generations receive new wrappers. The manager must outlive the bridge, and the
bridge must outlive its controls. Draw submission still requires an actual sink.
No texture upload, movie playback or animation is simulated by this adapter.
Multiple animation frames, reduction requests, released manager state and
unbound calls fail explicitly. Only static images are integrated here.

15 runtime checks pass using synthetic TGA inputs through the original bitmap
loader: old/new configuration syntax, crop/half-texel UVs, tile/filter state,
cache identity, missing image, rejection paths and generation retention.
Full host ASan/UBSan suite: 39/39 passed. A cached executable lost its executable
state and was regenerated before the final run; local procfs requires disabling
LeakSanitizer. CI remains necessary for independent ARM64/GLES verification.

A reproducible complete-control link probe now runs in host CI. It includes all
frontend objects without section garbage collection and records actual unresolved
symbols, command and return code. Link failures with understood missing symbols
are diagnostic data, not a successful engine/menu check. Outputs containing
engine code stay external and are never uploaded. Current missing symbols: 12.

This work does not produce an original main-menu APK. Remaining dependencies
include theme/global interface setup, input, console, localization, dialogs,
sound and static/movie lifecycle integration, followed by live-root startup.
