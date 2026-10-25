#!/usr/bin/env python3
"""Stage complete original control/root/button/window translation units externally.
Produces compilation targets, not a linkable or runnable frontend. Movie handling,
font/interface services and several utility implementations remain link requirements.
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
        raise ValueError('Original source must be staged outside the repository')
    texts = {}
    hashes = json.loads((repo/'cmake/frontend-source-hashes.json').read_text())
    for name, digest in hashes.items():
        raw = (upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('Unreviewed frontend source: '+name)
        texts[name] = raw.decode().replace('\r\n', '\n')
    output.mkdir(parents=True, exist_ok=True)
    def replace_once(s, old, new):
        if s.count(old) != 1:
            raise ValueError('Expected one occurrence: '+old)
        return s.replace(old, new)
    for name in ['icontrol', 'icroot', 'icbutton', 'icwindow', 'ifvar', 'icsystembutton']:
        s = texts['interface/'+name+'.cpp']
        s = s.replace('#include "input.h"', '#include "control_input.hpp"')
        s = s.replace('#include "stdload.h"', '#include "control_stdload.hpp"')
        # Explicit dependencies in frontend_prelude replace legacy precompiled headers.
        for header in ['vid.h', 'debug_memory.h', 'perfstats.h']:
            s = s.replace('#include "'+header+'"', '')
        if name == 'ifvar':
            macro = re.search(r'^#define MULTILANGUAGE_MAXDATA .*$', texts['util/multilanguage.h'], re.M).group(0)
            s = replace_once(s, '#include "multilanguage.h"', macro)
        if name == 'icsystembutton':
            s = 'class ICListBox; using ICListBoxPtr=Reaper<ICListBox>;\n#include "icslider.h"\n'+s
        if name == 'icontrol':
            if s.count('Color(0L, 0L, 0L, 128L)') != 2:
                raise ValueError('Unexpected color construction sites')
            s = s.replace('Color(0L, 0L, 0L, 128L)', 'Color(S32(0), S32(0), S32(0), S32(128))')
            s = replace_once(s, 'alignTo->parent ? alignTo->parent : alignTo;',
                             'alignTo->parent ? alignTo->parent : alignTo.GetPointer();')
            s = replace_once(s, "char *dot = Utils::Strrchr(stripped, '.');",
                             "const char *dot = Utils::Strrchr(stripped, '.');")
            start = s.index('  if (texture && texture->texture && texture->texture->GetBink()')
            end = s.index('\n  // Notify control that is was deactivated', start)
            # This adapter is intentionally UNDEFINED, never a successful no-op.
            s = s[:start]+'  if (texture && texture->texture) reign::deactivate_control_movie(texture->texture);\n'+s[end:]
        (output/(name+'.cpp')).write_text(s)
    s = replace_once(texts['interface/input.h'], 'void Init(HINSTANCE inst, HWND window);',
                     '// Windows startup declaration excluded; native input lifecycle remains separate.')
    (output/'control_input.hpp').write_text(s)
    s = texts['system/array.h']
    s = s[s.index('template <class DATA'):s.index('// pads array data')]
    s = s[:s.rfind('//----------------------------------------------------------------------------')]
    s = s.replace('U32 ALIGNMENT = 0', 'U32 ALIGNMENT')
    (output/'control_array.hpp').write_text(s)
    s = texts['util/utiltypes.h']
    r = s[s.index('template <class TYPE> class Range'):s.index('//\n// NameSpace Ranges')]
    r = r[:r.rfind('///////////////////////////////////////////////////////////////////////////////')]
    s = texts['util/stdload.h'];s = s[s.index('namespace StdLoad'):]
    decl = '\n'.join(x for x in s.splitlines() if re.match(
        r'  (U32 TypeU32|F32 TypeF32|const char\* TypeString|U32 TypeStringCrc|template <class TYPE> void TypePoint\(FScope \*fScope)', x))
    (output/'control_stdload.hpp').write_text('#pragma once\n'+r+'\nnamespace StdLoad {\n'+decl+'\n}\n')
    # Preserve original menu scalar/string/coordinate loading, including defaults
    # and inclusive range validation. No rewritten approximation of the parser.
    source = texts['util/stdload.cpp']
    source = source[source.index('namespace StdLoad'):]
    signatures = re.findall(r'^  (?:U32 TypeU32|F32 TypeF32|const char\* TypeString(?:D)?|U32 TypeStringCrc)\([^\n]+\)', source, re.M)
    signatures += ['  template <> void TypePoint(FScope *fScope, Point<'+t+'> &val)' for t in ['F32','U32','S32']]
    if len(signatures) != 19:
        raise ValueError('Unexpected original scalar loader count')
    bodies = []
    for signature in signatures:
        start = source.index(signature+'\n  {')
        end = source.index('{', start)+1
        depth = 1
        while depth:
            depth += (source[end] == '{') - (source[end] == '}')
            end += 1
        bodies.append(source[start:end])
    ranges = re.findall(r'^Range<(?:U32|F32)> const Range<[^\n]+;', texts['util/utiltypes.cpp'], re.M)
    # Modern compilers require explicit specialization for these static members.
    ranges = ['template <> '+r.replace('U32_MIN','0u') for r in ranges]
    (output/'control_stdload.cpp').write_text('#include "control_types.hpp"\n#include "fscope.h"\n#include "control_stdload.hpp"\n'+
        '\n'.join(ranges)+'\nnamespace StdLoad {\n'+'\n\n'.join(bodies)+'\n}\n')
    (output/'manifest.json').write_text(json.dumps({'source_hashes': hashes,
        'translation_units': ['icontrol', 'icroot', 'icbutton', 'icwindow', 'ifvar', 'icsystembutton'],
        'linked': False, 'engine_initialized': False, 'menu_rendered': False}, indent=2)+'\n')
    print('Staged six full original frontend translation units; runtime linkage still required')

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('upstream', type=Path);p.add_argument('output', type=Path)
    a = p.parse_args();prepare(a.upstream, a.output)

