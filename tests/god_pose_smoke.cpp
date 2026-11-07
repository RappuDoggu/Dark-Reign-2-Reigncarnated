#include "reign/god_pose.hpp"
#include <cmath>
#include <iostream>
#include <limits>
namespace {
reign::GodAnimKey key(float frame,float x){reign::GodAnimKey k;k.frame=frame;k.type=1;k.quaternion={1,0,0,0};k.position={x,0,0};return k;}
reign::GodMesh mesh(){reign::GodMesh m;auto &g=m.geometry;g.states={key(0,0),key(0,0)};g.stateMatrices={reign::identity_pose_matrix,reign::identity_pose_matrix};g.vertices={{1,0,0}};g.vertexWeights={{{1,1}}};reign::GodNode root;root.state=0;root.parent=-1;root.objectMatrix=reign::identity_pose_matrix;auto child=root;child.state=1;child.parent=0;child.name="bone";m.nodes={root,child};reign::GodCycle c;c.name="move";c.maxFrame=10;c.endFrame=9;c.states=g.states;reign::GodAnimation a;a.state=1;a.keys={key(0,0),key(10,10)};c.animations={a};m.cycles={c};return m;}
}
int main(){try{unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Pose check "+std::to_string(checks));};auto near=[&](float a,float b){check(std::abs(a-b)<.0001f);};auto reject=[&](auto fn){bool failed=false;try{fn();}catch(const std::exception&){failed=true;}check(failed);};
 auto m=mesh();reign::GodPoseEvaluator p(m);near(p.sample("").vertices[0][0],1);near(p.sample("MOVE",5).vertices[0][0],6);near(p.sample("move",10).vertices[0][0],11);near(p.sample("move",0).vertices[0][0],1);
 auto world=reign::identity_pose_matrix;world[12]=20;near(reign::compose_pose_matrices(world,world)[12],40);near(p.sample("move",5,world).vertices[0][0],26);
 // Skinning: one weight ignores its scalar, multiple weights use each scalar.
 auto weighted=m;weighted.geometry.vertexWeights[0]={{{.25f,0},{.75f,1}}};reign::GodPoseEvaluator wp(weighted);near(wp.sample("move",5).vertices[0][0],4.75f);near(wp.sample("move",5,reign::identity_pose_matrix,false).vertices[0][0],1);
 weighted.geometry.vertexWeights[0]={{{.25f,1}}};reign::GodPoseEvaluator sp(weighted);near(sp.sample("move",5).vertices[0][0],6);
 auto rotated=m;auto &keys=rotated.cycles[0].animations[0].keys;keys[0].type=keys[1].type=2;keys[1].position={0,0,0};keys[1].quaternion={0,0,0,1};reign::GodPoseEvaluator rp(rotated);auto v=rp.sample("move",5).vertices[0];near(v[0],0);near(v[1],-1);near(v[2],0);
 auto scaled=m;for(auto &k:scaled.cycles[0].animations[0].keys)k.type=5;scaled.cycles[0].animations[0].keys[1].scale={3,1,1};reign::GodPoseEvaluator sc(scaled);near(sc.sample("move",5).vertices[0][0],7);
 // Parent chain, bind inverse and external placement are independent transforms.
 auto chain=m;chain.geometry.states.push_back(key(0,0));chain.geometry.stateMatrices.push_back(reign::identity_pose_matrix);chain.geometry.stateMatrices[2][12]=-3;chain.geometry.stateMatrices[2][13]=-4;chain.nodes[1].objectMatrix[12]=3;auto n=chain.nodes[1];n.state=2;n.parent=1;n.objectMatrix=reign::identity_pose_matrix;n.objectMatrix[13]=4;chain.nodes.push_back(n);chain.geometry.vertexWeights[0][0].state=2;chain.cycles.clear();reign::GodPoseEvaluator ch(chain);v=ch.sample("",0,world).vertices[0];near(v[0],21);near(v[1],0);near(ch.sample("").worldMatrices[2][13],4);
 // Control nodes retain rest state when the cycle has no track for that node.
 auto control=m;control.nodes[1].name="CP-gun";control.cycles[0].states[1].position[0]=90;control.cycles[0].animations.clear();reign::GodPoseEvaluator cp(control);near(cp.sample("move",5).vertices[0][0],1);
 auto sign=m;for(auto &k:sign.cycles[0].animations[0].keys)k.type=2;sign.cycles[0].animations[0].keys[1].quaternion={-1,0,0,0};reign::GodPoseEvaluator sg(sign);near(sg.sample("move",5).vertices[0][0],1);
 auto redundant=m;for(auto &k:redundant.cycles[0].animations[0].keys)k.type=2;reign::GodPoseEvaluator rd(redundant);near(rd.sample("move",5).vertices[0][0],1);
 reject([&]{p.sample("missing",0);});reject([&]{p.sample("move",11);});reject([&]{p.sample("move",-1);});reject([&]{p.sample("move",std::numeric_limits<float>::quiet_NaN());});world[3]=1;reject([&]{p.sample("",0,world);});
 for(unsigned kind=0;kind<7;++kind){auto bad=m;if(kind==0)bad.nodes[1].parent=1;if(kind==1)bad.nodes[1].state=0;if(kind==2)bad.geometry.vertexWeights[0][0].state=3;if(kind==3)bad.cycles[0].animations[0].keys[1].frame=0;if(kind==4)bad.cycles[0].states.clear();if(kind==5)bad.cycles[0].animations[0].keys.resize(1);if(kind==6)bad.geometry.vertexWeights[0].clear();reject([&]{reign::GodPoseEvaluator fail(bad);});}
 std::cout<<"Original pose interpolation, transforms, control initialization, skinning and validation: PASS ("<<checks<<")\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
