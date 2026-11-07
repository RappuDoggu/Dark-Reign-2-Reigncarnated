#!/usr/bin/env python3
"""Build a private runtime ZIP from owned installation ZIPs, first input wins.

No assets are downloaded. Every collision, omission and exclusion is recorded.
Output must stay outside this source checkout. Uses the repository pack inspector.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import tempfile
import zipfile

from inspect_pack import inspect

TREES = {'packs', 'missions', 'worlds', 'sides', 'mods', 'movies', 'data', 'library', 'music'}
EXTENSIONS = {'.zwp', '.cfg', '.bik', '.ogg', '.mp3', '.wav', '.tga', '.bmp', '.pic', '.god', '.blk', '.fff', '.xsi', '.txt'}
REQUIRED = {
    'packs/base.zwp': {'runcode_main_login.cfg', 'english.cfg'},
    'missions/system/shell.zwp': {'interface_shell.cfg'},
    'worlds/sprawl.zwp': {'types_world.cfg'},
    'sides/client/jda.zwp': {'interface_game_side.cfg'},
    'sides/client/sprawler.zwp': {'interface_game_side.cfg'},
    'library/engine/startup.cfg': set(),
    'missions/multiplayer/group.cfg': set(),
}

def sha(data):
    return hashlib.sha256(data).hexdigest()

def canonical(name):
    path = name.replace('\\', '/')
    parts = path.split('/')
    if path.startswith('/') or ':' in path or '\x00' in path or '..' in parts:
        raise ValueError('Unsafe input ZIP path: ' + repr(name))
    parts = [part.lower() for part in parts if part]
    for index, part in enumerate(parts[:-1]):
        if part in TREES:
            target = '/'.join(parts[index:])
            return target if PurePosixPath(target).suffix in EXTENSIONS else None
    return None

def build(inputs, output, exclusions):
    repo = Path(__file__).resolve().parents[1]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('Keep owned runtime ZIPs outside the source checkout')
    if output in [path.resolve() for path in inputs]:
        raise ValueError('Output must not replace an input')
    output.parent.mkdir(parents=True, exist_ok=True)
    report = {'schema': 1, 'policy': 'first input wins; differing duplicates recorded',
              'inputs': [], 'files': [], 'conflicts': [], 'identical_duplicates': [],
              'excluded': [], 'ignored': [], 'campaign_references': []}
    selected = {}
    opened = []
    temporary = None
    try:
        for ordinal, path in enumerate(inputs):
            with path.open('rb') as stream:
                digest = hashlib.file_digest(stream, 'sha256').hexdigest()
            report['inputs'].append({'index': ordinal, 'name': path.name, 'sha256': digest, 'bytes': path.stat().st_size})
            archive = zipfile.ZipFile(path)
            opened.append(archive)
            local = set()
            for entry in archive.infolist():
                if entry.is_dir():
                    continue
                target = canonical(entry.filename)
                if target is None:
                    report['ignored'].append({'input': ordinal, 'path': entry.filename})
                    continue
                if target in local:
                    raise ValueError('Duplicate normalized path within input: ' + target)
                local.add(target)
                if target in exclusions:
                    report['excluded'].append({'input': ordinal, 'path': target, 'reason': exclusions[target]})
                    continue
                if not 0 < entry.file_size <= 512 * 1024 * 1024:
                    raise ValueError('Empty or oversized runtime file: ' + target)
                data = archive.read(entry)  # zipfile verifies the entry CRC.
                item = {'path': target, 'bytes': len(data), 'sha256': sha(data), 'input': ordinal}
                if target.endswith('.zwp'):
                    metadata = inspect(data)
                    item['pack_files'] = len(metadata['files'])
                    names = {f['name'].lower() for f in metadata['files']}
                    if not REQUIRED.get(target, set()) <= names:
                        raise ValueError('Required internal assets missing: ' + target)
                if target in selected:
                    previous = selected[target][2]
                    same = previous['sha256'] == item['sha256']
                    report['identical_duplicates' if same else 'conflicts'].append({
                        'path': target, 'selected': previous, 'other': item})
                    continue
                selected[target] = (archive, entry, item)
        missing = sorted(set(REQUIRED) - set(selected))
        tracks = [n for n in selected if n.startswith('music/') and PurePosixPath(n).suffix in {'.ogg', '.mp3', '.wav'}]
        if missing or not 1 <= len(tracks) <= 128:
            raise ValueError('Incomplete runtime: missing=' + repr(missing) + ', music tracks=' + str(len(tracks)))
        if not any(n.startswith('mods/') for n in selected) or not any(n.startswith('missions/multiplayer/') and n.endswith('.zwp') for n in selected):
            raise ValueError('Missing mods or multiplayer maps')
        total = sum(item[2]['bytes'] for item in selected.values())
        if total > 2 * 1024**3:
            raise ValueError('Runtime exceeds importer 2 GiB limit')
        for name, (archive, entry, _) in selected.items():
            if name.endswith('/group.cfg'):
                content = archive.read(entry).decode('latin1')
                for mission in re.findall(r'\bAdd\s*\(\s*"([^"]+)"', content):
                    target = name.rsplit('/', 1)[0] + '/' + mission.lower() + '.zwp'
                    report['campaign_references'].append({'group': name, 'mission': mission, 'present': target in selected})
                    if target not in selected:
                        raise ValueError('Missing campaign mission: ' + target)
        report.update({'runtime_files': len(selected), 'runtime_bytes': total, 'music_tracks': len(tracks)})
        report['files'] = [selected[n][2] for n in sorted(selected)]
        descriptor, temporary = tempfile.mkstemp(prefix='owned-runtime-', suffix='.part', dir=output.parent)
        os.close(descriptor)
        with zipfile.ZipFile(temporary, 'w', zipfile.ZIP_DEFLATED, compresslevel=1) as result:
            for name in sorted(selected):
                archive, entry, _ = selected[name]
                result.writestr(name, archive.read(entry))
            result.writestr('packaging-manifest.json', json.dumps(report, indent=2) + '\n')
        with zipfile.ZipFile(temporary) as result:
            bad = result.testzip()
            if bad:
                raise ValueError('Output CRC failure: ' + bad)
        os.replace(temporary, output)
        temporary = None
        with output.open('rb') as stream:
            report['output'] = {'name': output.name, 'bytes': output.stat().st_size,
                                'sha256': hashlib.file_digest(stream, 'sha256').hexdigest()}
        return report
    finally:
        for archive in opened:
            archive.close()
        if temporary:
            os.unlink(temporary)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('inputs', nargs='+', type=Path, help='Owned ZIPs, preferred complete installation first')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--exclude', action='append', default=[], metavar='PATH=REASON')
    args = parser.parse_args()
    exclusions = {}
    for value in args.exclude:
        path, separator, reason = value.rpartition('=')
        if not separator or not path or not reason:
            parser.error('--exclude requires PATH=REASON')
        exclusions[path.lower()] = reason
    if args.report.resolve() == args.output.resolve() or args.report.resolve() in [p.resolve() for p in args.inputs]:
        parser.error('Report must not replace an input or output ZIP')
    try:
        report = build(args.inputs, args.output, exclusions)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({key: report[key] for key in ['runtime_files', 'runtime_bytes', 'music_tracks', 'output']}, indent=2))
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        parser.exit(1, str(error) + '\n')

if __name__ == '__main__':
    main()
