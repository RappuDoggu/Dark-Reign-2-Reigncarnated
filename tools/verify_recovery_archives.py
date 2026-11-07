#!/usr/bin/env python3
"""Verify saved recovery archives and that staged engine inputs are present."""
import argparse, hashlib, json, tarfile
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('installation',type=Path);p.add_argument('upstream',type=Path)
a=p.parse_args()
def digest(path):
 h=hashlib.sha256()
 with path.open('rb') as stream:
  for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
 return h.hexdigest()
expected=[(a.installation,'e12ede0e457d79dc1d5d4cf220521b537421a899b7ff02cdb9ee1805991f630d'),(a.upstream,'51e5ecfbb04c4d8fbd4b617ea15fc2d0351bafce0039fa71c725372d083acaf2')]
for path,checksum in expected:
 if digest(path)!=checksum:raise ValueError('Recovery archive checksum mismatch: '+str(path))
files={}
with tarfile.open(a.upstream,'r:gz') as archive:
 for member in archive.getmembers():
  if member.isfile():
   if not member.name.startswith('darkreign2-9ce9da5/'):raise ValueError('Unexpected source root')
   name=member.name.split('/',1)[1]
   if name in files:raise ValueError('Duplicate source member')
   files[name]=hashlib.sha256(archive.extractfile(member).read()).hexdigest()
reviewed=set()
for manifest in (Path(__file__).resolve().parents[1]/'cmake').glob('*hashes.json'):
 for name,value in json.loads(manifest.read_text()).items():
  if isinstance(value,str) and len(value)==64:
   candidates=[name] if '/' in name else [n for n in files if n.rsplit('/',1)[-1]==name]
   matches=[n for n in candidates if files.get(n)==value]
   if len(matches)!=1:raise ValueError('Reviewed input missing, ambiguous or changed: '+name)
   reviewed.add(matches[0])
print(json.dumps({'installation_sha256':expected[0][1],'source_sha256':expected[1][1],'source_files':len(files),'reviewed_source_files_present':len(reviewed),'desktop_files_required_for_current_inputs':False},indent=2))
