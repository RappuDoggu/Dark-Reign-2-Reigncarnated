#!/usr/bin/env python3
"""Stage original control declarations and coordinate methods outside the repo.

This is a compilation boundary, not a substitute implementation of IControl.
Full control construction, virtual dispatch, drawing and event handling remain
in the original full translation unit and are not implemented by this target.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path

def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('Original source staging must remain outside repository')
    texts = {}
    for name, digest in json.loads((repo/'cmake/control-source-hashes.json').read_text()).items():
        raw = (upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('Unreviewed original control dependency: '+name)
        texts[name] = raw.decode().replace('\r\n', '\n')
    def declaration(text, signature):
        start = text.index(signature)
        at = text.index('{', start)
        depth = 1
        end = at + 1
        while depth:
            depth += (text[end] == '{') - (text[end] == '}')
            end += 1
        return text[start:end]
    float_type = declaration(texts['system/utils.h'], 'struct Float2Int')+';\n'
    magic = re.search(r'^double Float2Int::magic = .*?;', texts['system/utils.cpp'], re.M).group(0)
    area = texts['util/area.h']
    # Remove only Win32 RECT reinterpretation operators. Portable coordinates stay original.
    start = area.index('  // windows conversion')
    end = area.index('\n};', start)
    area = area[:start]+area[end:]
    iftypes = texts['interface/iftypes.h'].replace('sizeof TextureSkin', 'sizeof(TextureSkin)')
    parts = [float_type, area, 'using ClipRect=Area<S32>;\n', texts['util/color.h'], iftypes,
             texts['interface/iface_types.h'], texts['util/promotelink.h'], texts['interface/icontrol.h']]
    parts = [re.sub(r'^#include[^\n]*\n', '', part, flags=re.M) for part in parts]
    notice = '// Original material staged externally: Copyright Pandemic Studios, Dark Reign II.\n'
    output.mkdir(parents=True, exist_ok=True)
    (output/'control_types.hpp').write_text(notice+'#pragma once\n#include "reign/control_prelude.hpp"\n'+ '\n'.join(parts)+'\nnamespace IFace { S32 GetMetric(U32); }\n')
    signatures = ['Point<S32> IControl::ScreenToWindow(const Point<S32> &p) const',
                  'Point<S32> IControl::ScreenToClient(const Point<S32> &p) const',
                  'Point<S32> IControl::ClientToScreen(const Point<S32> &p) const',
                  'Bool IControl::InClientFromWindow(const Point<S32> &p) const',
                  'ClipRect IControl::GetAdjustmentRect()']
    body = '\n\n'.join(declaration(texts['interface/icontrol.cpp'], s) for s in signatures)
    (output/'control_coordinates.cpp').write_text(notice+'#include "control_types.hpp"\n'+body+'\n')
    (output/'control_color_magic.cpp').write_text(notice+'#include "control_types.hpp"\n'+magic+'\n')
    print('Staged original IControl declarations and coordinate/border methods externally:', output)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('upstream', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    prepare(a.upstream, a.output)
