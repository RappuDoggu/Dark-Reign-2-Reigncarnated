# Source provenance and third-party notices — 0.1

This records the project's actual inputs and release materials. It is not a
claim that a PC distribution agreement's expiry makes copyright expire.

## Original engine

The port uses the separately fetched source at
`grasmanek94/darkreign2@9ce9da5f21604310a997f0c41e9cd383f5e292c3`, verified by the
repository's 1,791-file hash inventory. Original Pandemic Studios/Dark Reign II
copyright notices remain in source and generated overlays. No original game
asset packs or commercial middleware binaries are bundled with the Android app.

The project owner authorizes this release preparation and states that the source
was made public through Activision. Independently checked evidence on 2026-11-07:

- The [Google Code archive metadata](https://storage.googleapis.com/google-code-archive/v2/code.google.com/darkreign2/project.json)
  declares `license: mit` and attributes source acquisition to an ex-developer
  of Activision.
- The pinned [GitHub source mirror](https://github.com/grasmanek94/darkreign2)
  has no root licence identified by GitHub's repository metadata.

Those observations are recorded distinctly from the owner's statement; neither
an official corporate upload nor a specific rights-holder grant was independently
established by this metadata check. The dated Phase 0 audit is historical context,
not a blanket instruction to stop the owner-authorized port/build work. Preserve
attribution and supplied component notices; do not invent a licence for the
combined engine or describe licence expiry as copyright expiry.

## Android replacements

The native Android build uses the project's GLES/AAudio/platform adapters. It
does not ship the original Windows Miles Sound System or proprietary Bink runtime.
Movies/speech use the pinned FFmpeg build; original interface names in source do
not mean that those Windows middleware binaries are included.

FFmpeg **n6.1.1**, LGPL2.1 or later, is built with the reviewed minimal Bink/MP3
recipe in `tools/build_bink_decoder.sh`. GPL/nonfree/version3 features are not
enabled. Its full pinned source archive, licence, configuration, build recipe,
decoder libraries and both application libraries' link inputs are supplied in
`Reigncarnated-0.1-source-relink.zip`. The release validates exact library hashes
by relinking from a separately extracted copy. See its README and the licence
for modification, relinking and redistribution terms.

Existing APK notices include FFmpeg, LLVM and Android runtime notices. Preserve
them when rebuilding. Application objects in the source/relink kit have debug
information removed; development keys, raw diagnostics and unstripped native
libraries remain private. Port source, authored tests, build instructions and
release records accompany the kit; original engine source is fetched at its pin.

## Game data and project identity

Users import their own legally obtained Dark Reign 2 data. The APK/source kit
contains no `.zwp` game archives or original music/movies. Private data exports
must not be attached to public source/releases. Reigncarnated is a community port;
this documentation does not claim publisher endorsement or ownership of the
original game, brands or third-party components.
