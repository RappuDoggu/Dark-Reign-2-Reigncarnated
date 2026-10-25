#!/usr/bin/env python3
"""Stage original IFace metrics implementation outside the repository."""
import argparse,hashlib,json
from pathlib import Path

def prepare(upstream,output):
    repo=Path(__file__).resolve().parents[1];output=output.resolve()
    if output==repo or repo in output.parents:raise ValueError('Metric staging must be external')
    texts={}
    for name,digest in json.loads((repo/'cmake/interface-metric-hashes.json').read_text()).items():
        raw=(upstream/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=digest:raise ValueError('Unreviewed IFace metric source: '+name)
        texts[name]=raw.decode().replace('\r\n','\n')
    h=texts['interface/iface_types.h'];a=h.index('  enum MetricType');b=h.index('  // Crc lookup tables',a);enum=h[a:b]
    source=texts['interface/iface.cpp']
    def function(signature):
        a=source.index(signature+'\n  {');start=source.index('{',a);depth=1;b=start+1
        while depth:depth+=(source[b]=='{')-(source[b]=='}');b+=1
        return source[a:b].replace('  static void ','  void ',1)
    body='\n\n'.join(function(s) for s in ['  static void UpdateScreenMetrics()','  static void InitMetrics()','  static void ConfigureMetrics(FScope *fScope)','  S32 GetMetric(U32 type)'])
    body=body.replace('metricTableCrc','state().keys').replace('metricTable','state().values')
    for call,field in [('Width()','width'),('Height()','height'),('Depth()','depth')]:body=body.replace('Vid::backBmp.'+call,'state().'+field)
    notice='// Selected original IFace implementation: Copyright 1997-1999 Pandemic Studios, Dark Reign II.\n'
    output.mkdir(parents=True,exist_ok=True)
    (output/'metrics_slice.hpp').write_text(notice+'#pragma once\n#include "reign/config_prelude.hpp"\n#include "fscope.h"\nnamespace reign::metric_detail {\n'+enum+'\nstruct State {S32 values[MAX_METRICS]{};U32 keys[MAX_CUSTOM_METRICS]{};S32 width=0,height=0,depth=32;};\nState &state();void InitMetrics();void UpdateScreenMetrics();void ConfigureMetrics(FScope *);S32 GetMetric(U32);\n}\n')
    (output/'metrics_slice.cpp').write_text(notice+'#include "metrics_slice.hpp"\nnamespace reign::metric_detail {\n'+body+'\n}\n')
    print('Staged original IFace metric configuration/default/read functions externally:',output)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args();prepare(a.upstream,a.output)
