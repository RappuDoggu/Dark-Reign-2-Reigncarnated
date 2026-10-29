#!/usr/bin/env python3
"""Port original calendar/statistics code, replacing OS counters and x86 timers."""
import argparse,hashlib,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('upstream',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
repo=Path(__file__).resolve().parents[1];root=a.upstream.resolve();out=a.output.resolve()
if out==root or root in out.parents or out in root.parents or out==repo or repo in out.parents or out in repo.parents:raise ValueError('External staging required')
expected=json.loads((repo/'cmake/engine-clock-source-hashes.json').read_text())
for name,digest in expected.items():
 if hashlib.sha256((root/'system'/name).read_bytes()).hexdigest()!=digest:raise ValueError('Unreviewed clock source: '+name)
text=(root/'system/clock.cpp').read_text().replace('#pragma comment (lib, "winmm")', '// Native backend has no WinMM autolink dependency.')
def body(signature,replacement):
 global text
 start=text.index(signature);begin=text.index('{',start);depth=1;end=begin+1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 text=text[:begin+1]+'\n'+replacement+'\n'+text[end-1:]
text=text.replace('#include <mmsystem.h>', '#include "clock.h"\n#include <chrono>\n#include <ctime>\n#include <limits>\nnamespace {\nU64 NativeNanoseconds() { return U64(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }\nstd::tm NativeLocalTime() { std::time_t value=std::time(nullptr); std::tm result{}; if(!localtime_r(&value,&result)) throw std::runtime_error("Local time conversion failed"); return result; }\n}')
body('const char * GetDateAndTime()', '''    static thread_local char result[256];
    const auto local=NativeLocalTime();
    if(!std::strftime(result,sizeof(result),"%A, %B %d, %Y %H:%M:%S",&local)) throw std::runtime_error("Date formatting failed");
    return result;''')
body('void Init()', '      ASSERT(!initialized);\n      initialized=TRUE;')
body('U32 Ms()', '      return U32(NativeNanoseconds()/1000000);')
body('U64 Us()', '      ASSERT(initialized);\n      return NativeNanoseconds()/1000;')
body('U32 UsLwr()', '      return U32(Us());')
text=text.replace('_strtime(str);','const auto local=NativeLocalTime();\n      std::strftime(str,9,"%H:%M:%S",&local);')
text=text.replace('_strdate(str);','const auto local=NativeLocalTime();\n      std::strftime(str,9,"%m/%d/%y",&local);')
body('char *GetVerbose()', '''      static thread_local char result[32];
      const std::time_t now=std::time(nullptr);
      if(!ctime_r(&now,result)) throw std::runtime_error("Date conversion failed");
      return result;''')
text=text.replace('(month >= 2)', '(month > 2)')
body('    void Start()', '''      const U64 start=NativeNanoseconds();
      oh=0;lo=U32(start);hi=U32(start>>32);''')
body('    void Stop()', '''      const U64 elapsed=NativeNanoseconds()-((U64(hi)<<32)|lo);
      lo=U32(elapsed);hi=U32(elapsed>>32);''')
body('  void CycleWatch::Start()', '''    if(!on) {
      const U64 start=NativeNanoseconds();
      tscLo=U32(start);tscHi=U32(start>>32);on=TRUE;
    }''')
body('  void CycleWatch::Stop()', '''    if(on) {
      const U64 elapsed=NativeNanoseconds()-((U64(tscHi)<<32)|tscLo);
      Sample(elapsed>U32_MAX?U32_MAX:U32(elapsed));on=FALSE;
    }''')
text=text.replace('smooth * 240 + sample * 16','U64(smooth) * 240 + U64(sample) * 16')
text=text.replace('n = sprintf\n    (\n      reportBuf,','n = snprintf\n    (\n      reportBuf, sizeof(reportBuf),')
out.mkdir(parents=True,exist_ok=True)
for name,data in [('clock.cpp',text),('clock.h',(root/'system/clock.h').read_text())]:
 dest=out/name
 if not dest.exists() or dest.read_text()!=data:dest.write_text(data)
