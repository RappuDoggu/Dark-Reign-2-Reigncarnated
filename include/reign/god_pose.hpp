#pragma once
#include "reign/god_mesh.hpp"
#include <memory>
namespace reign {
using PoseMatrix=std::array<float,16>;
inline constexpr PoseMatrix identity_pose_matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
// Original scalar row-vector matrix multiplication (local then parent).
PoseMatrix compose_pose_matrices(const PoseMatrix &local,const PoseMatrix &parent);
struct GodPoseFrame {
 std::vector<std::array<float,3>> vertices;
 std::vector<PoseMatrix> worldMatrices;
};
// Owns validated animation/geometry data. Samples a cycle from its initial state
// at an explicit frame; does not implement MeshEnt's clock, transitions or overlays.
class GodPoseEvaluator {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit GodPoseEvaluator(const GodMesh &mesh);
 ~GodPoseEvaluator();
 GodPoseEvaluator(GodPoseEvaluator&&)noexcept;
 GodPoseEvaluator &operator=(GodPoseEvaluator&&)noexcept;
 // Empty cycle means rest pose. Matrix is original row-vector convention.
 GodPoseFrame sample(const std::string &cycle,float frame=0,const PoseMatrix &world=identity_pose_matrix,bool multiWeight=true)const;
};
}
