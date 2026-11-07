#!/usr/bin/env python3
"""Stage the original engine definitions for native whole-file integration.

Keep original global names and complete headers; unlike isolated slice preludes,
this header can be consumed by original engine translation units consistently.
This is not a Win32 implementation and does not bypass missing OS/graphics APIs.
"""
import argparse
import hashlib
import json
from pathlib import Path

def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    upstream, output = upstream.resolve(), output.resolve()
    if output == repo or repo in output.parents or output == upstream or upstream in output.parents:
        raise ValueError('Use an external staging directory, outside both source trees')
    pins = json.loads((repo/'cmake/engine-abi-source-hashes.json').read_text())
    sources = {}
    for name, digest in pins.items():
        raw = (upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest() != digest:
            raise ValueError('Unreviewed engine ABI source: ' + name)
        sources[name] = raw.decode().replace('\r\n', '\n')
    definitions = sources['system/defines.h']
    for old, new in [('typedef unsigned long         U32;', 'typedef std::uint32_t         U32;'),
                     ('typedef signed long           S32;', 'typedef std::int32_t          S32;')]:
        if definitions.count(old) != 1:
            raise ValueError('Unexpected definition: ' + old)
        definitions = definitions.replace(old, new)
    start = definitions.index('#ifdef _WIN32', definitions.index('typedef std::int32_t'))
    end = definitions.index('typedef unsigned int', start)
    definitions = definitions[:start] + 'typedef std::uint64_t U64;\ntypedef std::int64_t S64;\n\n' + definitions[end:]
    start = definitions.index('#define STDCALL')
    end = definitions.index('//\n// Type Definitions', start)
    original = definitions[start:end]
    definitions = definitions[:start] + '#if defined(_WIN32)\n' + original + '''#else
#define STDCALL
#define FASTCALL
#define CDECL
#define NAKED __attribute__((naked))
#define THREAD thread_local
#define NORETURN __attribute__((noreturn))
#define DLL_EXPORT __attribute__((visibility("default")))
#define DLL_IMPORT
#endif

''' + definitions[end:]
    definitions = '#pragma once\n#include <cstdint>\n' + definitions
    definitions += '''
static_assert(sizeof(U32)==4 && sizeof(S32)==4 && sizeof(U64)==8 && sizeof(S64)==8);
#if !defined(_WIN32)
inline U32 _rotr(U32 value, int count) noexcept {
  const unsigned shift = static_cast<unsigned>(count) & 31u;
  return shift ? (value >> shift) | (value << (32u - shift)) : value;
}
#endif
'''
    sources['system/defines.h'] = definitions
    # Keep the original public File/Dir API. Only replace OS-owned state and
    # Windows constants; original callers remain unmodified.
    file_header = sources['system/file.h']
    file_header = file_header.replace('#include <io.h>', '#include "file_platform.h"')
    first = file_header.index('  // Handle for current file')
    last = file_header.index('public:', first)
    file_header = file_header[:first] + '  NativeFileState *native;\n\n' + file_header[last:]
    for old, new in [('FILE_BEGIN', '0'), ('FILE_CURRENT', '1'), ('FILE_END', '2'),
                     ('int handle;', 'NativeFindState *handle = nullptr;'),
                     ('_finddata_t finddata;', 'NativeFindData finddata;')]:
        if file_header.count(old) != 1:
            raise ValueError('Unexpected File API boundary: ' + old)
        file_header = file_header.replace(old, new)
    # Owning descriptors and directory iterators must never be shallow-copied.
    file_header = file_header.replace('  File();',
        '  File();\n  File(const File &) = delete;\n  File &operator=(const File &) = delete;')
    file_header = file_header.replace('    friend class Dir;',
        '    Find() = default;\n    ~Find();\n    Find(const Find &) = delete;\n'
        '    Find &operator=(const Find &) = delete;\n    friend class Dir;')
    sources['system/file.h'] = file_header
    # Original signed char indexing reads before the CRC table for bytes >=128.
    sources['system/crc.cpp'] = sources['system/crc.cpp'].replace(
        'toLower[*p]', 'toLower[static_cast<U8>(*p)]')
    output.mkdir(parents=True, exist_ok=True)
    for name, text in sources.items():
        dest = output/Path(name).name
        if not dest.exists() or dest.read_text() != text:
            dest.write_text(text)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('upstream', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    prepare(args.upstream, args.output)
