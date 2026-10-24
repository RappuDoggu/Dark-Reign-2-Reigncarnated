# Renderer Test v0.4 — Fold device check

This APK contains original porting adapters only, no original game code or
bundled assets. It does not start the game. Original font/IFace code is currently
host-tested and ARM64-compiled separately; it is not included in this public APK.

1. Install the local v0.4 APK over the previously delivered local v0.3 build.
   Their debug signing certificates match. CI generates its own debug key.
2. Open it. All 12 GPU checks should say PASS, including **Integer-coordinate
   glyph coverage**, **Offscreen presentation orientation**, and **Render-target
   resize and presentation**. No archive is needed for these checks.
3. Choose the existing base.zwp. The preview should show the same cursor atlas.
   Expected archive baseline: 2,074 entries, 99,631,656 payload bytes, 465 configs,
   334 GOD meshes, 567 textures.
4. Press Home and reopen. Check that GPU checks still pass and the cursor atlas
   returns if the same Activity survives. The report shows context generation and
   surface dimensions. Try folding/unfolding or rotation for resize behavior.
   If Android recreates the Activity or kills the process, reselect base.zwp;
   this diagnostic deliberately does not persist imported game assets.
5. Use Share test report and return the text/screenshot in this chat. Include any
   failure text or blank/inverted/cropped preview.

The v0.3 owner screenshot remains verified evidence for its nine earlier checks
on Adreno 740/GLES3.2. It does not establish that the new framebuffer presentation
path works on the Fold. v0.4 is a new runtime checkpoint, not a gameplay build.

Source/build evidence: FONT_PROGRESS.md and BUILD_STATUS.md. Proprietary
installation bytes stay local and are not CI fixtures or repository content.
