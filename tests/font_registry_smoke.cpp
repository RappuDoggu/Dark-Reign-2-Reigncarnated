#include "reign/font_registry.hpp"
#include "font_fixture.hpp"
#include <iostream>
int main(){try{
 unsigned reads=0,checks=0;bool broken=false;
 auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Registry check "+std::to_string(checks));};
 auto rejects=[&](auto action){bool threw=false;try{action();}catch(const std::runtime_error &){threw=true;}check(threw);};
 reign::FontRegistry registry([&](const std::string &name){if(name=="fonts.cfg"){
  std::string config="CreateFont(\"System\") { File(\"one.fff\"); } ConfigureInterface() { CreateFont(\"Tiny\") { File(\"ONE.FFF\"); } Metrics() { Unimplemented(1); } }";return std::vector<std::uint8_t>(config.begin(),config.end());}
  ++reads;if(broken)throw std::runtime_error("injected missing font");return font_fixture();});
 check(registry.configure("fonts.cfg")==2);check(reads==1&&registry.alias_count()==2&&registry.file_count()==1);
 auto first=registry.find("system");check(first&&first==registry.find("TINY"));check(!registry.find("unknown"));
 registry.create("SYSTEM","ignored.fff");check(reads==1&&registry.find("system")==first);
 registry.reload();check(reads==2&&registry.find("System")!=first&&registry.find("Tiny")==registry.find("System"));check(first->width(u"AB")==6);
 broken=true;rejects([&]{registry.create("Bad","bad.fff");});check(registry.alias_count()==2&&registry.file_count()==1);
 auto before=registry.find("system");rejects([&]{registry.reload();});check(registry.find("system")==before);
 broken=false;registry.clear();check(!registry.find("System")&&registry.alias_count()==0&&registry.file_count()==0);check(first->glyph_count()==2);
 registry.create("New","one.fff");check(registry.alias_count()==1);
 rejects([&]{registry.create(std::string(64,'a'),"one.fff");});
 rejects([&]{registry.create("","one.fff");});rejects([&]{registry.create(std::string("a\0b",3),"one.fff");});
 reign::FontRegistry other([](const std::string &){return font_fixture(1,1);});other.create("New","one.fff");check(other.find("new")!=registry.find("new")&&other.find("new")->glyph_count()==1);
 reign::FontRegistry *self=nullptr;reign::FontRegistry reentrant([&](const std::string &){self->find("New");return font_fixture();});self=&reentrant;
 rejects([&]{reentrant.create("New","one.fff");});check(reentrant.file_count()==0&&reentrant.alias_count()==0);
 std::cout<<"Original FontSys aliases/config/sharing/reload/error ownership: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
