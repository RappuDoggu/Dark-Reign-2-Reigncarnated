#pragma once
#include "reign/config.hpp"
#include <array>
#include <cstddef>
#include <optional>
namespace reign {
struct GodFace {
 std::uint16_t bucket=0;
 std::array<std::uint16_t,3> vertices{},normals{},uvs{};
};
struct GodMaterial {
 std::string name;
 std::array<float,4> diffuse{},specular{},emissive{},ambient{};
 float specularPower=0;
};
struct GodTexture {std::string name;std::uint32_t type=0,mipCount=0;};
struct GodBucket {
 std::uint32_t flags=0,vertexCount=0,indexCount=0,teamColor=0,envMap=0,overlay=0;
 std::optional<GodMaterial> material;
 std::optional<GodTexture> texture0,texture1;
};
struct GodWeight {float weight=0;std::uint16_t state=0;};
struct GodAnimKey {
 float frame=0;std::uint32_t type=0;
 std::array<float,4> quaternion{};
 std::array<float,3> position{},scale{{1,1,1}};
};
// Serialized root prefix, NOT a fully loaded/posed MeshRoot. Animation cycles,
// MRM, node hierarchy and chunks follow this boundary and remain unvalidated.
// Raw flags/materials are retained; runtime material/texture policy is not applied.
struct GodMeshPrefix {
 unsigned version=0;
 std::string name;
 float radius=0,scale=0;
 std::array<float,3> extent{},offset{};
 std::vector<std::array<float,3>> vertices,normals;
 std::vector<std::array<float,2>> uvs;
 std::vector<std::uint32_t> colors;
 std::vector<GodFace> faces;
 std::vector<GodBucket> buckets;
 // Original VertIndex keeps the first two serialized influences without renormalizing.
 std::vector<std::vector<GodWeight>> vertexWeights;
 std::vector<std::array<float,4>> planes;
 std::vector<std::array<float,16>> stateMatrices;
 std::vector<GodAnimKey> states;
 std::size_t prefixBytes=0,blockBytes=0;
};
// Bounded little-endian compatibility reader for MeshRoot::Load through its
// initial state keys. No engine managers, GPU objects or original assets created.
GodMeshPrefix read_god_mesh_prefix(ConfigLoader loader,const std::string &filename);
struct GodAnimation {std::uint32_t state=0;float maxFrame=0;std::vector<GodAnimKey> keys;};
struct GodCycle {
 std::string name;std::uint32_t type=0;
 float maxFrame=0,endFrame=0,speed=0,controlFrame=0,framesPerMeter=0;
 std::vector<float> bounds;
 std::vector<GodAnimKey> states;std::vector<GodAnimation> animations;
};
struct GodLocalGroup {
 std::uint32_t state=0,vertexCount=0,indexCount=0,plane=0;
 std::optional<GodMaterial> material;std::optional<GodTexture> texture;
};
struct GodNode {
 std::string name;std::uint32_t state=0,flags=0;int parent=-1;
 std::array<float,16> objectMatrix{};
 std::vector<std::array<float,3>> vertices,normals;
 std::vector<std::array<float,2>> uvs;
 std::vector<std::uint32_t> colors;
 std::vector<GodLocalGroup> groups;std::vector<std::uint16_t> indices;
};
struct GodMrmUpdate {std::uint32_t face=0,token=0;std::array<std::uint32_t,2> indices{};};
struct GodMrmVertex {std::uint32_t newFaces=0,newNormals=0,newUvs=0;std::vector<GodMrmUpdate> updates;};
struct GodMrm {
 std::uint32_t maxVertices=0,minVertices=0,minMinVertices=0;
 std::vector<GodMrmVertex> vertices;
};
// Complete serialized root tree. Raw keys/matrices are retained: runtime cycle
// normalization, root state reset, pose evaluation and GPU setup are separate.
struct GodMesh {
 GodMeshPrefix geometry;
 std::vector<GodCycle> cycles;std::optional<GodMrm> mrm;float mrmFactor=0;
 std::vector<GodNode> nodes;
 std::uint32_t chunkIndex=0;std::vector<GodMesh> chunks;
 std::size_t serializedBytes=0;
};
GodMesh read_god_mesh(ConfigLoader loader,const std::string &filename);

}
