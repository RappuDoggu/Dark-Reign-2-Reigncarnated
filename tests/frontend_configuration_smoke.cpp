#include "frontend_configuration_slice.hpp"
#include "reign/frontend_configuration.hpp"
#include "reign/command_scopes.hpp"
#include "reign/interface_events.hpp"
#include "reign/frontend_frame.hpp"
#include "reign/frontend_effects.hpp"
#include "reign/frontend_font.hpp"
#include "reign/frontend_draw.hpp"
#include "reign/engine_file.hpp"
#include "icbutton.h"
#include "font_fixture.hpp"
#include <iostream>
namespace {IControl *button_factory(U32,IControl *parent,U32){return new ICButton(parent);}}
int main(){try{
 unsigned checks=0,batches=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Configuration check "+std::to_string(checks));};
 auto rejects=[&](auto fn){bool caught=false;try{fn();}catch(const std::exception&){caught=true;}check(caught);};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 std::string text=R"(ConfigureInterface(){
 CreateFont("System"){File("fixture.fff");}
 Metrics(){ButtonWidth(91);}
 CreateColorGroup("MenuColors"){NormalBg(12,34,56,255);}
 DefineControlType("MenuButton","Button"){Size(90,24);Font("System");Text("AB");ColorGroup("MenuColors");}
 CreateRegData("Sample"){SampleValue(7);}
 }
 CreateControl("FromConfig","MenuButton"){Pos(4,5);})";
 auto loader=[&](const std::string &name){if(name=="fixture.fff")return font_fixture();if(name!="menu.cfg")throw FileSys::FileNotFound(name);return std::vector<U8>(text.begin(),text.end());};
 reign::InterfaceMetrics metrics(640,480);reign::FrontendFrame frame(metrics);frame.set_time({16,16,.016f});
 reign::FrontendTheme theme;reign::FontRegistry fonts(loader);reign::FrontendEffects effects;reign::InterfaceEvents events;
 reign::FrontendFonts font_bridge(fonts,[&](const auto &,const auto &){++batches;});font_bridge.set_frame({{0,0},{640,480}},128);
 reign::CommandScopes scopes(loader);reign::FrontendConfiguration config(theme,fonts,metrics);config.register_scopes(scopes);
 events.run([&]{theme.run([&]{frame.run([&]{effects.run([&]{font_bridge.run([&]{config.run([&]{
  ICRoot root;root.SetName("Root");auto &state=reign::iface_event_detail::state();state.root=&root;state.treeReady=TRUE;
  struct Detach{reign::iface_event_detail::State &s;~Detach(){s.root=nullptr;s.treeReady=FALSE;}} detach{state};
  IFace::RegisterControlClass("Button",button_factory);check(root.Activate());
  check(scopes.execute("menu.cfg"));check(config.processed()==5&&metrics.get("ButtonWidth")==91&&fonts.alias_count()==1);
  auto *button=IFace::FindByName("FromConfig");check(button&&button->GetSize().x==90&&button->GetPos().x==4);check(button->Activate());
  auto *data=IFace::FindRegData("Sample",TRUE,FALSE);check(data&&data->NextArgInteger()==7);
  auto pi=root.GetPaintInfo();reign::FrontendDraw::run_batches([&]{root.Draw(pi);},[&](const auto &){++batches;});check(batches>0);
  text="ConfigureInterface(){UnknownSetting();}";rejects([&]{scopes.execute("menu.cfg");});
  text="ConfigureInterface(){Sound(){}}";rejects([&]{scopes.execute("menu.cfg");});
  text="CreateControl(\"Missing\",\"UnportedWidget\"){}";rejects([&]{scopes.execute("menu.cfg");});
  check(root.Deactivate());events.process();
 });});});});});});
 text="ConfigureInterface(){}";rejects([&]{scopes.execute("menu.cfg");});
 std::cout<<"Original config execution/font/metrics/derived type/live control/draw: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
