#include "reign/world_projection.hpp"
#include "world_math_slice.hpp"
#include <stdexcept>
namespace reign {
struct WorldProjection::Impl{world_detail::Matrix view,projection;};
WorldProjection::WorldProjection(std::array<float,3> position,std::array<float,3> direction,float fov,float aspect,float nearPlane,float farPlane):impl_(std::make_unique<Impl>()){
 for(float v:position)if(!std::isfinite(v))throw std::invalid_argument("Invalid camera position");
 for(float v:direction)if(!std::isfinite(v))throw std::invalid_argument("Invalid camera direction");
 if(!std::isfinite(fov)||fov<=0||fov>=3.14159265f||!std::isfinite(aspect)||aspect<=0||!std::isfinite(nearPlane)||nearPlane<=0||!std::isfinite(farPlane)||farPlane<=nearPlane)throw std::invalid_argument("Invalid camera frustum");
 const double length=std::sqrt(double(direction[0])*direction[0]+double(direction[1])*direction[1]+double(direction[2])*direction[2]);
 if(length<1e-8)throw std::invalid_argument("Zero camera direction");
 world_detail::Vector front(float(direction[0]/length),float(direction[1]/length),float(direction[2]/length));
 if(double(front.x)*front.x+double(front.z)*front.z<1e-12)throw std::invalid_argument("Original camera basis is singular at vertical direction");
 world_detail::Matrix camera;camera.SetFromFront(front);camera.posit={position[0],position[1],position[2]};
 impl_->view.SetInverse(camera);impl_->projection.SetProjection(nearPlane,farPlane,fov,aspect);
 // Original Vid::SetProjTransform flips screen y before the logical-target flip.
 impl_->projection.up.y*=-1;
}
WorldProjection::~WorldProjection()=default;
std::array<float,4> WorldProjection::transform(std::array<float,3> point)const{
 for(float v:point)if(!std::isfinite(v))throw std::invalid_argument("Invalid world position");
 std::array<float,3> camera{},clip{};
 impl_->view.Transform_Vectors(camera.data(),point.data(),12,1);
 impl_->projection.Transform_Vectors(clip.data(),camera.data(),12,1);
 std::array<float,4> result{clip[0],clip[1],clip[2],camera[2]};
 for(float v:result)if(!std::isfinite(v))throw std::runtime_error("Nonfinite projected position");return result;
}
}
