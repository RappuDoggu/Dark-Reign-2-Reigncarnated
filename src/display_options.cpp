#include "display_options_slice.hpp"
#include "reign/display_options.hpp"
#include "iclistbox.h"
#include <limits>
namespace {
thread_local const reign::DisplaySnapshot *active=nullptr;
const reign::DisplaySnapshot &display(){if(!active)throw std::runtime_error("Native display snapshot not bound");return *active;}
IControl *factory(U32,IControl *parent,U32){return new Vid::Options(parent);}
IControl *graphics_factory(U32,IControl *parent,U32){return new Vid::Graphics(parent);}
ICListBox *list(IControl &control,U32 id){auto *item=control.Find(id);if(!item)return nullptr;auto *result=IFace::Promote<ICListBox>(item);if(!result)throw std::runtime_error("Display options child is not a listbox");return result;}
}
namespace reign {
DisplayOptions::DisplayOptions(DisplaySnapshot s){update(std::move(s));}
void DisplayOptions::update(DisplaySnapshot s){if(s.renderer.empty()||!s.width||!s.height||s.width>unsigned(std::numeric_limits<int>::max())||s.height>unsigned(std::numeric_limits<int>::max()))throw std::invalid_argument("Invalid native display snapshot");snapshot_=std::move(s);}
void DisplayOptions::register_class(){IFace::RegisterControlClass("Vid::Options",factory);IFace::RegisterControlClass("Vid::Graphics",graphics_factory);}
void DisplayOptions::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing display callback");auto *previous=active;active=&snapshot_;struct Restore{const DisplaySnapshot *p;~Restore(){active=p;}} restore{previous};call();}
}
namespace Vid {
void Options::FillDrivers(){auto &s=display();if(auto *box=list(*this,0x8A931010)){box->DeleteAllItems();box->AddTextItem(s.renderer.c_str(),nullptr);box->SetSelectedItem(1);}}
void Options::FillModes(){auto &s=display();if(auto *box=list(*this,0xE1E86F9F)){box->DeleteAllItems();auto name=std::to_string(s.width)+" x "+std::to_string(s.height)+" (Android surface)";box->AddTextItem(name.c_str(),nullptr);box->SetSelectedItem(1);}OnModeChange();}
void Options::OnModeChange(){auto &s=display();for(NList<Options>::Iterator it(&allWindows);*it;it++){auto *control=*it;control->drvName=s.renderer.c_str();
 // GL has no portable dedicated-VRAM measurement. Hide the original numeric
 // field rather than presenting a fabricated zero-megabyte GPU.
 for(const char *name : {"Mem","Memory"})if(auto *memory=control->Find(Crc::CalcStr(name)))memory->SetVisible(FALSE);
 control->SendNotify(control,0x46A402AB); // NoTripleBuf
 control->SendNotify(control,s.trilinear?0x0D95AA79:0xBA0A0855);
 control->SendNotify(control,0xF765541C); // RGBA texture uploads
 control->SendNotify(control,0x3DB725BF); // NoGamma
 control->SendNotify(control,s.multitexture?0xF7C32628:0x614B22FC);
 control->SendNotify(control,0xE281A7D9); // NoAntiAlias
 control->SendNotify(control,0x0D9215F9); // NoWindowed
 control->SendNotify(control,0x7811F848); // NoHardTL
}}
U32 Options::HandleEvent(Event &e){if(e.type==IFace::EventID()&&e.subType==IFace::NOTIFY){switch(e.iface.p1){
 case IControlNotify::Activated:FillDrivers();FillModes();break;
 case 0xF2F6914B:FillDrivers();return TRUE;
 case 0xD6FA23BD:FillModes();return TRUE;
 case 0xC7C383A3:throw std::runtime_error("Native graphics settings application not implemented");
 case 0xA3A50304:case 0x338BCFAC:throw std::runtime_error("DirectDraw mode cycling is unavailable on Android");
 }}return ICWindow::HandleEvent(e);}
}

namespace reign::display_options_detail {bool clip_guard(){display();return false;}void mode_changed(){display();VarSys::SetIntegerRange("vid.clip.guard.size",0,0);}}
