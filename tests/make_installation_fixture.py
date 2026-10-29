"""Small original test data; no game assets or copied configurations."""
from pathlib import Path
import struct
import sys
root = Path(sys.argv[1])
(root/'library/engine').mkdir(parents=True, exist_ok=True)
(root/'packs').mkdir(exist_ok=True)
(root/'library/engine/startup.cfg').write_text('StartupConfig() {}\nGameConfig() {}\n')
payload = b'ProbeConfig() { Value(42); Text("native fixture"); }\n'
name = b'probe.cfg'
foff = 56+len(payload)
entry = struct.pack('<IB',0,len(name))+name+struct.pack('<II',56,len(payload))
header = struct.pack('<4s5I32x',b'NORK',1,0,foff+len(entry),1,foff)
(root/'packs/base.zwp').write_bytes(header+payload+entry)
