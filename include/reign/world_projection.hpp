#pragma once
#include <array>
#include <memory>
namespace reign {
// Original scalar Matrix orientation/inverse/projection. The returned coordinates
// precede perspective division and retain points behind/at the camera for clipping.
class WorldProjection {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 WorldProjection(std::array<float,3> position,std::array<float,3> direction,
                 float verticalFovRadians,float aspectHeightOverWidth,float nearPlane,float farPlane);
 ~WorldProjection();
 WorldProjection(const WorldProjection&)=delete;
 WorldProjection &operator=(const WorldProjection&)=delete;
 // x/y follow Vid's screen-down homogeneous convention; z uses Direct3D [0,w].
 std::array<float,4> transform(std::array<float,3> point)const;
};
}
