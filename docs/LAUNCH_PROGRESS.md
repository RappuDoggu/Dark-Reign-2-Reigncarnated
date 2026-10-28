# Original launch checkpoint

The owner confirmed v0.4 Android execution and requested no further incremental
APK tests. This checkpoint is source/compilation work, not a new APK release.
The menu is native geometry; only the game's original introduction and billboard
assets use video decoding.

## Implemented

- Numbered original intro-movie Bink assets from imported base.zwp, in order,
  with original audio decoded to stereo PCM and Android AudioTrack streaming.
  Tap skips the current clip. Shell music/looping rain ambience starts afterward.
- Original entrance camera, initial black hold/fade, and delayed menu activation.
- Environment color keys and sun direction, ambient/directional model lighting,
  original fog settings, sky mesh and texture, drifting cloud texture, rain
  streaks, terrain texture animation and Bink billboard playback.
- ZIP/folder import retains supported runtime trees, including other worlds,
  missions, sides and rule packs. Additional ZWP tables are validated. Windows
  executables are excluded; imported mods are retained, not automatically enabled.
- Original menu destinations remain disabled for the proof of concept. Import
  setup is still the existing Android screen; a replacement splash comes later.

Original source is pinned to 9ce9da5f21604310a997f0c41e9cd383f5e292c3.
The launch source hash manifest records the reviewed environment, camera and
weather implementations. This is a port with adaptations, not a claim that every
render path is already the original implementation.

## Validation and limits

All 65 host tests pass. ZIP import passes 12 checks; importing the full owned
installation passes 38 checks, retaining nine music tracks and supported runtime
trees. Three original intros decode completely on the host, including audio (about
15.4, 7.9 and 77.1 seconds). Combined scene/menu/fade rendering is identical across
two recreated software GLES contexts. Android ARM64 JNI and Java compilation are
checked. These checks do not establish device audio sync or visual equivalence.

Still required for 1:1 launch: original ambient particle emitters, rain ground and
water splashes, sun/moon sprites, water rendering, complete material and light
policy, and full cinematic/object simulation. Rain distribution is deterministic
for tests and its projection is adapted. Point lights, specular response and all
legacy pass ordering are not restored. Pause/resume now checkpoints the active intro and its elapsed position, or the
shell camera/fade time. Restoration recreates GPU resources and rebuilds Bink
reference frames while discarding earlier PCM; reconstruction time is excluded
from the launch clock. Old queued audio callbacks are rejected across pauses.
All three original movies reproduce the same resumed frame and subsequent PCM
as uninterrupted host decoding. Reconstruction can delay return to a long clip;
there may be an audio gap at the resume boundary. Music restarts on return.
This checkpoint is Activity-local: process death/recreation starts a fresh launch.
Android lifecycle execution, audio focus and synchronization still need device
validation at the next agreed APK milestone. Login is intentionally bypassed in this proof of concept.

## Decoder build

Run tools/build_bink_decoder.sh <external-root> host (or android with
ANDROID_NDK_HOME), then configure with REIGN_FFMPEG_ROOT pointing to its install
folder. FFmpeg n6.1.1 is checksum pinned; no GPL/nonfree components or networking
are enabled. Source and libraries remain external to this repository.

FFmpeg is LGPL 2.1 or later in this configuration. Any distributed static-linked
APK must include its license/attribution and be accompanied by corresponding
FFmpeg source/configuration plus application object files and instructions needed
to relink a modified decoder. The packaging script embeds the license; preparing
and verifying the relink materials remains a release requirement. No APK with
this dependency is distributed by this checkpoint.
