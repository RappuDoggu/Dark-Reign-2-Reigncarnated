#!/usr/bin/env python3
"""Stage original Babel substitution algorithm with checked native boundaries."""
import argparse, hashlib, json
from pathlib import Path

def prepare(upstream, output):
 repo=Path(__file__).resolve().parents[1];output=output.resolve()
 if output==repo or repo in output.parents:raise ValueError('External staging required')
 raw=(upstream/'util/babel.cpp').read_bytes()
 if hashlib.sha256(raw).hexdigest()!=json.loads((repo/'cmake/babel-hashes.json').read_text())['util/babel.cpp']:raise ValueError('Unreviewed Babel source')
 s=raw.decode().replace('\r\n','\n')
 def patch(a,b,n=1):
  nonlocal s
  if s.count(a)!=n:raise ValueError('Babel anchor changed: '+a)
  s=s.replace(a,b)
 patch('#include "babel.h"\n#include "multilanguage.h"','#include "babel_slice.hpp"\n#include "reign/babel_support.hpp"\n#include "localization_slice.hpp"\n#define MULTILANGUAGE_MAXDATA 16384')
 patch('  LOGDEFLOCAL("Babel")','')
 patch('static CH buffers','static thread_local CH buffers')
 patch('static int bufferIndex','static thread_local int bufferIndex')
 patch('static const CH *error = (const CH*)L"Error";',"static const CH error[] = {'E','r','r','o','r',0};")
 patch('    CH buffer[MULTILANGUAGE_MAXDATA];','    if (!key || std::strlen(key)>=MULTILANGUAGE_MAXDATA) throw std::invalid_argument("Invalid Babel key length");\n    CH buffer[MULTILANGUAGE_MAXDATA];')
 start=s.index('        static CH unknown[128];');end=s.index('        //LOG_ERR',start)
 s=s[:start]+'''        thread_local CH unknown[128];
        reign::babel_detail::unknown(unknown, 128, key);

'''+s[end:]
 patch('      Utils::Memcpy(buffer, ptr, size);','      if ((size & 1u) || size >= sizeof(buffer)) throw std::length_error("Babel lookup too long");\n      Utils::Memcpy(buffer, ptr, size);')
 patch('    size; // Yeah, we\'ll do something with this someday','    reign::babel_detail::validate(format, params);\n    if (!dst || !size) throw std::invalid_argument("Invalid Babel destination");')
 patch('    BinTree<Sub> subs;','    BinTree<Sub> subs;\n    struct Cleanup {BinTree<Sub> &tree; ~Cleanup(){tree.DisposeAll();}} cleanup{subs};')
 patch("      ptr = Utils::Strchr(ptr, CH('{'));","      if (*ptr == CH('{') && *(ptr+1) == CH('{')) ptr += 2;\n      ptr = reign::babel_detail::find(ptr, CH('{'));",2)
 s=s.replace('Utils::Strchr(', 'reign::babel_detail::find(').replace('Utils::AtoI(', 'reign::babel_detail::index(')
 # Error logging is preserved by original engine_log; no Windows-wide formatter.
 s=s.replace('Utils::Unicode2Ansi(format)', '"UTF16 Babel format"')
 s=s.replace('Utils::Sprintf(temp, BABEL_MAXFORMAT,', 'reign::babel_detail::format(temp, BABEL_MAXFORMAT,')
 patch('          Utils::Strncpy(dst, src, len);','          reign::babel_detail::copy(dst, size, src, len);\n          size -= len;')
 patch('        Utils::Strcpy(dst, sub->format);','        reign::babel_detail::copy(dst, size, sub->format);\n        size -= Utils::Strlen(sub->format);')
 s=s.replace('Utils::Strcpy(dst, error);','reign::babel_detail::copy(dst, size, error);')
 patch('      Utils::Strcpy((*i)->format, temp);','      reign::babel_detail::copy((*i)->format, BABEL_MAXFORMAT, temp);')
 patch('    Utils::Strcpy(dst, src);','    reign::babel_detail::copy(dst, size, src);')
 # va_end must run on native boundary exceptions as well as successful calls.
 patch('    const CH *ptr = Translate(args, params, key);\n    va_end(args);\n    return (ptr);','    try { const CH *ptr = Translate(args, params, key); va_end(args); return ptr; }\n    catch (...) {va_end(args); throw;}')
 patch('    Sprintf(buffer, size, args, params, format);\n    va_end(args);','    try {Sprintf(buffer, size, args, params, format); va_end(args);}\n    catch (...) {va_end(args); throw;}')
 output.mkdir(parents=True,exist_ok=True)
 (output/'babel_slice.cpp').write_text(s)
 (output/'babel_slice.hpp').write_text('''#pragma once
#include "control_types.hpp"
#include "reign/frontend_support.hpp"
#include <cstdarg>
namespace Babel {
const CH *Translate(const char *,U32,...);
const CH *Translate(va_list,U32,const char *);
void Sprintf(CH *,U32,const CH *,U32,...);
void Sprintf(CH *,U32,va_list,U32,const CH *);
}
''')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)
