# v0.7 performance test checkpoint

Source: 4bdab5af0e9c58c62f6d033db1f17190d5b19db6. APK SHA256: 9527160c7d594b6b36a99e71cb0ead8303ed58340d49e56e6c85f916d0fccc7b.

Host functional tests passed; device performance is NOT acceptable. Fold 5 feedback: menu ~1.4 FPS, mission 10–15 FPS, choppy movement, missing mission audio and low visual quality. A mission sample measured 95.1 ms inside the engine call, 81 draws/frame, 1 texture upload/sec and 0.1 ms presentation submission. These are wall timings, not CPU/GPU attribution.

Identified follow-up work, not yet implemented: menu visibility culling/batching, finer profiling, missing GameSound::Poll integration, and configurable render resolution. No post-v0.7 fix APK exists at this checkpoint.

Run `python reassemble.py` to reconstruct the recovery ZIP from five ordered binary parts. It contains source, objects, static dependencies, FFmpeg sources/license and relink scripts. The private signing key is excluded. Standalone unstripped libraries are omitted because the included objects can regenerate them. No game assets are included. See docs/PERFORMANCE_V07.md for validation details.
