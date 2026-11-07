#!/usr/bin/env python3
"""Emit installation metadata/hashes only; no asset content or player details."""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
from inspect_pack import inspect

def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def audit(root, source_archive=None):
    files = [p for p in root.rglob('*') if p.is_file()]
    result = dict(files=len(files), bytes=sum(p.stat().st_size for p in files), packs=[], executables=[])
    if source_archive:
        result['input_archive_sha256'] = sha256(source_archive)
    for path in sorted(root.rglob('*.zwp')):
        if path.relative_to(root).parts[0].lower() == 'users':
            continue
        with path.open('rb') as stream, mmap.mmap(stream.fileno(),0,access=mmap.ACCESS_READ) as data:
            meta = inspect(data)
        result['packs'].append(dict(path=path.relative_to(root).as_posix(), bytes=path.stat().st_size,
            sha256=sha256(path), entries=len(meta['files']), payload_bytes=sum(x['size'] for x in meta['files'])))
    try:
        import pefile
    except ImportError:
        result['pe_metadata_status'] = 'pefile not installed; executable inspection skipped'
        return result
    for name in ['dr2.exe','DDraw.dll','D3DImm.dll','mss32.dll','binkw32.dll']:
        path = root/name
        if not path.is_file():
            continue
        pe = pefile.PE(str(path))
        versions = {}
        for group in getattr(pe,'FileInfo',[]):
            for info in group:
                for table in getattr(info,'StringTable',[]):
                    for key,value in table.entries.items():
                        if key.decode(errors='replace') in ['FileVersion','ProductVersion','FileDescription','Build Date']:
                            versions[key.decode()] = value.decode(errors='replace')
        result['executables'].append(dict(name=name, sha256=sha256(path), machine=hex(pe.FILE_HEADER.Machine),
            versions=versions, imports=[x.dll.decode(errors='replace') for x in getattr(pe,'DIRECTORY_ENTRY_IMPORT',[])]))
        pe.close()
    return result

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('installation', type=Path)
    p.add_argument('--source-archive', type=Path)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    args.output.write_text(json.dumps(audit(args.installation.resolve(),args.source_archive),indent=2)+'\n')
