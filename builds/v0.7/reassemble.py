from pathlib import Path
import hashlib
root=Path(__file__).resolve().parent
out=root/'Reigncarnated-v07-GitHub-Recovery.zip'
with out.open('wb') as f:
 for i in range(1,6):f.write((root/('recovery.zip.part'+str(i))).read_bytes())
print(out,hashlib.sha256(out.read_bytes()).hexdigest())
