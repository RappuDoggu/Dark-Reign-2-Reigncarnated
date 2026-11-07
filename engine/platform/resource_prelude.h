#pragma once
#include "io_prelude.h"
#include <algorithm>
#include <cctype>
#include <cstdarg>
#include <cstdlib>
#include <exception>
#include <memory>
#include <limits>
namespace NativeEngine {
[[noreturn]] inline void Error(const char *format, ...) {
  char text[2048]; va_list ap; va_start(ap,format);
  std::vsnprintf(text,sizeof text,format,ap); va_end(ap);
  throw std::runtime_error(text);
}
inline void Log(const char *format, ...) {
  va_list ap; va_start(ap,format); std::vfprintf(stderr,format,ap); va_end(ap);
  std::fputc('\n',stderr);
}
}
#define VALIDATE(x) ASSERT(x)
#define ERR_FATAL(args) NativeEngine::Error args;
#define ERR_CONFIG(args) NativeEngine::Error args;
#define LOG_DIAG(args) NativeEngine::Log args;
#define LOG_WARN(args) NativeEngine::Log args;
#define LOG_ERR(args) NativeEngine::Log args;
#define LOGDEC
#define LOGDEF(a,b)
#include "nlist.h"
#include "ntree.h"
#include "tree.h"
#include "list.h"
#include "strcrc.h"
#include "filetypes.h"
