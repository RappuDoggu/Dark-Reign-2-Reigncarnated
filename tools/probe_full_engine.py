#!/usr/bin/env python3
"""Inventory the original app project and compile representative complete TUs.

Does not use extracted-function overlays, invent Windows stubs, or claim a link.
The manifest retains the project dependency closure for a future full build.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

NS = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
SAMPLES = {
    'coregame': 'game.cpp', 'coregame_objects': 'unitobj.cpp',
    'coregame_tasks': 'tasks_unitbuild.cpp',
    'coregame_orders': 'orders_game_build.cpp',
    'coregame_interface': 'client_events.cpp', 'graphics': 'vidrend.cpp',
    'game': 'gameruncodes.cpp', 'multiplayer': 'multiplayer.cpp',
}

def inventory(root):
    projects = {}
    def visit(path):
        path = path.resolve()
        name = path.relative_to(root).as_posix()
        if name in projects:
            return
        tree = ET.parse(path)
        refs = [(path.parent / e.attrib['Include'].replace('\\', '/')).resolve()
                for e in tree.findall('.//m:ProjectReference', NS)]
        sources = [(path.parent / e.attrib['Include'].replace('\\', '/')).resolve()
                   for e in tree.findall('.//m:ClCompile[@Include]', NS)]
        groups = tree.findall('.//m:ItemDefinitionGroup', NS)
        group = next(g for g in groups if 'Release|Win32' in g.attrib.get('Condition', ''))
        def values(tag):
            value = group.find('m:ClCompile/m:' + tag, NS)
            return [s for s in (value.text or '').split(';') if s and not s.startswith('%(')] if value is not None else []
        projects[name] = {
            'sources': [p.relative_to(root).as_posix() for p in sources],
            'references': [p.relative_to(root).as_posix() for p in refs],
            'include_directories': [(path.parent / s.replace('\\', '/')).resolve().relative_to(root).as_posix()
                                    for s in values('AdditionalIncludeDirectories')],
            'forced_includes': values('ForcedIncludeFiles'),
            'missing_sources': [p.relative_to(root).as_posix() for p in sources if not p.is_file()],
        }
        for ref in refs:
            visit(ref)
    visit(root / 'appdr2/appdr2.vcxproj')
    return projects

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream', type=Path)
    parser.add_argument('--compiler', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    root = args.upstream.resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    projects = inventory(root)
    results = []
    for project, info in projects.items():
        sample = SAMPLES.get(Path(project).stem)
        if not sample:
            continue
        source = next(s for s in info['sources'] if Path(s).name == sample)
        cmd = [str(args.compiler.resolve()), '-std=c++17', '-DDEVELOPMENT=1',
               '-fsyntax-only', '-ferror-limit=6']
        for inc in info['include_directories']:
            cmd += ['-I', str(root / inc)]
        for inc in info['forced_includes']:
            cmd += ['-include', inc]
        cmd.append(str(root / source))
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        diagnostic = (result.stdout + result.stderr).replace(str(root), '<upstream>')
        log = source.replace('/', '__') + '.log'
        (args.output / log).write_text(diagnostic)
        errors = [line for line in diagnostic.splitlines() if 'error:' in line]
        results.append({'source': source, 'sha256': hashlib.sha256((root/source).read_bytes()).hexdigest(),
                        'exit_code': result.returncode, 'first_error': errors[0] if errors else None,
                        'diagnostics': log})
    report = {'method': 'Original Release project closure; representative whole-TU Android syntax probes. No platform shims. Not a complete compiler-error inventory.',
              'projects': projects, 'source_count': sum(len(p['sources']) for p in projects.values()),
              'probes': results, 'linked': False, 'mission_booted': False}
    (args.output/'full-engine.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({'projects': len(projects), 'sources': report['source_count'],
                      'probes': len(results), 'passed': sum(r['exit_code']==0 for r in results),
                      'first_errors': dict(Counter(r['first_error'] for r in results)),
                      'linked': False, 'mission_booted': False}, indent=2))
    return 1 if any(r['exit_code'] for r in results) else 0

if __name__ == '__main__':
    raise SystemExit(main())
