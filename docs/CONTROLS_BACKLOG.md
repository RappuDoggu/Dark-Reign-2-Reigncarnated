# Single-screen controls and remaining ideas

Version 18 implements the owner's immediate touch-control choices. Controller
changes and control-group design are deferred.

| Action | Touch control |
| --- | --- |
| Rotate map | Twist two fingers clockwise/counterclockwise. Pinching does not zoom. |
| Pan map | Drag two fingers together; the gesture stays in pan mode until lifted. |
| Zoom | Camera button opens separate Zoom +/− buttons. |
| Select / ordinary left click | Tap the scene; drag retains rectangle selection. |
| Aim a building | After choosing a building, tap/drag moves its preview without placing. |
| Rotate building | Build ↶ / ↷ changes the original placement direction. |
| Place / explicit left click | Left click confirms at the last world cursor position. |
| Quick deselect / cancel | Deselect clears selection and exits forced placement/order mode. |

Left click and Deselect remain fixed in the top toolbar. Building rotation,
Camera and Debug are in its scrollable remainder on narrow displays. Building
preview mode is detected from the original `Client::CM_CONSTRUCT`, with an
on-screen hint; unit movement/orders still use the original left-click path.
The two-finger classifier locks either rotation or pan for each gesture and
does not emit wheel input. Finger-ID changes, angle wrapping and lifting the
remaining finger must not cause stray clicks. The original Standard camera's
`camera.rotateby` performs map rotation; its Circle path changes yaw without
calling Swoop. Unsupported camera types produce a visible error.

Host gesture checks pass; physical gesture feel, map pivot/terrain behavior and
building placement/rotation still require the owner's device test.

## Earlier controls

Version 17 introduced two-finger pan and pinch zoom. Tap selects/orders through the
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
