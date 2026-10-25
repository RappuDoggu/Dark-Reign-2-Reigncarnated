#include "tracksys.h"
#include <iostream>
#include <memory>
#include <vector>
using namespace reign::tracking_detail;
static_assert(sizeof(DTrack::Info)==16&&offsetof(DTrack::Info,trackPos)==8);
struct Object {DTrack::Info dTrack;int value=42;};
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Tracking check "+std::to_string(checks));};
 TrackSys::Init();Object first;TrackSys::RegisterConstruction(first.dTrack);Reaper<Object> old(&first);auto id=old.Id();check(old.Alive()&&old->value==42&&id==1);
 TrackSys::RegisterDestruction(first.dTrack);check(old.Dead());TrackSys::RegisterConstruction(first.dTrack);Reaper<Object> fresh(&first);check(fresh.Alive()&&fresh.Id()!=id&&!old.Alive());TrackSys::RegisterDestruction(first.dTrack);check(!fresh.Alive());
 std::vector<std::unique_ptr<Object>> objects;std::vector<Reaper<Object>> observers;
 for(unsigned i=0;i<5000;++i){objects.push_back(std::make_unique<Object>());TrackSys::RegisterConstruction(objects.back()->dTrack);observers.emplace_back(objects.back().get());}
 for(unsigned i=0;i<5000;i+=2)TrackSys::RegisterDestruction(objects[i]->dTrack);
 for(unsigned i=0;i<5000;++i)check(observers[i].Alive()==bool(i%2));
 for(unsigned i=0;i<5000;i+=2){TrackSys::RegisterConstruction(objects[i]->dTrack);check(!observers[i].Alive());TrackSys::RegisterDestruction(objects[i]->dTrack);}
 for(unsigned i=1;i<5000;i+=2)TrackSys::RegisterDestruction(objects[i]->dTrack);
 for(auto &r:observers)check(!r.Alive());TrackSys::Done();
 bool caught=false;try{TrackSys::RegisterDestruction(first.dTrack);}catch(const std::exception &){caught=true;}check(caught);
 DTrack local("claim",64);DTrack::Info max,next;local.RegisterConstruction(max,U32_MAX);local.RegisterConstruction(next);check(max.id==U32_MAX&&next.id==1);local.RegisterDestruction(max);local.RegisterDestruction(next);
 std::cout<<"Original DTrack/Reaper/TrackSys identity/reuse/5000-object lifetime: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
