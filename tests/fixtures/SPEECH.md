Authored test tones, generated with FFmpeg lavfi sine and libmp3lame. No game assets.

    ffmpeg -f lavfi -i sine=frequency=440:sample_rate=44100:duration=0.5 -ac 1 -codec:a libmp3lame -b:a 64k -map_metadata -1 speech-mono.mp3
    ffmpeg -f lavfi -i sine=frequency=660:sample_rate=22050:duration=0.5 -ac 2 -codec:a libmp3lame -b:a 64k -map_metadata -1 speech-stereo.mp3

These fixtures cover MPEG-1 and MPEG-2 Layer III, mono duplication, stereo output,
and queued original Reserved voice playback; they are never packaged in the APK.
