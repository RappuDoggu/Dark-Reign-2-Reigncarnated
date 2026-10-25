# REA / Ghidra workspace setup checkpoint

Date: 2026-10-25. Status: **BLOCKED — installation and decompilation not verified**.

## Actual execution result

The Work execution service rejected the attempted `pwd`, `command -v rea`, `command -v ghidraRun`, and `command -v java` invocation before a shell started:

```text
No usable execution environment is available. Wait for an environment to become available before using this tool.
```

This is execution-service output, **not REA provider output**. No REA process, Ghidra bridge, JDK installation, provider doctor, or Dark Reign 2 decompilation ran during this checkpoint. No provider result exists to report. GitHub API access remained available and was used to preserve these instructions and original tooling. No existing analysis was opened, migrated, deleted, or modified; filesystem preservation could not be independently inspected while execution was unavailable.

## Official requirements inspected

- [morluto/rea source checkpoint](https://github.com/morluto/rea/tree/25f0f9934d226caa62fb89d68e7bacad74f4fe73), package.json version 6.8.0, npm package `rea-agents`, CLI `rea`.
- [Official installation guide](https://github.com/morluto/rea/blob/25f0f9934d226caa62fb89d68e7bacad74f4fe73/docs/installation.md#ghidra): Ghidra **12.1.x**; upstream documents bridge verification with **12.1.4 and JDK 21**. Full 64-bit JDK including javac, matching host architecture/native decompiler required. REA setup does not install Ghidra or Java.
- Node `^22.23.4 || ^24.22.1 || >=26.12.1`; npm; Linux procps-compatible ps.
- [Official Ghidra release](https://github.com/NationalSecurityAgency/ghidra/releases/tag/Ghidra_12.1.4_build): `ghidra_12.1.4_PUBLIC_20260921.zip`, SHA256 `ddac49f903da9d5bac833e5cc79395098b9c33cfd3279be5f31bd00387d2d4db`.
- [Official Temurin JDK release](https://github.com/adoptium/temurin21-binaries/releases/tag/jdk-21.0.13%2B9): Linux x64 JDK21.0.13+9 archive, SHA256 `d7f5d5cf865a2902b70ff549b97586a7d3f234e016f041ab447ea740c7d66d1c`.
- Release metadata and hashes were read through GitHub, **download bytes were not fetched or verified locally**.
- REA's `v6.8.0` GitHub release URL returned 404. package.json version is not proof that npm has published it. The pinned npm installation below remains untested and must fail visibly if unavailable; do not silently substitute another release.

## Reproduce when Work execution returns

The scripts are unexecuted recipes, not a successful setup report. Linux x86_64 is the only installer target. Provision a supported Node/npm runtime first through official Node distribution tooling. Do not reuse an existing installation directory.

```bash
bash tools/setup_rea.sh /absolute/new/reigncarnated-rea-toolchain
source /absolute/new/reigncarnated-rea-toolchain/env.sh
```

This installs JDK/Ghidra into the new prefix, checks the pinned download hashes, installs the exact npm package version into that prefix, retains package-lock.json, captures version/doctor/providers output, and leaves user profiles/MCP registrations untouched. It requires network access to GitHub release downloads and npm. Retain failed-install logs and use a new prefix for a subsequent attempt rather than deleting existing material.

Before analysis, inventory existing .gpr/.rep projects, REA snapshots, and notes. Make backups before intentionally opening anything with a newer Ghidra. Verification should use an independent target copy and fresh output directory; do not pass an existing analysis snapshot, purge caches, or delete an existing project.

Use the owner's original installation executable, record its SHA256, and keep it outside the repository. The previously described community executable has not been proved identical to the pinned source. No verified function address is currently recorded. Inspect the real provider's analysis/search output to locate a candidate, then establish identity from instructions, call sites, and source comparison.

```bash
rea analyze /absolute/copy/dr2.exe --provider ghidra --json > /absolute/new-evidence/overview.json
rea search /absolute/copy/dr2.exe 'source-backed identifying string' --provider ghidra --json > /absolute/new-evidence/search.json
python3 tools/verify_rea_function.py /absolute/copy/dr2.exe CONFIRMED_FUNCTION_ADDRESS /absolute/new-function-evidence
```

Create the overview directory first with mkdir without -p, so an existing directory is rejected. Replace the example search text and address using actual discoveries, not guessed addresses. The verification script refuses an existing output directory, records target SHA256, exact argv, exit codes, stdout/stderr, doctor, providers, function dossier, and decompilation. It does not mark function identity confirmed automatically.

A useful source-backed target is `WinMain` compared against `appdr2/winmain.cpp`, or a CRC routine compared against `system/crc.cpp`; neither has an established installation-binary address here. A recompiled engine_smoke function can provide an additional symbol-rich tool fixture, but must be labeled **source-compiled fixture**, not verification against the original installation executable.

## Acceptance evidence still required

1. Actual installed versions and complete successful Ghidra doctor/provider output.
2. Actual nonempty decompiler result, target SHA256 and selected address.
3. Source comparison proving this is the intended Dark Reign 2 function; explain uncertainty with community-build provenance.
4. Preserve raw results privately and review licensing before committing pseudocode, executable bytes, or other derived proprietary content. Commit a concise findings report and nonproprietary metadata.
5. CLI success is distinct from MCP client registration. No MCP integration has been attempted or established here.

No tests or syntax checks of the new scripts ran in this checkpoint. No APK, engine/menu milestone, or runtime result is implied.
