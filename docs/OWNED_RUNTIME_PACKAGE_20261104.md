# Verified owned runtime package — 2026-11-04

The newly supplied full installation resolves the missing assets from the earlier
missions-only ZIP: base, shell, client sides, eight worlds, startup config,
multiplayer group metadata, original maps, AI personalities/rules and seven music
tracks. All 80 packs in the full-installation ZIP pass the existing inspector.
All three intro Bink movies and the billboard movie are inside base.zwp, so an
absent loose movies folder does not mean intros are missing.

The review branch is based on recovery/native-arm64-v012 revision
ff43d5b0f15e42e67f2d0e6d9eddf5b0562517c7. This is data packaging work; no engine,
renderer, timing, APK or gesture changes are included.

## Private download and identity

[Reigncarnated-Verified-Game-Data.zip](https://chatgpt.com/library?file_id=libfile_8670c189d0508191a633cae65184cafe&account_id=personal)

433,896,584 bytes; SHA-256:
`dffbc6b2516ff9c7416f9ebf8e0401fb7d41724052addf62d7f050e6635fbf0d`.

Contains 306 runtime files (574,377,162 uncompressed bytes), plus its packaging
manifest, seven music tracks, eight worlds and all 31 campaign Add targets.
Select this ZIP alone in the current app's game-data importer. Do not also select
the original two ZIPs; they have conflicting duplicate runtime paths.

The owning account must download/attach this private file to a separate Codex
task. A GitHub document link does not grant its workspace access to the bytes.
No proprietary asset bytes or signing material are committed to GitHub.

## Explicit merge policy

The complete installation is the first/preferred input. Forty-four differing
mission-path collisions retain that installation's version; seven overlapping
files are byte-identical. Additional custom maps come from missions.zip.
Every decision and per-file hash is in
[the receipt](builds/owned-runtime-package-20261104.json).

One malformed custom pack is explicitly excluded from the runtime ZIP:
`missions/multiplayer/!!!!skulkys_4aiv3ai+you_t-bone=).zwp`.
The repository Python inspector and Java importer both reject its archive name.
It remains preserved in the user's original missions.zip. No validation was
weakened. Sixty-three JPG map previews are not runtime inputs accepted by the
current importer/exporter, and remain preserved in that original ZIP. Desktop
executables/DLLs, installers and user files are excluded by the runtime allowlist.
The originals are unchanged. Seven supplied music tracks are preserved; this
package does not claim to recreate two additional tracks from another archive.

## Reproduce

Requires Python 3.11+ and the owned input ZIPs outside the source checkout:

```sh
python3 tools/package_owned_runtime.py \
  '/private/Dark Reign 2.zip' '/private/missions.zip' \
  --output /private/output/Reigncarnated-Verified-Game-Data.zip \
  --report /private/output/packaging-manifest.json \
  --exclude 'missions/multiplayer/!!!!skulkys_4aiv3ai+you_t-bone=).zwp=Rejected by existing pack validators: invalid archive name'
```

The tool enforces runtime pack requirements and storage limits, validates ZWP
structure/internal required names and campaign references, records all conflicts,
checks CRCs and publishes the completed ZIP atomically outside the checkout.
Content is reproducible; ZIP timestamps may change the whole-archive checksum
between builds. The published checksum above identifies the actual saved file.

## Validation and remaining device work

The real current Java importer accepted the assembled ZIP: seven tracks, three
scene packs. The real GameDataZip exporter then emitted all 306 runtime files
and seven tracks; the exported result was imported again successfully. Required
world/side/map/group/startup files were byte-identical after restore. Existing
exporter/importer/music tests passed 26/12/15 checks during the initial audit.
The output ZIP CRC scan passed.

These checks validate packaging and current runtime format. They do not prove
all custom maps can run, complete campaign/cutscene flow, audio correctness or
desktop fidelity. Continue the source recovery work on the recovery branch;
review the conflict receipt before choosing any alternative campaign versions.
No new APK was built for this data-only change.
