#pragma once
#include "native_stack.h"
#include <dlfcn.h>
#include <cstring>
#include <sstream>
#include <string>
namespace NativeEngine {
// Fault-only evidence, captured before stack unwinding. No VarPtr layout change.
inline std::string &LastVariableFailure(){static thread_local std::string value;return value;}
inline void RecordVariableFailure(const char *operation)noexcept{
 try{
  std::uintptr_t frames[16];auto count=CaptureStack(frames,16);std::ostringstream out;out<<operation<<'\n';
  for(std::size_t i=0;i<count;++i){
   Dl_info module{};const auto pc=frames[i]?frames[i]-1:0;
   if(dladdr(reinterpret_cast<void *>(pc),&module)&&module.dli_fbase){
    const char *name=module.dli_fname?module.dli_fname:"unknown";if(const char *slash=std::strrchr(name,'/'))name=slash+1;
    out<<'#'<<i<<' '<<name<<"+0x"<<std::hex<<(pc-reinterpret_cast<std::uintptr_t>(module.dli_fbase))<<std::dec<<'\n';
   }
  }
  LastVariableFailure()=out.str().substr(0,4096);
 }catch(...){} // Preserve the original exception even if diagnostic allocation fails.
}
}
