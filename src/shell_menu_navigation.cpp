#include "reign/shell_menu_navigation.hpp"
#include <cmath>
#include <sstream>
#include <stdexcept>
namespace reign {
namespace {
const ConfigNode &one(const ConfigNode &parent,const std::string &name){
 const ConfigNode *result=nullptr;for(const auto &n:parent.children)if(n.name==name){if(result)throw std::runtime_error("Duplicate shell navigation scope: "+name);result=&n;}
 if(!result)throw std::runtime_error("Missing shell navigation scope: "+name);return *result;
}
std::string text(const ConfigNode &node){if(node.arguments.size()!=1||!std::holds_alternative<std::string>(node.arguments[0]))throw std::runtime_error("Invalid shell navigation text");return std::get<std::string>(node.arguments[0]);}
float time(const ConfigNode &node){float value;
 if(node.arguments.size()!=1)throw std::runtime_error("Invalid shell navigation time");
 if(auto *i=std::get_if<std::int32_t>(&node.arguments[0]))value=float(*i);else if(auto *f=std::get_if<float>(&node.arguments[0]))value=*f;else throw std::runtime_error("Shell navigation time is text");
 if(!std::isfinite(value)||value<0||value>86400)throw std::runtime_error("Invalid shell navigation time");return value;
}
void clock(float now){if(!std::isfinite(now)||now<0)throw std::invalid_argument("Invalid shell navigation clock");}
}
ShellMenuNavigation::Transition ShellMenuNavigation::read(const std::vector<ConfigNode> &nodes,const std::string &name){
 const ConfigNode *found=nullptr;
 for(const auto &node:nodes)if(node.name=="CreateObjectType"&&!node.arguments.empty()&&std::holds_alternative<std::string>(node.arguments[0])&&std::get<std::string>(node.arguments[0])==name){if(found)throw std::runtime_error("Duplicate shell navigation objective: "+name);found=&node;}
 if(!found)throw std::runtime_error("Missing original shell navigation objective: "+name);
 const auto &cinematic=one(one(one(*found,"ObjectiveObj"),"Action"),"Cineractive");
 Transition result{"",-1};unsigned bookmarks=0,completion=0;
 for(const auto &at:cinematic.children){
  if(at.name!="At")throw std::runtime_error("Unsupported shell navigation instruction: "+at.name);
  const float when=time(at);
  for(const auto &action:at.children){
   if(action.name=="SetBookmark"){
    if(when!=0||++bookmarks!=1)throw std::runtime_error("Unsupported shell navigation bookmark schedule");
    result.bookmark=text(one(action,"Name"));
   }else if(action.name=="Action"){
    for(const auto &command:action.children){
     if(command.name!="Cmd")throw std::runtime_error("Unsupported shell navigation action: "+command.name);
     std::istringstream words(text(command));std::string cmd,target,event,extra;words>>cmd>>target>>event;
     if(cmd!="iface.sendnotifyevent"||target!="Shell::EventHandler"||event!="CineractiveCompleted"||(words>>extra)||++completion!=1)throw std::runtime_error("Unsupported shell navigation completion command");
     result.ready=when;
    }
   }else throw std::runtime_error("Unsupported shell navigation action: "+action.name);
  }
 }
 if(bookmarks!=1||completion!=1||result.bookmark.empty()||result.ready<=0)throw std::runtime_error("Incomplete shell navigation objective: "+name);
 return result;
}
ShellMenuNavigation::ShellMenuNavigation(const std::vector<ConfigNode> &nodes):enter_(read(nodes,"Cineractive::Multi::Enter")),exit_(read(nodes,"Cineractive::Multi::Exit")){}
bool ShellMenuNavigation::main_ready(bool finished)const{return page_==Page::Main&&(returned_||finished);}
std::string ShellMenuNavigation::begin_instant(float now,bool finished){clock(now);if(!main_ready(finished))throw std::logic_error("Wait for the main menu entrance to finish");page_=Page::EnteringInstant;started_=now;return enter_.bookmark;}
std::string ShellMenuNavigation::return_main(float now){clock(now);if(page_==Page::Main||page_==Page::ReturningMain)return {};page_=Page::ReturningMain;started_=now;return exit_.bookmark;}
ShellMenuNavigation::Event ShellMenuNavigation::advance(float now){clock(now);if(now<started_)throw std::invalid_argument("Shell navigation clock moved backwards");
 if(page_==Page::EnteringInstant&&now-started_>=enter_.ready){page_=Page::Instant;return Event::ShowInstant;}
 if(page_==Page::ReturningMain&&now-started_>=exit_.ready){page_=Page::Main;returned_=true;return Event::ShowMain;}
 return Event::None;
}
void ShellMenuNavigation::launch(){if(page_!=Page::Instant)throw std::logic_error("Instant Action setup is not ready");page_=Page::Launching;}
void ShellMenuNavigation::reset(){page_=Page::Main;started_=0;returned_=false;}
}
