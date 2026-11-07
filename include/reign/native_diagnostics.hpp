#pragma once
#include <array>
#include <string>
#include <sstream>
#include <cstdint>
#ifdef __ANDROID__
#include <android/log.h>
#endif
namespace reign {
// Render-thread ownership. Bounded journal; do not log every rejected frame.
struct NativeDiagnostics {
 struct Site {std::string text;std::uint64_t count=0;};
 std::array<Site,64> sites;unsigned size=0;std::uint64_t overflow=0;std::string lastDropped;
 void record(std::string text){
  if(text.size()>512)text.resize(512);
  for(unsigned i=0;i<size;++i)if(sites[i].text==text){++sites[i].count;return;}
  if(size==sites.size()){++overflow;lastDropped=std::move(text);return;}
#ifdef __ANDROID__
  __android_log_print(ANDROID_LOG_WARN,"ReignNative","%s",text.c_str());
#endif
  sites[size++]={std::move(text),1};
 }
 std::string report()const{std::ostringstream out;out<<"Native diagnostic sites (cumulative):\n";for(unsigned i=0;i<size;++i)out<<sites[i].count<<" x "<<sites[i].text<<'\n';if(overflow)out<<"overflow "<<overflow<<" last "<<lastDropped<<'\n';return out.str();}
};
inline thread_local NativeDiagnostics nativeDiagnostics;
}
