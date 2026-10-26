#!/usr/bin/env python3
"""Stage original language lookup externally; platform font setup is excluded."""
import argparse, hashlib, json
from pathlib import Path

def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('External staging required')
    raw = (upstream / 'util/multilanguage.cpp').read_bytes()
    expected = json.loads((repo / 'cmake/localization-hashes.json').read_text())
    if hashlib.sha256(raw).hexdigest() != expected['util/multilanguage.cpp']:
        raise ValueError('Unreviewed localization source')
    source = raw.decode().replace('\r\n', '\n')
    def extract(signature):
        a = source.index(signature); b = source.index('{', a) + 1; depth = 1
        while depth:
            depth += (source[b] == '{') - (source[b] == '}'); b += 1
        return source[a:b]
    def replace(text, before, after):
        if text.count(before) != 1:
            raise ValueError('Localization patch anchor changed: ' + before)
        return text.replace(before, after)
    path = extract('  const char * MakePath(const char *path)')
    path = replace(path, '    ASSERT(initialized)\n    ASSERT(currentLanguage)', '''    auto &context = reign::localization_detail::state();
    if (!path) throw std::invalid_argument("Null language path");
    if (std::strlen(path) + context.language.size() + 15 >= VARSYS_MAXVARPATH)
      throw std::length_error("Language path too long");''')
    path = replace(path, 'static char buffer[400];', 'char *buffer = context.path;')
    path = replace(path, 'rootScope', '"multilanguage"')
    path = replace(path, 'currentLanguage->name.str', 'context.language.c_str()')
    item = extract('  Item * GetItem(const char *path, U32 &size)')
    item = replace(item, '    ASSERT(initialized)', '    (void)reign::localization_detail::state();')
    item = replace(item, '      size = item->BinarySize();', '''      if (item->type != VarSys::VI_BINARY)
        throw std::runtime_error("Language value is not binary");
      size = item->BinarySize();
      if (size < 4 || (size & 1u) || size > 4 + 2 * 16383)
        throw std::runtime_error("Invalid language record size");
      const auto *bytes = item->Binary();
      bool high = false;
      for (U32 at = 4; at < size; at += 2) {
        U16 ch = U16(bytes[at]) | (U16(bytes[at+1]) << 8);
        if (high) { if (ch < 0xdc00 || ch > 0xdfff) throw std::runtime_error("Invalid UTF16 pair"); high = false; }
        else if (ch >= 0xd800 && ch <= 0xdbff) high = true;
        else if (ch >= 0xdc00 && ch <= 0xdfff) throw std::runtime_error("Unexpected UTF16 low surrogate");
      }
      if (high) throw std::runtime_error("Incomplete UTF16 pair");''')
    lookups = []
    for signature in ['  void Lookup(const char *path, const CH *&data, U32 &size, U32 &version)',
                      '  void Lookup(const char *path, const CH *&data, U32 &size)']:
        lookup = extract(signature)
        lookup = replace(lookup, '    ASSERT(initialized);', '''    (void)reign::localization_detail::state();
    if (!path) throw std::invalid_argument("Null language key");''')
        lookups.append(lookup)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'localization_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include <string>
#include <cstring>
namespace reign::localization_detail {
struct State {std::string language; char path[VARSYS_MAXVARPATH]{};};
State &state();
}
namespace MultiLanguage {
void Lookup(const char *, const CH *&, U32 &, U32 &);
void Lookup(const char *, const CH *&, U32 &);
}
''')
    (output / 'localization_slice.cpp').write_text('#include "localization_slice.hpp"\nnamespace MultiLanguage {\n' +
        extract('  struct Item') + ';\n' + path + '\n' + item + '\n' + '\n'.join(lookups) + '\n}\n')
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('upstream', type=Path); parser.add_argument('output', type=Path)
    args = parser.parse_args(); prepare(args.upstream, args.output)
