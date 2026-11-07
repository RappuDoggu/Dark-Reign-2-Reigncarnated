#include "iface_events.hpp"
#include "icbutton.h"
#include "icstatic.h"
#include "iface_messagebox.h"
#include "reign/frontend_font.hpp"
#include "font_fixture.hpp"
#include "reign/interface_events.hpp"
#include "reign/frontend_theme.hpp"
#include "reign/frontend_frame.hpp"
#include "reign/frontend_effects.hpp"
#include "reign/frontend_draw.hpp"
#include <iostream>
namespace {
U32 selected=0;void selected_button(U32 id,U32){selected=id;}
IControl *button_factory(U32,IControl *parent,U32){return new ICButton(parent);}
IControl *static_factory(U32,IControl *parent,U32){return new ICStatic(parent);}
}
int main(){try{
 unsigned checks=0,batches=0,glyph_batches=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Live control check "+std::to_string(checks));};
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 reign::FontRegistry fonts([](const std::string&){return font_fixture();});fonts.create("System","a.fff");fonts.create("WindowTitle","a.fff");
 reign::FrontendFonts font_bridge(fonts,[&](const auto &,const auto &){++glyph_batches;});font_bridge.set_frame({{0,0},{640,480}},128);
 IControlPtr tracked;
 reign::InterfaceMetrics metrics(640,480);reign::FrontendFrame frame(metrics);frame.set_time({16,16,.016f});
 reign::FrontendTheme theme;reign::FrontendEffects effects;reign::InterfaceEvents events;
 events.run([&]{theme.run([&]{frame.run([&]{effects.run([&]{font_bridge.run([&]{
  ICRoot root;root.SetName("Root");auto &state=reign::iface_event_detail::state();state.root=&root;state.treeReady=TRUE;
  struct Detach{reign::iface_event_detail::State &s;~Detach(){s.root=nullptr;s.treeReady=FALSE;}} detach{state};
  IFace::RegisterControlClass("Button",button_factory);
  IFace::RegisterControlClass("Static",static_factory);
  auto *button=IFace::CreateControl("Launch", "Button", &root);check(button!=nullptr);tracked=button;
  button->SetPos(20,30);button->SetSize(120,32);
  check(root.Activate());check(root.IsActive()&&button->IsActive());
  check(IFace::RootWindow()==&root&&IFace::FindByName("Launch")==button);
  auto pi=root.GetPaintInfo();
  reign::FrontendDraw::run_batches([&]{root.Draw(pi);},[&](const reign::FrontendBatch &batch){check(!batch.vertices.empty()&&!batch.indices.empty());++batches;});
  check(batches>0);events.process();
  const CH label[]={'A','B',0};const CH body[]={'A','B','\\','n','A','B',0};
  auto *box=IFace::MsgBox(label,body,0,new MBEventCallback("Accept",label,selected_button,0),new MBEventCallback("Other",label,selected_button,0),new MBEventCallback("Cancel",label,selected_button,0));
  IControlPtr box_track=box;check(box&&box->IsActive()&&IFace::GetModal()==box);
  pi=root.GetPaintInfo();reign::FrontendDraw::run_batches([&]{root.Draw(pi);},[&](const auto &){++batches;});check(glyph_batches>0);
  Event notify{};notify.type=IFace::EventID();notify.subType=IFace::NOTIFY;notify.iface.p1=Crc::CalcStr("Accept");
  check(box->HandleEvent(notify)!=0&&selected==Crc::CalcStr("Accept"));
  IControl::DeleteMarkedControls();events.process();check(!box_track.Alive()&&IFace::GetModal()==nullptr);
  auto rejects=[&](auto call){bool caught=false;try{call();}catch(const std::exception&){caught=true;}check(caught);};
  MBEvent unused("Unused",label);
  std::vector<CH> tooLong(1024,'A');tooLong.push_back(0);
  rejects([&]{IFace::MsgBox(label,tooLong.data(),0,&unused);});
  std::vector<CH> tooMany;for(unsigned i=0;i<21;++i){if(i)tooMany.insert(tooMany.end(),{'\\','n'});tooMany.push_back('A');}tooMany.push_back(0);
  rejects([&]{IFace::MsgBox(label,tooMany.data(),0,&unused);});
  rejects([&]{IFace::MsgBox(label,label,0,&unused,nullptr,&unused);});
  for(CH finalChar:{CH('\\'),CH('\r')}){
   CH trailing[]={'A',finalChar,0};auto *tail=IFace::MsgBox(label,trailing,0,new MBEventCallback("Tail",label,selected_button,0));
   check(tail->IsActive());tail->Deactivate();tail->MarkForDeletion();IControl::DeleteMarkedControls();events.process();
  }


  check(root.Deactivate());check(!button->IsActive());events.process();
 });});});});});
 check(!tracked.Alive());
 std::cout<<"Live original root/button factory/activation/draw/modal-message/deactivation: PASS ("<<checks<<" checks, "<<batches<<" batches); real menu not loaded\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
