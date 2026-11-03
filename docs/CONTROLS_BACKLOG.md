# Single-screen controls and remaining ideas

Version 17 adds two-finger pan and pinch zoom. Tap selects/orders through the
original mouse input path; one-finger drag retains rectangle selection. Starting
with two fingers avoids issuing a tap. Deselect calls the engine's discrete
deselect event. The Controls toggle exposes zoom, direction/building rotation
and the existing arrow-key buttons. Rotate changes the placement/order direction,
not the camera heading. All engine commands execute on the render thread and
the original simulation remains at 10 Hz. Gesture feel, placement rotation and
selection accuracy still need device testing.

Remaining touch candidates using the original selection/order model:

- Add an explicit selection-mode toggle if device tests show accidental orders.
- Long press opens contextual orders; selected-unit commands remain visible and cancellable.
- Optional pen/stylus support can use hover for labels and a supported pen button for contextual orders. Ordinary touch must remain fully usable; no stylus feature is assumed available.
- Test UI scaling, selection accuracy and accidental orders on Fold 5 before committing to a gesture scheme.

Dual-screen work comes later: upper battlefield, lower minimap/unit/build/orders UI sharing one authoritative simulation. Probe actual Thor display APIs and touch routing on hardware; retain a complete single-screen mode. Do not split gameplay simulation between displays.
