#pragma once
#include <chrono>
#include <map>
#include <string>
#include <sstream>
#ifdef __ANDROID__
#include <android/trace.h>
#endif
namespace reign {
struct FrameCost {double milliseconds=0;unsigned calls=0;};
inline thread_local std::map<std::string,FrameCost> frameCosts;
class FrameSpan {
 const char *name;std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
public:
 explicit FrameSpan(const char *n):name(n){
#ifdef __ANDROID__
 ATrace_beginSection(name);
#endif
 }
 ~FrameSpan(){auto &cost=frameCosts[name];cost.milliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++cost.calls;
#ifdef __ANDROID__
 ATrace_endSection();
#endif
 }
};
inline std::string frame_profile(){std::ostringstream out;out.precision(3);for(const auto &v:frameCosts)out<<v.first<<' '<<v.second.milliseconds/(v.second.calls?v.second.calls:1)<<" ms | ";frameCosts.clear();return out.str();}
}
