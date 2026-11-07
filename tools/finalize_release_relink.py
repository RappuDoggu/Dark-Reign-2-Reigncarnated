#!/usr/bin/env python3
"""Finalize a current source/relink kit with the unsigned APK and hash manifest."""
import argparse,hashlib,json,shutil,zipfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--kit',type=Path,required=True);p.add_argument('--unsigned-apk',type=Path,required=True);p.add_argument('--receipt',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
a=p.parse_args();root=a.kit.resolve();output=a.output.resolve()
if root==output or root in output.parents:p.error('Write the ZIP outside the kit directory')
receipt=json.loads(a.receipt.read_text());inputs=json.loads((root/'kit-inputs.json').read_text());assert receipt['source_revision']==inputs['release_source_revision']
with zipfile.ZipFile(a.unsigned_apk) as z:
 assert z.testzip() is None
 for name,digest in receipt['native_libraries'].items():assert hashlib.sha256(z.read('lib/arm64-v8a/'+name)).hexdigest()==digest
shutil.copy2(a.unsigned_apk,root/'unsigned.apk');shutil.copy2(a.receipt,root/'release-receipt.json')
manifest={}
for f in sorted(root.rglob('*')):
 if not f.is_file() or f.name=='SHA256.json':continue
 assert f.suffix.lower() not in ('.keystore','.jks','.zwp','.bik','.trace') and not f.name.endswith('.unstripped.so'),f
 manifest[f.relative_to(root).as_posix()]=hashlib.sha256(f.read_bytes()).hexdigest()
(root/'SHA256.json').write_text(json.dumps(manifest,indent=2)+'\n')
output.parent.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(output,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as z:
 for f in sorted(root.rglob('*')):
  if f.is_file():
   info=zipfile.ZipInfo(f.relative_to(root).as_posix(),(1980,1,1,0,0,0));info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,f.read_bytes())
with zipfile.ZipFile(output) as z:assert z.testzip() is None
print(json.dumps({'file':output.name,'bytes':output.stat().st_size,'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),'verified_members':len(manifest)},indent=2))
