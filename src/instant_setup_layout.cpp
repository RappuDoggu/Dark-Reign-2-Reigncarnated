#include "reign/instant_setup_layout.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
namespace reign {
namespace {
using Nodes=std::vector<ConfigNode>;
std::string arg(const ConfigNode &n,unsigned i=0){if(i>=n.arguments.size()||!std::holds_alternative<std::string>(n.arguments[i]))throw std::runtime_error("Invalid original setup scope: "+n.name);return std::get<std::string>(n.arguments[i]);}
ConfigNode node(std::string name,std::vector<ConfigValue> args={},Nodes children={}){return {std::move(name),0,std::move(args),std::move(children)};}
ConfigNode notify(const char *message){return node("TranslateEvent",{std::string("Button::Notify::Pressed"),std::string(message),std::string("<<")});}
void emit(std::ostream &out,const Nodes &nodes){for(const auto &n:nodes){out<<n.name<<'(';for(unsigned i=0;i<n.arguments.size();++i){if(i)out<<',';std::visit([&](const auto &v){using T=std::decay_t<decltype(v)>;if constexpr(std::is_same_v<T,std::string>){if(v.find('"')!=std::string::npos||v.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("Unsupported quote in original setup scope");out<<'"'<<v<<'"';}else out<<v;},n.arguments[i]);}out<<')';if(n.children.empty())out<<";\n";else{out<<"{\n";emit(out,n.children);out<<"}\n";}}}
void bind_button(ConfigNode &button,const char *message){auto &c=button.children;c.erase(std::remove_if(c.begin(),c.end(),[](const auto &n){return n.name=="OnEvent"||n.name=="TranslateEvent";}),c.end());c.push_back(notify(message));}
}
std::string original_instant_layout(ConfigLoader loader){
 auto base=parse_config(loader,"interface_setup_base.cfg"),instant=parse_config(loader,"interface_setup_instant.cfg");Nodes defs;bool players=false,mission=false,menu=false;ConfigNode root;
 for(const auto &top:base)if(top.name=="ConfigureInterface")for(auto def:top.children){
  if(def.name=="CreateColorGroup"){defs.push_back(std::move(def));continue;}
  if(def.name!="DefineControlType")continue;auto name=arg(def);
  if(name=="GameConfigPlayers"){
   auto &children=def.children;children.erase(std::remove_if(children.begin(),children.end(),[](const auto &c){return c.name=="CreateControl"&&arg(c)!="AddAi";}),children.end());
   for(auto &c:children)if(c.name=="CreateControl")bind_button(c,"NativeSetup::AI");
   defs.push_back(std::move(def));players=true;
  }else if(name=="GameConfigMission"){
   for(auto &c:def.children)if(c.name=="CreateControl"&&arg(c)=="Preview")c.arguments[1]=std::string("Static");
   defs.push_back(std::move(def));mission=true;
  }else if(name=="GameConfigMenu"){
   auto &c=def.children;c.erase(std::remove_if(c.begin(),c.end(),[](const auto &n){return n.name=="AddItem"&&arg(n)!="Back"&&arg(n)!="Map"&&arg(n)!="Launch";}),c.end());
   for(auto &item:c)if(item.name=="AddItem"){auto name=arg(item);bind_button(item,name=="Back"?"NativeSetup::Back":name=="Map"?"NativeSetup::Map":"NativeSetup::Launch");}
   defs.push_back(std::move(def));menu=true;
  }
 }
 for(auto &top:instant)if(top.name=="CreateControl"&&arg(top)=="GameSetup::Instant")root=top;
 if(!players||!mission||!menu||root.name.empty())throw std::runtime_error("Original Instant Action layout is incomplete; import the complete base pack");
 root.arguments[1]=std::string("Native::InstantSetup");
 auto &children=root.children;children.erase(std::remove_if(children.begin(),children.end(),[](const auto &n){return n.name=="OnEvent"||(n.name=="CreateControl"&&arg(n)!="GameConfigPlayers"&&arg(n)!="GameConfigMission"&&arg(n)!="GameConfigMenu");}),children.end());
 children.insert(children.begin(),{node("Size",{640,480}),node("Style",{std::string("Transparent"),std::string("NoAutoActivate"),std::string("!DropShadow")})});
 for(auto &c:children)if(c.name=="CreateControl"&&arg(c)=="GameConfigPlayers"){
  auto extra=parse_config([](const std::string &){std::string cfg=R"(
CreateControl("Player","Static"){Pos(12,12);Size(270,30);Font("WindowTitle");Style("Transparent");Text("Commander");}
CreateControl("Opponent","Static"){Pos(12,52);Size(270,30);Font("WindowTitle");Style("Transparent");}
CreateControl("Difficulty","Button"){Pos(12,100);Size(270,34);Font("System");TranslateEvent("Button::Notify::Pressed","NativeSetup::Difficulty","<<");}
CreateControl("SeedLabel","Static"){Pos(12,148);Size(100,25);Font("System");Style("Transparent");Text("Random seed");}
CreateControl("Seed","Edit"){Pos(118,145);Size(160,34);Font("System");UseVar("gamegod.instant.seed");}
CreateControl("Limit","Static"){Pos(12,192);Size(275,48);Font("System");Style("Transparent");Text("Offline: one AI. Team options pending.");}
)";return std::vector<std::uint8_t>(cfg.begin(),cfg.end());},"offline-players.cfg");
  c.children.insert(c.children.end(),extra.begin(),extra.end());
 }
 Nodes result{node("ConfigureInterface",{},std::move(defs)),std::move(root)};std::ostringstream out;emit(out,result);
 // The map chooser replaces the unavailable Windows host-session controller;
 // the surrounding GameConfigMission/GameConfigMenu are the imported controls.
 out<<R"(
CreateControl("GameConfigHostOptions","Native::InstantSetup"){
 ReadTemplate("Std::Window");Geometry("HCentre","VCentre");Size(490,365);Style("NoAutoActivate","Modal","TitleBar");Text("Select map");
 CreateControl("Maps","Listbox"){Pos(10,10);Size(470,280);Style("VSlider");UseVar("gamegod.instant.map");ItemConfig(){Font("System");Geometry("AutoSizeY","ParentWidth");Size(0,9);}}
 CreateControl("Done","Button"){Pos(330,305);Size(150,34);Font("System");Text("Done");TranslateEvent("Button::Notify::Pressed","NativeSetup::MapDone","<");}
})";
 return out.str();
}
}
