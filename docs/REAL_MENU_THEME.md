# Recovered installation and real menu theme integration

The previously supplied Dark Reign 2.7z was recovered, not replaced by a new disc
installation. Extraction produced 496 files / 469,820,640 bytes externally.
The executable SHA-256 matches the earlier analysis input exactly:
5ea4734ab2f2ad381877ce1c0d9b03365ddb3dd8e3ff0719a634296be1c15250.
No assets or executable bytes are committed. Input hashes are recorded under
logs/real-menu-theme.

The original-tooling extractor uses system libarchive and rejects path traversal,
links, overwrites, invalid member sizes and special files. Five synthetic tests
verify byte extraction, traversal/link rejection and preservation of existing
files/partial files. Reproduction requires Python 3 and system libarchive:

```
python3 tools/extract_installation_7z.py /external/Dark-Reign-2.7z /external/install
python3 tests/test_installation_extract.py
```

frontend_theme_probe parses with original PTree/FScope, reconstructs typed scopes
from the parsed tree, and executes the original theme configuration against the
original bitmap-manager slice. It deliberately counts other statements without
executing them; it is not the game startup interpreter. Reproduce after building:

```
ASAN_OPTIONS=detect_leaks=0 /external/build/frontend_theme_probe /external/install/packs/base.zwp runcode_main_login.cfg interface_shell_mainmenu.cfg
```

Actual results:
- Standard theme: 9 groups, 14 skins, 1 bitmap.
- Login configuration: 12 groups, 14 skins, 1 bitmap, 121 other statements not executed.
- Login then shell-menu configuration: 21 groups, 14 skins, 2 bitmaps, 183 other statements not executed.
- Shell-menu configuration alone failed the probe's coverage requirement because
  it assumes earlier standard initialization; the sequential test preserves that
  ordering rather than treating the isolated failure as corrupt assets.

The original shell runcode starts a shell mission, not merely a static UI screen.
Main-menu readiness therefore also needs real root/control creation, command and
localization services, event routing, scene/frame presentation and lifecycle
verification. No engine startup, displayed menu or gameplay has been tested.
The user does not need to re-upload assets or test another renderer-only APK now.

Texture identity handling was tightened: when the original manager cannot find
the loaded image's animation ring (including its known directory-key mismatch),
the bridge rejects it rather than silently freezing a potentially animated
image. The focused texture test now passes 16 checks.
