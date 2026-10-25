# REA / Ghidra workspace verification

Date: 2026-10-25. **Toolchain installed; standalone Ghidra decompilation verified; REA provider integration blocked.**

## Installed and executed

A fresh isolated toolchain was installed by `tools/setup_rea.sh` at:

`/workspace/scratch/2e29d93ded9e/reign-work/rea-toolchain-verified`

- Node 24.22.1 (existing supported workspace runtime), REA CLI 6.8.0 from the official npm `rea-agents` package.
- Full Temurin JDK 21.0.13+9, including javac.
- Ghidra 12.1.4, including the Linux x86_64 native decompiler.
- Both release archive SHA256 checks pass. Exact package URL/npm integrity and release hashes: [installed metadata](logs/rea-workspace/installed-toolchain.json).
- npm installation, CLI version, java/javac versions and standalone decompilation executed. Setup exits **1 at REA doctor**; that is a failed readiness gate, not a successful integration.

The initial disconnected-workspace blocker is resolved. The earlier official GitHub `v6.8.0` release URL returned 404, but actual npm installation now confirms that `rea-agents@6.8.0` is published and installable.

## Actual REA output

Commands used the new prefix's env.sh and the owner's original installation executable:

```bash
source /workspace/scratch/2e29d93ded9e/reign-work/rea-toolchain-verified/env.sh
rea doctor --provider ghidra --json
rea decompile '/workspace/scratch/2e29d93ded9e/reign-work/installation/Dark Reign 2/dr2.exe' 0x004c3930 --provider ghidra --json
rea providers --json
```

Doctor fails with `capability_unavailable`; decompile and providers exit 1 with `execution_failure`. Actual common message:

```text
Linux process ownership inspection requires a procps-compatible ps on REA's PATH before launching a child: Command failed: ps -axo pid=,ppid=,pgid=,uid=,stat=,command=
fatal library error, lookup self
```

Raw [doctor](logs/rea-workspace/rea-doctor.json), [decompile](logs/rea-workspace/rea-decompile.json), [providers](logs/rea-workspace/rea-providers.json). Investigation found processes reporting namespace PID 5/6 while /proc/self resolved to host PIDs in the thousands. procps is installed; this is an execution-environment PID/procfs mismatch. Creating a private namespace with bwrap also failed with Operation not permitted. No ownership checks, process identities, ps output or REA code were patched or bypassed. Repairing the workspace's PID/procfs view remains required for REA provider startup. No MCP registration or tool integration has been established.

## Actual standalone Ghidra verification

Imported the supplied **dr2.exe**, not a recompiled fixture, into a new independent DR2Startup project. Ghidra auto-analysis completed in 134 seconds. Then repeated decompilation using the freshly installed toolchain with the project opened **read-only**, without re-analysis.

Actual Ghidra script output:

```text
DR2_STARTUP_DECOMPILE address=004c3930 run_codes=[Intro, KeyCheck, Login, Mission, Outro, Shell, Studio] characters=659
```

[Machine-checked verification](logs/rea-workspace/ghidra-verification.json), [import log](logs/rea-workspace/ghidra-import.log), [fresh toolchain/read-only log](logs/rea-workspace/ghidra-readonly-verification.log).

Executable SHA256: `5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250`; Ghidra language `x86:LE:32:default`. Function `0x004c3930` references all seven run-code names, registers them in precisely the order in `appdr2/winmain.cpp`, and returns zero. This strongly identifies WinMain by source comparison. It does **not** prove complete equivalence of the community executable to the pinned source. Decompiler names/types are inferred and the 659-character output is pseudocode, not recovered original source.

Private pseudocode and the new project remain in `reign-work/decompilation-owned-toolchain/` and `reign-work/dr2-ghidra-project/`, outside the repository. No command in this DR2 work opened or migrated the existing Magic 2014 projects. A private inventory covered 19 of their files; a later comparison detected four database/journal files changed during the session. Attribution is not established (no active matching Java process remained at inspection), so byte-for-byte preservation cannot be claimed. Those files were not restored or overwritten. DR2 uses its own separately named project, toolchain prefix and evidence directories. Source/asset/executable bytes and pseudocode are excluded from the commit.

## Reproduce

Official REA requirements were read at [source checkpoint 25f0f99](https://github.com/morluto/rea/tree/25f0f9934d226caa62fb89d68e7bacad74f4fe73). The installation guide accepts Ghidra 12.1.x and documents upstream verification of 12.1.4 with JDK 21. Node requirement is `^22.23.4 || ^24.22.1 || >=26.12.1`; Linux also requires procps-compatible ps. REA setup itself does not install Java/Ghidra.

```bash
# Use a nonexistent absolute prefix. Existing installations are never overwritten.
bash tools/setup_rea.sh /absolute/new/toolchain
source /absolute/new/toolchain/env.sh
```

Optionally set `REIGN_REA_DOWNLOAD_CACHE` to a directory containing the exact official archive filenames. Cache copies are still checked against pinned hashes. The actual Work run used the existing verified download cache, then extracted fresh installations and performed a fresh npm install. Versions/logs/env.sh/package-lock.json are retained even when doctor fails. Node/npm, curl, tar, unzip, sha256sum and ps must already be available. Run bash -n and Python py_compile for the checked-in scripts; these checks pass in Work.

For standalone Ghidra, create fresh project and private evidence directories, and use the supplied owner executable and external source checkout:

```bash
mkdir /absolute/new/project-directory /absolute/new/private-evidence
"$GHIDRA_INSTALL_DIR/support/analyzeHeadless" /absolute/new/project-directory DR2Startup \
 -import /absolute/owner/dr2.exe -scriptPath "$PWD/tools/ghidra" \
 -postScript DR2StartupProbe.java /absolute/new/private-evidence \
 -analysisTimeoutPerFile 240 -max-cpu 2
python3 tools/verify_ghidra_startup.py /absolute/owner/dr2.exe /absolute/external/upstream /absolute/new/private-evidence
```

The helper compares the real executable hash, decompiler completion, nonempty output and ordered registrations to the inspected source. An initial verifier error counted the commented-out MeshView registration; stripping source comments corrected that verifier without changing the observed decompilation. Review structured results: a Ghidra launcher exit alone is insufficient.

To repeat against the resulting project safely, use `-process dr2.exe -noanalysis -readOnly` instead of `-import`, with a new evidence directory. Never point the import command at existing analysis. Once REA readiness works, `tools/verify_rea_function.py` records separate real provider evidence and refuses an existing evidence directory. Standalone success must not be relabeled REA success.

## Main-menu APK status

The executable, assets, source, host compiler and Android NDK/SDK are available. The remaining main-menu blocker is engine implementation/integration, not another owner asset upload. Original control declarations and five coordinate/border methods now compile natively, with 15 host checks of original UI types and successful ARM64 compilation; full IControl, IFace startup, Main/GameGod and menu controls are not linked. No original-menu APK or gameplay launch is established. See [control progress](CONTROL_PROGRESS.md).


## Follow-up troubleshooting — 2026-10-25

Reproduced the provider prerequisite failure in ordinary and PTY execution. Node PID6 resolved to /proc/self=14462; PTY PID5 resolved to14549. procps exited1 with `fatal library error, lookup self` in both. Inspected installed REA ProcessOwnershipObservation.js: it reads /proc/PID/stat, status and environ directly, so a ps wrapper alone cannot repair ownership validation. Docker and Podman executables are absent. A private user/mount/PID namespace with unshare --mount-proc was attempted; it failed with `unshare: write failed /proc/self/uid_map: Operation not permitted`. The earlier bwrap namespace attempt was denied. No process-ownership checks were disabled.

Full original control compile was repeated with native declarations and still fails at Pix/VertexTL and subsequent input/font/utility dependencies. No further menu functionality or APK runtime success resulted from this troubleshooting. Standalone Ghidra remains the usable analysis path. A normal Linux execution host with matching PID/procfs visibility is the concrete REA prerequisite; no additional game files are needed.

Scheduling remains uncertain before full control/IFace linkage. A provisional engineering allowance is 2–6 focused developer-weeks for the original menu, with substantial overrun risk from startup dependencies; this is a planning estimate, not a measured delivery forecast or a promise of background execution. Re-estimate after the first original interactive window and an integrated desktop startup loop.
