# 0.1 release preparation audit — 2026-11-07

## Published status

The owner explicitly authorized publication on 2026-11-07, superseding the earlier
do-not-publish instruction. PR #3 merged the recovered implementation and the
banner/landing-page work into main at `eca093e9019d9ffa84f8831ab22d340f77ff7d0b`.
PRs #1 and #4 are also marked merged because their commits are included.
[v0.1 is public](https://github.com/RappuDoggu/Dark-Reign-2-Reigncarnated/releases/tag/v0.1)
as an early-access prerelease. All four public assets were downloaded and match
the existing hashes; all 15 README destinations return HTTP 200. The live public
page contains the new tagline and exact SVG banner. Integration CI passes host
and Android jobs on `ea185e5...`. Signing keys and game packs remain private.

GitHub refused the proposed About-description and topic changes with HTTP403
"Resource not accessible by integration". The README/banner and release published
successfully. Detailed public verification is in
[release-0.1-github.json](builds/release-0.1-github.json).

## Repository and API state at the start of preparation

The restored executor works, the existing private update signer is present, and
GitHub API reads now succeed. The repository's default/main branch is the older
`0bc1fe7...` audit baseline. Current native implementation is on
`recovery/native-arm64-v012`, with APK31 source `e47230e...` and later documentation.
There are no existing Releases from the release-list read. Existing PR #1 is the
older dev/native-arm64-primitives branch; it is not silently replaced or merged.
Release preparation preserves recovery history, proposes the complete current
work for main through a separate reviewable PR and prepares a draft v0.1 release.
A draft and an APK in the recovery branch are not a published GitHub Release.

Delivery follow-up: draft PR #3 is open and mergeable. The v0.1 draft prerelease
targets delivery `1b55b7c...` and contains the APK, matching source ZIP, checksums
and receipt. Direct uploads returned HTTP401; the scoped draft-only Actions job
`42381059656` successfully attached them. Independently downloaded release assets
and public immutable raw links match all recorded hashes/sizes. GitHub's previous
Android renderer diagnostic failure was traced to a missing propagated `log`
dependency, fixed in `ae03dde...` and reproduced successfully with a clean local
package build. GitHub CI run `42381059560` also passes both host and Android jobs
on that fix, including the previously failing step and native frontend/decoder
compilation. See [GitHub verification](builds/release-0.1-github.json).

## Findings addressed

| Finding | Resolution |
| --- | --- |
| Android/public version mismatch | Package versionName 0.1, monotonically increasing versionCode 32, while preserving the existing package and signer. Launcher label becomes Reigncarnated. |
| Old menu-only relink materials | New portable kit includes both current menu and mission library inputs, full pinned FFmpeg source/configuration, port source and an unsigned APK for replacement/signing. Extract/relink and compare exact stripped hashes. |
| Stale landing page and proposed 0.0.1 notes | Make 0.1 early-access scope, installation/update path, downloads, source, checksum, player guide, roadmap and limitations explicit. Retain old notes as historical. |
| Source/licence claims conflated | Record the owner's position, verified archive MIT metadata and mirror evidence separately; preserve actual notices. No automatic copyright-expiry claim and no obsolete audit-only build prohibition. |
| New Java models missing from CI tests | Add report retention, music policy, campaign selection and resource parsing to the existing host job. Host tests do not stand in for device acceptance. |
| Renderer diagnostic CI link failure | Propagate Android log dependency from reign_gles; clean SDK35/NDK27 probe package/signature/alignment/CRC passes. |
| Release asset upload authentication | Attach only verified immutable candidate files to the existing draft through a contents-write Actions job token; independent release downloads match all hashes. |
| Main lacks current implementation | Prepare a separate reviewable recovery-to-main PR; do not merge or rewrite main automatically. |
| Incomplete feature claims | Keep full campaigns, saves, NaN UV rendering defects, device/audio verification, controller changes and networking clearly scoped. |

## StarDroid comparison

Reference: [kinopiowarrior/StarDroid](https://github.com/kinopiowarrior/StarDroid),
README/player guide and latest release API inspected on 2026-11-07. Latest release
observed: v1.5.4. Assets: APK (10,739,517 bytes), matching source ZIP (29,555,494
bytes), SHA256SUMS.txt. README links downloads, release notes, source, player guide
and bug reporting; states original game data is separate and explains update
without uninstalling. It also distinguishes its released source ZIP from an older
repository code snapshot. Those release practices are useful here.

Reigncarnated adopts a matching APK/source/checksum/receipt bundle, a player guide,
clear data-import/update instructions, controls table, explicit supported-feature
list and evidence-driven roadmap. It improves traceability by identifying the
exact source in the APK and the relink-kit manifest. StarDroid's nine languages,
Bluetooth/Wi-Fi Direct, controllers, save/replay and EUD features are not borrowed
claims; no implementation or licence equivalence is assumed.

## Acceptance boundaries

The gameplay/native sources are unchanged from APK31 for this packaging release.
Version identity, release tooling, notices/documentation and CI coverage change.
Existing native objects are verified against their recorded source and original
object hashes, debug-stripped and relinked for portable matching release inputs.
The original 10 Hz simulation, ABI flags, retained textures, geometry stream and
strict draw validation remain intact.

Run existing host/foundation/Java suites; rerun real-engine resource/campaign
fixtures where required by new changes. Verify pinned upstream/source archives,
signer continuity, manifest, APK source/documents, native hashes/build IDs,
ARM64/static libc++, 16KiB ELF/ZIP alignment and private/game-data exclusion.
Validate kit hashes again from an extracted archive in a different directory.
The release receipt records actual results. No new 0.1 phone execution, campaign
completion or phone performance result may be inferred from host checks.
