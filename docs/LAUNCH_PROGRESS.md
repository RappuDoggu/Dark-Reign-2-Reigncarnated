# Original launch checkpoint

The owner confirmed v0.4 Android execution, then authorized the remaining visual
effect work and APK compilation/testing. v0.5 is now packaged; device execution
remains to be checked. See NATIVE_LAUNCH_V05.md for the receipt.
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
- Sun/moon sprites, original water texture with checkerboard waves, ground rain
  splashes and water rings. The original pinned SetupPerf disables the second
  water layer, so this port also renders one layer.
- Shell-selected smoke, flame, two-layer power glow and water-drop particles,
  using original config keys and model attachment points. Thunder/lightning uses
  the original sample and simulation cadence. Transparent model buckets are
  ordered by depth without disabling depth writes on opaque alpha-blend textures.
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

All 65 host tests pass (one restored executable needed its execute bit repaired,
then its previously unstarted test passed). The current ZIP suite passes 13 checks
including the real companion ZIP; the folder fixture passes 28 checks. A previous
full-installation run passed 38 checks. Three original intros decode completely,
including audio and resumed-frame/PCM equivalence. Seven combined scene/menu
frames match exactly across two software GLES contexts, including a lightning
flash and thunder event. The fixture yields 19 emitter layers and 23 water
clusters. Rendering the Java-imported ZIP layout matches the original fixture.

Still required to establish 1:1 launch: full cinematic/object simulation (including
scripted unit routes), water reflections, complete point/specular lighting and
legacy transparent pass ordering. Rain and particle distributions use deterministic
seeds; splat placement and particle integration are adapted rather than running the
entire original simulation. Ember ground collision uses a vertical normal. The
sun/moon may be outside the menu camera; code support does not prove device parity.
No Android device/emulator was available, so device startup, performance, AV sync
and lifecycle behavior are unverified for v0.5. Menu destinations remain disabled.

Pause/resume now checkpoints the active intro and its elapsed position, or the
shell camera/fade time. Restoration recreates GPU resources and rebuilds Bink
reference frames while discarding earlier PCM; reconstruction time is excluded
from the launch clock. Old queued audio callbacks are rejected across pauses.
All three original movies reproduce the same resumed frame and subsequent PCM
as uninterrupted host decoding. Reconstruction can delay return to a long clip;
there may be an audio gap at the resume boundary. Music restarts on return.
This checkpoint is Activity-local: process death/recreation starts a fresh launch.
Android lifecycle execution, audio focus and synchronization still need device
validation at v0.5 testing. Login is intentionally bypassed in this proof of concept.

## Decoder build

Run tools/build_bink_decoder.sh <external-root> host (or android with
ANDROID_NDK_HOME), then configure with REIGN_FFMPEG_ROOT pointing to its install
folder. FFmpeg n6.1.1 is checksum pinned; no GPL/nonfree components or networking
are enabled. Source and libraries remain external to this repository.

FFmpeg is LGPL 2.1 or later in this configuration. Any distributed static-linked
APK must include its license/attribution and be accompanied by corresponding
FFmpeg source/configuration plus application object files and instructions needed
to relink a modified decoder. The APK embeds the license and attribution. The accompanying decoder/relink ZIP
includes the pinned FFmpeg source, build configuration, application objects and
relink/repack instructions. Running its relink recipe reproduces the packaged
native library byte for byte. It excludes game data and signing keys.
