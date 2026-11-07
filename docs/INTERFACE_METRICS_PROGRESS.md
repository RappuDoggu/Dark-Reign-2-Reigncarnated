# Original interface metrics checkpoint — 2026-10-24

Original IFace metric defaults, configuration, reads and screen updates now execute alongside original font registration. This supplies another control-startup dependency, not a running interface or menu.

## Inspected implementation and native boundaries

`tools/prepare_interface_metrics.py` verifies hashes for `interface/iface.cpp` and `interface/iface_types.h`, then stages the original MetricType enumeration and InitMetrics, ConfigureMetrics, UpdateScreenMetrics and GetMetric functions outside this repository. Metric table values and CRC lookup entries remain the original implementation. The source is not redistributed here.

Native scoped state replaces process-global arrays. The three Vid::backBmp dimension reads now use caller-provided logical viewport dimensions and the RGBA8 render target's32-bit depth. Resize calls the original UpdateScreenMetrics and preserves configured custom values. This does not query an Android window or initialize a graphics device. Calls on one metrics object must be serialized.

FontRegistry::configure accepts an optional InterfaceMetrics consumer. The same original PTree/FScope parse dispatches CreateFont and Metrics, in declaration order at top level or directly inside ConfigureInterface. Original ConfigureMetrics handles integer values and string-to-CRC font references, including its original unknown-name/unsupported-type diagnostics. Other interface configuration remains deliberately unexecuted. Supplying no metrics consumer retains the previous font-only behavior.

The native string-name accessor maps through the original CRC table before calling GetMetric; invalid/unknown names fail explicitly. Font reference resolution uses original FontSys CRC lookup and the existing registry snapshots. TitleFont and ThinTitleFont are the recognized font-valued metrics; missing aliases return null. Screen dimensions come from the original dynamically populated metric slots. The original MAX_CUSTOM_METRICS sentinel leaves a reserved slot before SCREEN_WIDTH; the extracted enumeration preserves those indexes rather than compacting the table.

## Actual verification

- Host ASan/UBSan:19 CTest targets PASS. New original_interface_metrics target has33 checks covering all14 non-font custom defaults, configured font CRCs, overrides, signed values, unknown/float behavior, required argument errors, top-level and wrapped configuration, resize/preservation, registry reload, missing aliases and independent state.
- Supplied interface_standard_fonts.cfg: original PTree/FScope produces nine aliases using six files. Original Metrics resolves TitleFont to WindowTitle and ThinTitleFont to System. ShadowAlpha remains the original default88. The1692x666 viewport is a host fixture matching the owner's reported dimensions, not an Android execution of this slice.
- All42 software GLES checks PASS in each of two recreated EGL contexts. New check resolves a configured font reference, reads configured shadow alpha77 and checks the resulting original Font/IFace/Vid output. CPU font/metrics configuration and original draw behavior remain involved; this is not a recreated menu.

No compiler failure or unexpected runtime failure occurred in the local host/GPU attempts. Expected malformed-config diagnostics are part of the tests. Build and test logs are retained in docs/logs/interface-metrics; ARM64 compilation and CI results are recorded in BUILD_STATUS after completion. No new original-derived Android runtime evidence is claimed.

## Remaining frontend work

Original bitmap manager ownership/reload/animation/mipmaps, interface colors/skins/control class registration, frame/event orchestration and Main/GameGod services are unfinished. A real original menu requires those systems to initialize together. M3 and playable missions remain unachieved; touch/pen and dual-screen stay deferred. The owner-tested public v0.4 adapter APK remains unchanged and contains no original engine implementation or game assets.
