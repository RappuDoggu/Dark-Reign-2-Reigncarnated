#include "movetable.h"
#include <iostream>
#include <string>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Movement check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception &){caught=true;}check(caught);};
 for(int cycle=0;cycle<2;++cycle){
 MoveTable::Init();struct Close{~Close(){MoveTable::Done();}} close;
 auto ground=MoveTable::SurfaceIndex("ground"),wheels=MoveTable::TractionIndex("wheels");
 check(ground==0&&wheels==0);check(MoveTable::SurfaceIndex("ground")==ground);
 auto initial=MoveTable::GetBalance(ground,wheels);check(initial.speed==0.05f&&initial.health==0&&initial.slope==0);
 FScope root(nullptr,"CreateTractionType");root.AddArgString("wheels");
 root.AddFunction("DefaultSlope")->AddArgInteger(45);
 auto *over=root.AddFunction("OverSurface");over->AddArgString("ground");over->AddArgFPoint(0.75f);over->AddArgFPoint(-0.25f);
 root.InitIterators();MoveTable::ProcessCreateTractionType(&root);
 auto changed=MoveTable::GetBalance(ground,wheels);check(changed.speed==0.75f&&changed.health==-0.25f&&changed.slope==45);
 auto water=MoveTable::SurfaceIndex("water"),tracks=MoveTable::TractionIndex("tracks");
 check(MoveTable::GetBalance(water,wheels).speed==0.05f);check(MoveTable::GetBalance(ground,tracks).slope==0);
 check(MoveTable::FindSurfaceInfo(ground)->ident.crc==Crc::CalcStr("ground"));
 FScope bad(nullptr,"CreateTractionType");bad.AddArgString("wheels");auto *b=bad.AddFunction("OverSurface");b->AddArgString("ground");b->AddArgFPoint(0.01f);bad.InitIterators();rejects([&]{MoveTable::ProcessCreateTractionType(&bad);});
 check(MoveTable::GetBalance(ground,wheels).speed==0.75f);
 rejects([&]{MoveTable::GetBalance(63,63);});
 for(unsigned i=2;i<64;++i){check(MoveTable::SurfaceIndex(("surface"+std::to_string(i)).c_str())==i);check(MoveTable::TractionIndex(("traction"+std::to_string(i)).c_str())==i);}
 rejects([&]{MoveTable::SurfaceIndex("overflow");});rejects([&]{MoveTable::TractionIndex("overflow");});
 check(MoveTable::GetTable().KeyCountX()==64&&MoveTable::GetTable().KeyCountY()==64);
 }
 std::cout<<"Original movement defaults, traction config, bounds, capacity and restart: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
