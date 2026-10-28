#include "reign/god_pose.hpp"
#include "mesh_pose_slice.hpp"
#include <algorithm>
#include <cctype>
#include <limits>
namespace reign {
namespace {
using namespace pose_detail;
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
void finite(float f){require(std::isfinite(f),"Nonfinite pose value");}
Matrix matrix(const PoseMatrix &v){for(float f:v)finite(f);Matrix m;std::memcpy(&m,v.data(),64);return m;}
PoseMatrix packed(const Matrix &m){PoseMatrix v;std::memcpy(v.data(),&m,64);for(float f:v)finite(f);return v;}
Vector vector(const std::array<float,3> &v){for(float f:v)finite(f);return {v[0],v[1],v[2]};}
AnimKey key(const GodAnimKey &src){AnimKey k;k.frame=src.frame;finite(k.frame);k.type=src.type;for(float f:src.quaternion)finite(f);k.quaternion={src.quaternion[0],{src.quaternion[1],src.quaternion[2],src.quaternion[3]}};k.position=vector(src.position);k.scale=vector(src.scale);return k;}
std::string folded(std::string s){for(char &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
Matrix object(const AnimKey &k){require(std::isfinite(k.quaternion.Dot(k.quaternion))&&k.quaternion.Dot(k.quaternion)>0,"Invalid pose quaternion");Matrix m;m.Set(k.quaternion);m.Set(k.position);m.SetScale(k.scale);packed(m);return m;}
}
struct GodPoseEvaluator::Impl {
 struct Track {unsigned state;Animation animation;};
 struct Cycle {std::string name;float maxFrame;std::vector<AnimKey> states;std::vector<Track> tracks;};
 std::vector<Vector> vertices;
 std::vector<VertIndex> weights;
 std::vector<Matrix> bind;
 std::vector<AnimKey> rest;
 std::vector<bool> controls;
 struct Node{unsigned state;int parentState;};std::vector<Node> nodes;
 std::vector<Cycle> cycles;
 explicit Impl(const GodMesh &m){
  auto &g=m.geometry;const auto count=g.states.size();require(count>0&&count<=64&&g.stateMatrices.size()==count,"Pose state/matrix count mismatch");
  require(m.nodes.size()==count,"Pose hierarchy must cover every state");rest.resize(count);controls.resize(count);std::vector<bool> seen(count);
  for(std::size_t i=0;i<m.nodes.size();++i){auto &n=m.nodes[i];require(n.state<count&&!seen[n.state],"Duplicate/out-of-range pose node state");seen[n.state]=true;
   require(n.parent>=-1&&n.parent<int(i),"Invalid pose parent ordering");require((n.parent<0)==(n.state==0),"Pose root must be state zero");
   auto local=matrix(n.objectMatrix);auto &k=rest[n.state];k.quaternion.Set(local);k.position=local.posit;object(k);controls[n.state]=folded(n.name).rfind("cp-",0)==0;
   nodes.push_back({n.state,n.parent<0?-1:int(m.nodes[n.parent].state)});
  }
  rest[0]=AnimKey{};for(auto &b:g.stateMatrices)bind.push_back(matrix(b));
  require(g.vertexWeights.size()==g.vertices.size(),"Pose vertex weight count mismatch");
  for(std::size_t i=0;i<g.vertices.size();++i){vertices.push_back(vector(g.vertices[i]));auto &source=g.vertexWeights[i];require(!source.empty()&&source.size()<=2,"Pose requires one or two weights");VertIndex w;w.count=source.size();for(unsigned j=0;j<w.count;++j){require(source[j].state<count,"Pose bone index out of range");finite(source[j].weight);w.index[j]=source[j].state;w.weight[j]=source[j].weight;}weights.push_back(w);}
  for(auto &c:m.cycles){Cycle cycle;cycle.name=folded(c.name);cycle.maxFrame=c.maxFrame;finite(cycle.maxFrame);require(cycle.maxFrame>=0,"Invalid pose cycle range");require(c.states.size()==count,"Pose cycle state count mismatch");for(auto &s:c.states)cycle.states.push_back(key(s));
   require(std::none_of(cycles.begin(),cycles.end(),[&](auto &other){return other.name==cycle.name;}),"Duplicate pose cycle");
   for(auto &a:c.animations){require(a.state<count,"Pose track state out of range");Track t;t.state=a.state;for(auto &k:a.keys)t.animation.keys.values.push_back(key(k));auto &keys=t.animation.keys.values;
    require(keys.empty()||keys.size()>=2,"Original pose track requires at least two keys");
    for(std::size_t i=1;i<keys.size();++i)require(keys[i-1].frame<keys[i].frame,"Non-increasing pose key frames");
    // Match Mesh::LoadAnimation: a redundant quaternion-only end key is removed.
    // In that branch its subsequent sign adjustment affects the discarded array.
    if(keys.size()>=2){auto &a0=keys[keys.size()-2];auto &a1=keys.back();auto same=a0.quaternion.s==a1.quaternion.s&&a0.quaternion.v.x==a1.quaternion.v.x&&a0.quaternion.v.y==a1.quaternion.v.y&&a0.quaternion.v.z==a1.quaternion.v.z;
     if(a0.type==2&&a1.type==2&&same)keys.pop_back();
     else if(a0.quaternion.Dot(a1.quaternion)<0)a1.quaternion=a1.quaternion*-1.f;
    }
    t.animation.keys.count=keys.size();cycle.tracks.push_back(std::move(t));
   }cycles.push_back(std::move(cycle));
  }
 }
};
PoseMatrix compose_pose_matrices(const PoseMatrix &local,const PoseMatrix &parent){return packed(matrix(local)*matrix(parent));}
GodPoseEvaluator::GodPoseEvaluator(const GodMesh &m):impl_(std::make_unique<Impl>(m)){}
GodPoseEvaluator::~GodPoseEvaluator()=default;
GodPoseEvaluator::GodPoseEvaluator(GodPoseEvaluator&&)noexcept=default;
GodPoseEvaluator &GodPoseEvaluator::operator=(GodPoseEvaluator&&)noexcept=default;
GodPoseFrame GodPoseEvaluator::sample(const std::string &name,float frame,const PoseMatrix &world,bool multiWeight)const{
 require(bool(impl_),"Moved-from pose evaluator");finite(frame);require(frame>=0,"Negative pose frame");auto &p=*impl_;auto states=p.rest;
 if(!name.empty()){auto found=std::find_if(p.cycles.begin(),p.cycles.end(),[&](auto &c){return c.name==folded(name);});require(found!=p.cycles.end(),"Unknown pose cycle");require(frame<=found->maxFrame,"Pose frame beyond cycle range");
  for(std::size_t i=1;i<states.size();++i)if(!p.controls[i]){states[i].quaternion=found->states[i].quaternion;states[i].position=found->states[i].position;states[i].scale=found->states[i].scale;}
  for(auto &t:found->tracks)t.animation.SetFrame(frame,states[t.state]);
 }
 auto root=matrix(world);require(world[3]==0&&world[7]==0&&world[11]==0&&world[15]==1,"Pose world matrix must be affine");
 std::vector<Matrix> worlds(states.size()),skin(states.size());GodPoseFrame result;result.worldMatrices.resize(states.size());
 for(auto &n:p.nodes){worlds[n.state]=n.parentState<0?root:object(states[n.state])*worlds[n.parentState];result.worldMatrices[n.state]=packed(worlds[n.state]);skin[n.state]=p.bind[n.state]*worlds[n.state];packed(skin[n.state]);}
 MeshRoot original;result.vertices.reserve(p.vertices.size());for(std::size_t i=0;i<p.vertices.size();++i){Vector v;original.SetVert(v,p.vertices[i],skin.data(),p.weights[i],multiWeight);finite(v.x);finite(v.y);finite(v.z);result.vertices.push_back({v.x,v.y,v.z});}return result;
}
}
