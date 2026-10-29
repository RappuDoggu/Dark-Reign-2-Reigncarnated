#!/usr/bin/env python3
"""Adapt pinned DX7 declarations without defining _WIN32 on native compilers."""
import argparse,hashlib,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];out=a.output.resolve();source=a.upstream.resolve()
if out==repo or repo in out.parents or out in repo.parents or out==source or source in out.parents or out in source.parents: raise ValueError('External staging required')
out.mkdir(parents=True,exist_ok=True)
for name,digest in json.loads((repo/'cmake/engine-dx7-source-hashes.json').read_text()).items():
 raw=(source/'3rdparty/dx7'/name).read_bytes()
 if hashlib.sha256(raw).hexdigest()!=digest: raise ValueError('Unreviewed SDK header '+name)
 text=raw.decode().replace('\r\n','\n')
 text=re.sub(r'\b_WIN32\b|\bWIN32\b','REIGN_DX7_DECLARATIONS',text)
 # SDK uses pack(4)/pack() rather than balanced push/pop. Isolate its ABI packing.
 text='#pragma pack(push)\n'+text+'\n#pragma pack(pop)\n'
 dest=out/name
 if not dest.exists() or dest.read_text()!=text: dest.write_text(text)

# Default implementations reject unsupported interfaces explicitly. Supported
# memory-surface operations override these in surface_native.cpp.
text=(out/'ddraw.h').read_text()
block=text.split('DECLARE_INTERFACE_( IDirectDrawSurface7, IUnknown )',1)[1].split('};',1)[0]
methods=[]
for line in block.splitlines():
 match=re.search(r'STDMETHOD\((\w+)\)\s*\((.*?)\)\s*PURE;',line)
 if match:
  name,args=match.groups();args=args.replace('THIS_','').replace('THIS','').replace('FAR','').strip()
  methods.append(f' HRESULT {name}({args}) override {{ return E_NOTIMPL; }}')
# AddRef/Release are kept abstract so a default object can never leak silently.
header='#pragma once\n#include "ddraw.h"\nnamespace NativeEngine {\nstruct UnsupportedSurface : IDirectDrawSurface7 {\n'+'\n'.join(methods)+'\n};\n}\n'
dest=out/'surface_unsupported.h'
if not dest.exists() or dest.read_text()!=header:dest.write_text(header)

# Test/device adapters share the exact original virtual interface. Unsupported
# operations fail explicitly; ownership methods remain abstract.
text=(out/'d3d.h').read_text()
block=text.split('DECLARE_INTERFACE_(IDirect3DDevice7, IUnknown)',1)[1].split('};',1)[0]
methods=[]
for line in block.splitlines():
 match=re.search(r'STDMETHOD\((\w+)\)\s*\((.*?)\)\s*PURE;',line)
 if match:
  name,args=match.groups();args=args.replace('THIS_','').replace('THIS','').replace('FAR','').strip()
  methods.append(f' HRESULT {name}({args}) override {{ return E_NOTIMPL; }}')
header='#pragma once\n#include "d3d.h"\nnamespace NativeEngine {\nstruct UnsupportedDevice : IDirect3DDevice7 {\n'+'\n'.join(methods)+'\n};\n}\n'
dest=out/'device_unsupported.h'
if not dest.exists() or dest.read_text()!=header:dest.write_text(header)
