#pragma once
#include "reign/spline_types.hpp"
#include <cstdint>
#include <stdexcept>
// These types stay inside the isolated original camera implementation.
namespace reign::camera_detail {
using U32=std::uint32_t;
using Bool=std::int32_t;
constexpr Bool TRUE=1,FALSE=0;
struct Matrix {Vector posit,front;};
}
#ifndef ASSERT
#define ASSERT(x) if(!(x)) throw std::runtime_error("Original bookmark assertion: " #x);
#endif
#ifndef ERR_FATAL
#define ERR_FATAL(args) throw std::runtime_error args;
#endif
