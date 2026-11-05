# APK 28: JDA cinematic mesh failure fix

[Download APK 28](https://raw.githubusercontent.com/RappuDoggu/Dark-Reign-2-Reigncarnated/recovery/native-arm64-v012/builds/v0.22/Reigncarnated-Cinematic-Mesh-v28.apk)

Source: `69469fe5983f2d06ea91faf25db64cf6539532b5`.
SHA-256: `73a8e7b01806d93e23afc7697eb0455dc38f501489ba9eb3d40393fd6b505dd7`.
Size: 9,360,511 bytes (8.9 MiB).

Updates APK 22-27's **Reigncarnated IA City Test** with the same private signer;
keep the app/data. No game packs, raw model assets, private reports, keys or
unstripped symbols are included.

JDA's retained caller stack identifies the original raw mesh loader reading
unbound checkMaxVerts/checkMaxTris variables during its cycle-30 cinematic Mesh.
Bootstrap now binds the original vid.heap.check.verts/tris using actual heap
sizes. MeshRoot::Check remains active. The raw text loader's owned new[] buffers
also use matching delete[] cleanup in success and early-return paths.

The matching uncached original cinematic text-mesh regression now loads, renders
and cleans up twice without draw rejections; camera restoration and original
next/followup/final-menu tests pass twice. Reference Instant Action, 12 foundation
tests, 55 retained gesture checks, 13 report checks and SDK 35 Java also pass.
**Retest retail JDA Chapter 1** to confirm it proceeds and reveal any later issue.
This fixes the specifically captured missing binding, not a claim of complete
retail campaign validation.

Sprawler's APK 27 report confirms entry into Transition into S01 and 120 FPS at
collection. Remaining invalid UV draw rejections are unclaimed; reports now show
whether texturing was active. Placement, audio, original 10 Hz/ABI configuration
and fenced append-only streaming remain. No APK 28 phone benchmark is claimed.

[Diagnostic review](../../docs/DIAGNOSTICS_REVIEW_V27.md),
[build instructions](../../docs/RECOVERY_BUILD.md),
[progress](../../docs/RECOVERY_PROGRESS.md),
[verified receipt](../../docs/builds/cinematic-mesh-v28.json).
