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
#include "iclistbox.h"
#include "icmenu.h"
#include "colorbutton.h"
#include "display_options_slice.hpp"
#include "reign/display_options.hpp"
#include "game_saveload.h"
#include "reign/save_load_services.hpp"
#include "icsoundconfig.h"
#include "reign/sound_settings.hpp"
#include "console_viewer.h"
#include "console_slice.hpp"
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
  auto *list=new ICListBox(&root);list->SetName("ProfileList");list->SetSize(150,80);
  list->AddTextItem("ProfileA",nullptr);list->AddTextItem("ProfileB",nullptr);check(list->ItemCount()==2);
  check(list->SetSelectedItem("ProfileB",TRUE,FALSE));const char *selected=nullptr;check(list->GetSelectedItem(selected)&&std::string(selected)=="ProfileB");
  list->DeleteAllItems();check(list->ItemCount()==0);
  auto *menu=new ICMenu(&root);menu->SetName("TestMenu");CH caption[]={'A',0};check(menu->AddItem("Action",caption,"TestAction")!=nullptr);

  S32 red=0,green=0,blue=0,hue=0,saturation=0,luminosity=0;
  CalculateRGB(red,green,blue,0,0,255);check(red==255&&green==255&&blue==255);
  CalculateHLS(255,0,0,hue,saturation,luminosity);check(hue==0&&saturation==255&&luminosity==127);
  VarSys::CreateInteger("testconsole.color",0x00ff0000);
  auto *colorButton=new ColorButton(&root);colorButton->SetName("Color");
  FScope useColor(nullptr,"UseVar");useColor.AddArgString("testconsole.color");useColor.InitIterators();colorButton->Setup(&useColor);check(colorButton->Activate());
  auto before=batches;reign::FrontendDraw::run_batches([&]{root.Draw(pi);},[&](const auto &){++batches;});check(batches>before);
  reign::DisplayOptions display({"Test surface",640,480,false,false});
  reign::DisplayOptions::register_class();
  auto *options=new Vid::Options(&root);options->SetName("NativeDisplay");
  auto *drivers=new ICListBox(options);drivers->SetName("DriverList");
  auto *modes=new ICListBox(options);modes->SetName("ModeList");
  Event event{};event.type=IFace::EventID();event.subType=IFace::NOTIFY;
  event.iface.p1=Crc::CalcStr("FillDrivers");
  rejects([&]{options->HandleEvent(event);});
  display.run([&]{check(options->HandleEvent(event));check(drivers->ItemCount()==1);
   event.iface.p1=Crc::CalcStr("FillModes");check(options->HandleEvent(event));check(modes->ItemCount()==1);
   const char *mode=nullptr;check(modes->GetSelectedItem(mode)&&std::string(mode)=="640 x 480 (Android surface)");
   display.update({"Rotated surface",480,640,false,false});check(options->HandleEvent(event));
   check(modes->GetSelectedItem(mode)&&std::string(mode)=="480 x 640 (Android surface)");
   check(std::string(options->drvName)=="Rotated surface");
   event.iface.p1=Crc::CalcStr("SetMode");rejects([&]{options->HandleEvent(event);});
  });
  event.iface.p1=Crc::CalcStr("FillModes");rejects([&]{options->HandleEvent(event);});
  rejects([&]{display.update({"",640,480});});

  reign::SaveLoadServices::register_class();auto *saveLoad=new Game::SaveLoad(&root);saveLoad->SetName("SaveDialog");
  event.iface.p1=Game::SaveLoadMsg::Select;rejects([&]{saveLoad->HandleEvent(event);});
  unsigned infos=0,saves=0,loads=0;reign::SaveMission mission{"TestMission",false};
  reign::SaveLoadServices saveservice;
  saveservice.info=[&](const std::string &){++infos;return reign::SaveSlotInfo{true,"Saved mission"};};
  saveservice.active_mission=[&]{return &mission;};saveservice.date_time=[]{return "TestTime";};
  saveservice.save=[&](const std::string &,const std::string &description){++saves;check(description=="TestMission TestTime");return true;};
  saveservice.load=[&](const std::string &){++loads;return true;};
  saveservice.run([&]{check(saveLoad->HandleEvent(event));check(infos==1);
   event.iface.p1=Game::SaveLoadMsg::SaveRequest;check(saveLoad->HandleEvent(event));check(infos==2);
   event.iface.p1=Game::SaveLoadMsg::Save;check(saveLoad->HandleEvent(event));check(saves==1);
   event.iface.p1=Game::SaveLoadMsg::Load;check(saveLoad->HandleEvent(event));check(loads==1);
  });
  rejects([&]{saveLoad->HandleEvent(event);});

  reign::DisabledAudioSettings silent;silent.digital_volume=.5f;auto *sound=new ICSoundConfig(&root);sound->SetName("Sound");
  silent.run([&]{check(sound->Activate());check(!reign::disabled_audio_detail::Digital::Claimed3D());check(!reign::disabled_audio_detail::Redbook::Claim());
   reign::disabled_audio_detail::Digital::SetVolume(.25f);check(silent.digital_volume==.25f);
   rejects([&]{reign::disabled_audio_detail::Digital::SetVolume(2);});
   rejects([&]{reign::disabled_audio_detail::Redbook::Play(1);});check(sound->Deactivate());
  });
  rejects([&]{reign::disabled_audio_detail::Digital::Volume();});
  auto *viewer=new ConsoleViewer(&root);viewer->SetName("TestConsole");viewer->SetSize(150,80);VarSys::CreateString("testconsole.selected","");viewer->SetSelectedItemVar("testconsole.selected");FScope item(nullptr,"ItemConfig");item.AddFunction("Font")->AddArgString("System");item.InitIterators();viewer->Setup(&item);
  reign::run_console([&]{try {check(viewer->Activate());Console::AddString("AB",7);check(viewer->ItemCount()==1);viewer->Rebuild();check(viewer->ItemCount()==1);check(viewer->Deactivate());}catch(const std::exception &e){std::cerr<<"Viewer failure: "<<e.what()<<"\n";if(viewer->IsActive())viewer->Deactivate();root.Deactivate();throw;}},[](VarSys::VarItem *,U32){});
  check(root.Deactivate());events.process();
 });});});});});});
 VarSys::DeleteItem("testconsole");VarSys::DeleteItem("iface");
 text="ConfigureInterface(){}";rejects([&]{scopes.execute("menu.cfg");});
 std::cout<<"Original config execution/font/metrics/derived type/live control/draw: PASS ("<<checks<<" checks)\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
