#pragma once
#include "runcodes.h"
class FScope;
namespace Main {
  typedef void (ScopeHandlerType)(FScope *);
  extern RunCodes runCodes;
  void ScopeHandler(FScope *);
  Bool Exec(const char *, ScopeHandlerType *, Bool);
}
