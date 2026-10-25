#!/usr/bin/env python3
"""Privately compare real 24-bit BMP pixels; no images are written or uploaded."""
import argparse,io,mmap,subprocess
from pathlib import Path
from inspect_pack import inspect
from PIL import Image,__version__
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('probe',type=Path);p.add_argument('archive',type=Path,help='one archive or an installation directory');args=p.parse_args()
count=pixels=skipped=0
archives=[args.archive] if args.archive.is_file() else sorted(args.archive.rglob('*.zwp'))
for path in archives:
 with path.open('rb') as stream, mmap.mmap(stream.fileno(),0,access=mmap.ACCESS_READ) as pack:
    for entry in inspect(pack)['files']:
        if not entry['name'].lower().endswith('.bmp'):continue
        label=path.name+'/'+entry['name']
        data=pack[entry['offset']:entry['offset']+entry['size']]
        if len(data)<54:raise ValueError('Truncated real BMP: '+label)
        if int.from_bytes(data[28:30],'little')!=24:
            skipped+=1;print('Non-24-bit BMP excluded from standard decoder oracle:',label);continue
        expected=Image.open(io.BytesIO(data)).convert('RGBA')
        result=subprocess.run([str(args.probe.resolve())],input=data,capture_output=True,check=True)
        if result.stdout!=expected.tobytes():raise ValueError('RGBA mismatch: '+label)
        count+=1;pixels+=expected.width*expected.height
        print(f"{label}: RGBA matches Pillow {__version__}, {expected.width}x{expected.height}")
if not count:raise ValueError('No supported BMP images exercised')
print(f'Independent real BMP oracle: archives={len(archives)} images={count} pixels={pixels} mismatches=0 other_depths_skipped={skipped}')
