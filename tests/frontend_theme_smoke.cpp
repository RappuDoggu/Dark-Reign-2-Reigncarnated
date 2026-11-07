#include "reign/frontend_theme.hpp"
#include "reign/frontend_prelude.hpp"
#include "fscope.h"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Theme check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught);};
 rejects([&]{IFace::GetColor(0);});reign::FrontendTheme first,second;
 first.run([&]{
  check(U32(IFace::GetColor(IFace::CLR_STD_BG))==0xc0c0c0c0);
  auto *group=IFace::FindColorGroup(Crc::CalcStr("Sys::Default"));check(group&&group==IFace::FrontendData().cgDefault);
  check(IFace::FrontendData().alphaScale==1&&IFace::FrontendData().backBuf==nullptr);
  check(IFace::FindColorGroup(Crc::CalcStr("missing"))==nullptr);
  rejects([&]{IFace::GetColor(IFace::MAX_COLORS);});
  auto *old=group;second.run([&]{check(IFace::FrontendData().cgDefault!=old);});check(IFace::FrontendData().cgDefault==old);
 });
 FScope colors(nullptr,"Colors");auto *c=colors.AddFunction("StdBg");for(int v:{1,2,3,4})c->AddArgInteger(v);colors.InitIterators();first.configure_colors(&colors);
 first.run([&]{check(U32(IFace::GetColor(IFace::CLR_STD_BG))==0x04010203);check(U32(IFace::FrontendData().cgDefault->bg[0])==0x04010203);});
 second.run([&]{check(U32(IFace::GetColor(IFace::CLR_STD_BG))==0xc0c0c0c0);});
 FScope custom(nullptr,"CreateColorGroup");custom.AddArgString("Menu");custom.AddArgString("Sys::Default");custom.AddFunction("AllFg")->AddArgString("White");custom.InitIterators();first.configure_group(&custom);
 first.run([&]{auto *g=IFace::FindColorGroup(Crc::CalcStr("Menu"));check(g&&g!=IFace::FrontendData().cgDefault);check(U32(g->bg[0])==0x04010203);for(auto &v:g->fg)check(U32(v)==0xffffffff);});
 FScope skin(nullptr,"CreateSkin");skin.AddArgString("Frame");auto *state=skin.AddFunction("State");state->AddArgString("All");state->AddFunction("ColorGroup")->AddArgString("Menu");auto *border=skin.AddFunction("Border");for(auto name:{"Point1","Point2"}){auto *p=border->AddFunction(name);p->AddArgInteger(2);p->AddArgInteger(3);}skin.InitIterators();first.configure_skin(&skin);
 first.run([&]{auto *s=IFace::FindTextureSkin(Crc::CalcStr("Frame"));check(s&&s->border.p0.x==2&&s->border.p1.y==3);for(auto &st:s->states)check(st.colors==IFace::FindColorGroup(Crc::CalcStr("Menu")));});
 rejects([&]{first.configure_skin(nullptr);});rejects([&]{IFace::FrontendData();});
 std::cout<<"Original UI theme defaults/colors/groups/skins/isolation: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
