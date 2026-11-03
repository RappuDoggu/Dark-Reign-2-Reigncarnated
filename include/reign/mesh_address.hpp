#pragma once
#include "reign/legacy_gles.hpp"
#include <stdexcept>
namespace reign {
// Pinned graphics/Vertex.h RS_ADD_SHIFT=14: DX7 WRAP=1, MIRROR=2, CLAMP=3.
inline TextureAddress mesh_address(std::uint32_t flags){switch((flags>>14)&3){case 0:return TextureAddress::wrap;case 1:return TextureAddress::mirror;case 2:return TextureAddress::clamp;default:throw std::runtime_error("Unsupported GOD texture address mode");}}
}
