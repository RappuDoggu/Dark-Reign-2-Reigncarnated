#!/usr/bin/env python3
"""Read-only ZWP v1 metadata inspector. Never extracts or redistributes assets."""
import argparse
import json
import mmap
import struct

class PackError(ValueError):
    pass

def inspect(data):
    size = len(data)
    if size < 56:
        raise PackError('Truncated PACK header')
    magic, version, dirs, doff, files, foff = struct.unpack_from('<4s5I', data)
    if magic != b'NORK' or version != 1:
        raise PackError('Unsupported PACK signature/version')
    if not (56 <= foff <= doff <= size):
        raise PackError('Invalid table offsets')
    if files > (doff - foff) // 14 or dirs > (size - doff) // 2:
        raise PackError('Impossible entry counts')
    def take(pos, count, end):
        if count < 0 or pos < 0 or pos + count > end:
            raise PackError('Truncated PACK record')
        return data[pos:pos+count], pos+count
    def string(pos, end):
        raw, pos = take(pos, 1, end)
        raw, pos = take(pos, raw[0], end)
        if not raw or b'\0' in raw:
            raise PackError('Empty or NUL-containing name')
        return raw.decode('latin1'), pos
    entries, seen = [], set()
    pos = foff
    for _ in range(files):
        raw, pos = take(pos, 4, doff)
        index, = struct.unpack('<I', raw)
        name, pos = string(pos, doff)
        raw, pos = take(pos, 8, doff)
        offset, length = struct.unpack('<II', raw)
        if index > dirs or offset < 56 or offset + length > foff:
            raise PackError('Invalid directory index or asset range')
        if '/' in name or '\\' in name:
            raise PackError('Filename must not include a directory')
        # Engine folding is ASCII-only, not Unicode casefold.
        folded = name.translate(str.maketrans('ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz'))
        if folded in seen:
            raise PackError('Duplicate engine filename')
        seen.add(folded)
        entries.append(dict(name=name, directory_index=index, offset=offset, size=length))
    directories, pos = [], doff
    for _ in range(dirs):
        name, pos = string(pos, size)
        directories.append(name)
    return dict(format='NORK/PACK v1', bytes=size, directories=directories, files=entries)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('pack')
    args = p.parse_args()
    try:
        with open(args.pack, 'rb') as stream:
            if stream.seek(0, 2) < 56:
                raise PackError('Truncated PACK header')
            with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
                print(json.dumps(inspect(data), indent=2))
    except (OSError, PackError) as exc:
        p.exit(1, f'{exc}\n')
