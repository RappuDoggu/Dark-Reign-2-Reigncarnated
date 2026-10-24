#!/usr/bin/env python3
"""Stage reviewed original config sources outside this repository; apply explicit portability edits."""
import argparse
import hashlib
import json
from pathlib import Path

NAMES = ['ptree.cpp','ptree.h','fscope.cpp','fscope.h','vnode.cpp','vnode.h','stdparse.cpp','stdparse.h',
         'blockfile.cpp','blockfile.h','nlist.h','tree.h','list.h','strcrc.h','tbuf.cpp','tbuf.h','strbuf.h']

def prepare(upstream, output):
    repo = Path(__file__).resolve().parents[1]
    output = output.resolve()
    if output == repo or repo in output.parents:
        raise ValueError('Upstream overlays must stay outside the project repository')
    pins = json.loads((repo/'cmake/config-source-hashes.json').read_text())
    output.mkdir(parents=True,exist_ok=True)
    changes=[]
    def replace(text,old,new,label):
        if text.count(old)!=1:
            raise ValueError('Unreviewed source shape for '+label)
        changes.append(label)
        return text.replace(old,new)
    for name in NAMES:
        raw=(upstream/'system'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=pins[name]:
            raise ValueError('Unreviewed source hash: '+name)
        text=raw.decode().replace('\r\n','\n')
        if name=='tree.h':
            text=replace(text,'#include "debug_memory.h"','// Uses standard allocation in this isolated configuration slice.','tree debug allocator header boundary')
        if name=='ptree.h':
            text=replace(text,'#pragma pack(push, 1)','// PTree is a live C++ object, not a disk record.','natural PTree object alignment')
            text=replace(text,'#pragma pack(pop)','','remove paired PTree object pack pragma')
        if name in ['tbuf.cpp','fscope.cpp']:
            text=replace(text,'vsprintf(fmtBuf, fmt, args);','vsnprintf(fmtBuf, sizeof(fmtBuf), fmt, args);',name+' bounded diagnostics')
        if name=='vnode.h':
            text=replace(text,'static char* aTypeStrings[AT_MAX];','static const char* aTypeStrings[AT_MAX];','const diagnostic string declaration')
        if name=='vnode.cpp':
            text=replace(text,'char* VNode::aTypeStrings[AT_MAX]','const char* VNode::aTypeStrings[AT_MAX]','const diagnostic string definition')
        if name=='blockfile.cpp':
            text=replace(text,'ERR_FATAL((lastError));','ERR_FATAL(("%s", lastError));','safe preformatted block error')
            text=replace(text,'BlockFile::~BlockFile()\n{\n  Close();\n}',
                'BlockFile::~BlockFile()\n{\n'
                '  if (std::uncaught_exceptions()) {\n'
                '    blockMode = BM_CLOSED;\n'
                '    try { Close(); } catch (...) {}\n'
                '  } else { Close(); }\n}', 'exception-safe block cleanup at native error boundary')
        if name=='blockfile.h':
            text=replace(text,'#define BLOCK_ID       (*((U32*)"DOCB"))',
                '#define BLOCK_ID       0x42434F44u','alignment-safe little-endian DOCB identifier')
        if name=='stdparse.cpp':
            boundary=text.index('VNode* StdParse::ParseStringVNode')
            numeric=text[:boundary]
            numeric=replace(numeric,'VNode *newNode = new VNode;',
                'std::unique_ptr<VNode> owned(new VNode);\n  VNode *newNode = owned.get();','numeric parse exception ownership')
            numeric=numeric.replace('return (newNode);','return owned.release();')
            text=numeric+text[boundary:]
            text=replace(text,'    int val = 0;','    U32 val = 0;','defined 32-bit integer accumulation')
            text=replace(text,'val += 1<<digit;','val += U32(1)<<digit;','unsigned binary shift')
            text=replace(text,'      // Binary integer','      if (len > 33) { tBuf->TokenError("Binary integer exceeds 32 bits"); }\n      // Binary integer','bounded binary shifts')
            text=replace(text,'      for (int digit = 0; digit < len-2; digit++)',
                '      if (len > 10) { tBuf->TokenError("Hex integer exceeds 32 bits"); }\n      for (int digit = 0; digit < len-2; digit++)','bounded hexadecimal shifts')
            text=replace(text,'newNode->SetupInteger(val * (negate ? -1:1));',
                'U32 bits = negate ? U32(0) - val : val;\n'
                '    S32 signedValue; memcpy(&signedValue, &bits, sizeof(signedValue));\n'
                '    newNode->SetupInteger(signedValue);','defined two-complement signed integer result')
        if name=='ptree.cpp':
            text=replace(text,'ERR_FATAL(("Failed reading %d bytes from binary data"));','ERR_FATAL(("Failed reading %u bytes from binary data", size));','binary read diagnostic argument')
            text=replace(text,'    // Add this string and key\n    symbols.Add(crc, (const char *)binaryPos);',
                '    if (!len || len > binaryRemaining || binaryPos[len - 1] != 0)\n'
                '    { ERR_FATAL(("Invalid binary symbol length")); }\n'
                '    // Add this string and key\n    symbols.Add(crc, (const char *)binaryPos);','bounded binary symbol names')
        (output/name).write_text(text)
    for name in ['runcodes.cpp','runcodes.h','maincmd.cpp']:
        raw=(upstream/'main'/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=pins['main/'+name]:
            raise ValueError('Unreviewed main source hash: '+name)
        text=raw.decode().replace('\r\n','\n')
        if name=='maincmd.cpp':
            marker='  void ProcessCmdScope(FScope *fScope, ScopeHandlerType *func)'
            if text.count(marker)!=1: raise ValueError('Unreviewed Main::Exec source shape')
            text='#include "ptree.h"\n#include "main.h"\nnamespace Main {\n'+text[text.index(marker):]
            (output/'main_exec.cpp').write_text(text)
        else: (output/name).write_text(text)
    (output/'main.h').write_text('#include "reign/runcode_host.hpp"\n')
    (output/'utiltypes.h').write_text('using GameIdent = StrCrc<64>;\n')
    (output/'file.h').write_text('#include "reign/engine_file.hpp"\n')
    (output/'filesys.h').write_text('#include "reign/engine_file.hpp"\n')
    (output/'portability-manifest.json').write_text(json.dumps(dict(source_revision='9ce9da5f21604310a997f0c41e9cd383f5e292c3',edits=changes),indent=2)+'\n')
    return changes

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('upstream',type=Path);p.add_argument('output',type=Path)
    a=p.parse_args();print(json.dumps(prepare(a.upstream.resolve(),a.output)))
