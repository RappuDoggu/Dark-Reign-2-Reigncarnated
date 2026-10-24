# Pinned controls ideas — deferred until single-screen gameplay

Current priority is original engine launch and rendering. Touch controls and AYN Thor dual-screen UI are not prerequisites for engine bring-up.

Touch candidates to prototype with the original selection/order model:

- Tap a unit to select; drag a selection rectangle in an explicit selection mode.
- Two-finger pan and pinch zoom keep camera gestures separate from issuing orders.
- Long press opens contextual orders; selected-unit commands remain visible and cancellable.
- Optional pen/stylus support can use hover for labels and a supported pen button for contextual orders. Ordinary touch must remain fully usable; no stylus feature is assumed available.
- Test UI scaling, selection accuracy and accidental orders on Fold 5 before committing to a gesture scheme.

Dual-screen work comes later: upper battlefield, lower minimap/unit/build/orders UI sharing one authoritative simulation. Probe actual Thor display APIs and touch routing on hardware; retain a complete single-screen mode. Do not split gameplay simulation between displays.
