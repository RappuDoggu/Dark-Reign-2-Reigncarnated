#!/usr/bin/env python3
"""Stage complete original resource-stream translation units, with native ABI.

Reuse reviewed config portability fixes; do not use the menu's replacement
FileSys classes or namespaced runtime slices.
"""
import argparse
import hashlib
import json
from pathlib import Path
from prepare_config_slice import prepare as prepare_config

CONFIG = ['ptree.cpp','ptree.h','fscope.cpp','fscope.h','vnode.cpp','vnode.h',
          'stdparse.cpp','stdparse.h','blockfile.cpp','blockfile.h','tree.h',
          'list.h','strcrc.h','tbuf.cpp','tbuf.h']

def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    upstream, output = upstream.resolve(), output.resolve()
    if output == repo or repo in output.parents or output == upstream or upstream in output.parents:
        raise ValueError('Use external source staging')
    config = Path(str(output) + '-config')
    prepare_config(upstream, config)
    sources = {name:(config/name).read_text() for name in CONFIG}
    pins = json.loads((repo/'cmake/engine-resource-source-hashes.json').read_text())
    for name, digest in pins.items():
        raw = (upstream/'system'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('Unreviewed engine resource source: ' + name)
        sources[name] = raw.decode().replace('\r\n','\n')
    sources['packdef.h'] = sources['packdef.h'].replace(
        '#define PACK_ID       (*((U32*)"NORK"))', '#define PACK_ID       0x4b524f4eu')
    sources['str.cpp'] = sources['str.cpp'].replace(
        'vsprintf(str.ref->data, format, args)', 'vsnprintf(str.ref->data, str.ref->size, format, args)')
    # A portable path can contain either separator (including absolute POSIX paths).
    sources['filesys.cpp'] = sources['filesys.cpp'].replace(
        "if (Utils::Strchr(name, '\\\\'))", "if (Utils::Strchr(name, '\\\\') || Utils::Strchr(name, '/'))")
    sources['filesrcdir.cpp'] = sources['filesrcdir.cpp'].replace('index.DisposeAll();', 'index.UnlinkAll();')
    sources['filesrcpack.cpp'] = sources['filesrcpack.cpp'].replace(
        'memcpy(dst, memoryPtr + readPos, size);', 'memcpy(dst, memoryPtr + readPos, read);')
    # Full original API, but accept both native and legacy separators.
    sources['filesys.cpp'] = sources['filesys.cpp'].replace(
        "if (const char *slash = Utils::Strrchr(path, '\\\\'))",
        "const char *slash = Utils::Strrchr(path, '\\\\');\n"
        "    const char *forward = Utils::Strrchr(path, '/');\n"
        "    if (forward && (!slash || forward > slash)) slash = forward;\n"
        "    if (slash)")
    # FastFind is deleted through its base pointer by original sound records.
    sources['filesys.h'] = sources['filesys.h'].replace('    virtual DataFile* Open() = 0;', '    virtual DataFile* Open() = 0;\n    virtual ~FastFind() = default;')
    output.mkdir(parents=True, exist_ok=True)
    for name, text in sources.items():
        dest = output/name
        if not dest.exists() or dest.read_text() != text:
            dest.write_text(text)

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream',type=Path); parser.add_argument('output',type=Path)
    args=parser.parse_args(); prepare(args.upstream,args.output)
