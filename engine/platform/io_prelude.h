#pragma once
#include "defines.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#ifndef ASSERT
#define ASSERT(condition) do { if (!(condition)) throw std::logic_error("Engine assertion: " #condition); } while (0);
#endif
#include "utils.h"
#include "strbuf.h"
#include "crc.h"
