#pragma once
#include "reign/vars_prelude.hpp"
#include "varsys.h"
#include "event_slice.hpp"
#include <cmath>
#include <iomanip>
using namespace reign::vars_detail;
using Event = reign::event_detail::Event;
namespace EventSys = reign::event_detail::EventSys;
// Neighboring original headers must share the already staged VarSys class,
// rather than redeclaring the legacy class in the global namespace.
#define __VARSYS_H
template<class T> class Array;
class IFaceVar;
template<class T> inline T Max(T a,T b){return std::max(a,b);}
template<class T> inline void Swap(T &a,T &b){std::swap(a,b);}
template<class T> inline T Clamp(T low,T value,T high){return std::min(std::max(low,value),high);}
#define CDECL
constexpr S32 S32_MIN=std::numeric_limits<S32>::min();
constexpr S32 S32_MAX=std::numeric_limits<S32>::max();
constexpr F32 F32_MIN=-std::numeric_limits<F32>::max();
constexpr F32 F32_MAX=std::numeric_limits<F32>::max();
namespace Utils {
inline S32 FtoL(F32 value){
 if(!std::isfinite(value)||static_cast<double>(value)<S32_MIN||static_cast<double>(value)>S32_MAX)
  throw std::runtime_error("Control color conversion outside signed 32-bit range");
 return static_cast<S32>(value);
}
}
