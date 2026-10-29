#pragma once
#include <unwind.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
namespace NativeEngine {
struct StackCapture { std::uintptr_t *frames; std::size_t size,capacity; };
inline _Unwind_Reason_Code CaptureFrame(_Unwind_Context *context,void *data) {
 auto &capture=*static_cast<StackCapture *>(data);
 if(capture.size==capture.capacity) return _URC_END_OF_STACK;
 auto pc=_Unwind_GetIP(context);
 if(pc) capture.frames[capture.size++]=pc;
 return _URC_NO_REASON;
}
inline std::size_t CaptureStack(std::uintptr_t *frames,std::size_t capacity) {
 StackCapture capture{frames,0,capacity};
 _Unwind_Backtrace(CaptureFrame,&capture);
 return capture.size;
}
}
