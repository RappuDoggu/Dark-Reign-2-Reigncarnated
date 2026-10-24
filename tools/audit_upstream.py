#!/usr/bin/env python3
"""Emit metadata only; never copy upstream code into the destination repository."""
import argparse
import collections
import hashlib
import json
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

def audit(root):
    paths = subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z']).decode().split('\0')
    paths = [p for p in paths if p]
    ns = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
    projects = []
    for name in paths:
        if not name.endswith('.vcxproj'):
            continue
        p = root / name
        tree = ET.parse(p)
        refs = [x.attrib['Include'].replace('\\', '/') for x in tree.findall('.//m:ProjectReference', ns)]
        projects.append({'path': name,
            'toolsets': sorted({x.text for x in tree.findall('.//m:PlatformToolset', ns)}),
            'source_count': len(tree.findall('.//m:ClCompile[@Include]', ns)),
            'references': refs,
            'missing_references': [r for r in refs if not (p.parent / r).exists()],
            'link_dependencies': sorted({x.text for x in tree.findall('.//m:AdditionalDependencies', ns) if x.text})})
    patterns = {
        'assembly_or_intrinsics': r'\b(__asm|_asm|_rotr|_rotl|__declspec|_controlfp)\b',
        'windows_services': r'\b(CreateFile|CreateFileMapping|CreateThread|CreateEvent|WaitForMultipleObjects|CRITICAL_SECTION|PeekMessage|QueryPerformanceCounter)\b',
        'directx': r'\b(DirectDrawCreateEx|DirectInput8Create|DrawIndexedPrimitive|IDirect3DDevice7|IDirectDraw7|DirectSoundCreate)\b',
        'miles_bink': r'\b(AIL_\w+|Bink\w+)\b',
        'sockets': r'\b(WSA\w+|closesocket|SOCKET_ERROR)\b',
    }
    findings = {k: [] for k in patterns}
    for name in paths:
        if not name.endswith(('.cpp', '.h', '.c', '.cxx')) or name.startswith(('3rdparty/', 'won/')):
            continue
        for number, line in enumerate((root / name).read_text(errors='replace').splitlines(), 1):
            for kind, regex in patterns.items():
                symbols = sorted(set(re.findall(regex, line)))
                if symbols:
                    findings[kind].append({'path': name, 'line': number, 'symbols': symbols})
    return {'commit': subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD']).decode().strip(),
        'tracked_files': len(paths), 'top_level_files': dict(sorted(collections.Counter(p.split('/')[0] for p in paths).items())),
        'projects': projects, 'findings': findings,
        'primitive_sha256': {name: hashlib.sha256((root / 'system' / name).read_bytes()).hexdigest()
            for name in ('crc.cpp', 'crc.h', 'random.cpp', 'random.h', 'packdef.h')},
        'method': 'Lexical candidate inventory; includes comments/inactive code. Not a complete call graph or license grant.'}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('checkout', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.write_text(json.dumps(audit(args.checkout.resolve()), indent=2) + '\n')
