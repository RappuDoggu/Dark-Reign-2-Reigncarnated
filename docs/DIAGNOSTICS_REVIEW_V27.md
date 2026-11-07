# APK 27 diagnostic review: JDA cinematic mesh failure

The private report matches APK 27 source
`99efd4bfd00b6143ef7c9d9562cfb3c6d7313390` and its exact native library hashes.
Failed-mission retention now captures JDA J01 with its source/time and caller
stack. No raw device reports, images, traces or game assets are committed here.

## Confirmed JDA cause

J01 reaches simulation cycle 30, with its cinematic active and no movie active.
The timeline dispatches SetBookmark, Action, Letterbox and Mesh. The retained
stack resolves against the matching private unstripped mission ELF:

1. VarInteger::operator*()
2. Mesh::Manager::FindRead(const char *, const char *), meshread.cpp:3007
3. Viewer::Action::Mesh::Mesh, viewer_action.cpp:1920
4. Cineractive command dispatch / GameTimeSim / mission frame

The original raw text-mesh loader calls
`root->Check(MAXBUCKYS, *Vid::Var::checkMaxVerts, *Vid::Var::checkMaxTris * 3)`.
The native bootstrap bound other video/mesh variables but omitted these two.
They are live original VarInteger objects with no target, so dereferencing them
throws exactly the invalid VarPtr error in the device report. The original
Vid::Command::Init binds vid.heap.check.verts/tris from renderState.maxVerts/
maxTris. APK 28 creates these same bindings after the actual native heap sizes
are established. MeshRoot::Check stays enabled, with the actual heap limits;
no fallback values, geometry-validation bypass or exception suppression is used.

## Reproduction and adjacent cleanup

An optional authored cinematic fixture copies engine_plane.xsi from the existing
owned base pack to a private temporary alias, ensuring it is uncached. The real
original cinematic Mesh primitive then loads it through the raw text parser and
reaches the same mesh check. No model bytes enter source control or the APK.

This new path first exposes mismatched new[]/scalar delete cleanup under ASan.
The port now uses delete[] for the loader's owned text, vector, mapping and
vertex-index arrays in successful and early-return cleanup. Feature objects
allocated singly remain scalar deletes; original layouts and parse/validation
logic are preserved. With array cleanup corrected but bindings still absent,
the fixture reproduces the same invalid VarPtr. Adding the original bindings
makes the cinematic, render and cleanup path pass twice. Allocation-mismatch
checks were not disabled; only the existing ptrace-sandbox leak setting is used.

## Sprawler and rendering evidence

The other current report is Transition into S01, with 118 simulation cycles in
about 12 seconds and 120 FPS at collection. The original intro-to-transition
handoff therefore progressed on the phone. The report does not establish full
transition completion, S01 gameplay or complete campaign progression.

Sprawler still records rejected draws with invalid UVs/NaN. APK 28 adds the
active-texture flag to rejected-draw diagnostics so future evidence can show
whether that UV channel was used. Rendering validation is otherwise unchanged;
invalid operations are not silently accepted, sanitized or dropped from reports.
No fix for those remaining UV rejections is claimed by this patch.

## Validation and limits

The real uncached text-mesh cinematic, camera restoration, original NextMission
handoff, followup load and final-menu route pass twice in the host harness. The
reference Instant Action lifecycle also passes twice. All 12 foundation tests,
55 retained gesture checks and 13 report-retention checks pass. The full pinned
1,791-file source check passes; changed original meshread.cpp and ARM64 mission
adapters are rebuilt and linked. SDK 35 Java is compiled during packaging.

The cinematic test uses a real owned reference mesh and an authored timeline,
not retail J01 data. This fixes the specific missing binding identified by the
phone stack; retest JDA Chapter 1 to verify it proceeds and expose any later
issue. Anchored placement, original 10 Hz simulation/ABI flags, supported High
graphics, audio behavior and fenced unsynchronized streaming are retained.
No new phone FPS or full retail campaign completion result is claimed.
