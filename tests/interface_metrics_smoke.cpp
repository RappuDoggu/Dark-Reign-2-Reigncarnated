#include "reign/interface_metrics.hpp"
#include "reign/font_registry.hpp"
#include "font_fixture.hpp"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool b){++checks;if(!b)throw std::runtime_error("Metric check "+std::to_string(checks));};
 auto rejects=[&](auto f){bool threw=false;try{f();}catch(const std::exception &){threw=true;}check(threw);};
 reign::InterfaceMetrics metrics(640,480);
 const std::pair<const char *,int> defaults[]={{"SliderWidth",12},{"ButtonWidth",80},{"ButtonHeight",24},{"TitleHeight",16},{"ThinTitleHeight",11},{"DropShadowUp",2},{"DropShadowDown",1},{"ShadowAlpha",88},{"BorderThin",1},{"BorderThick",3},{"VGradient",150},{"HGradient",110},{"PollDelay",500},{"TipDelay",800}};
 for(auto p:defaults)check(metrics.get(p.first)==p.second);
 check(metrics.viewport().width==640&&metrics.viewport().height==480&&metrics.viewport().depth==32);
 auto loader=[](const std::string &name){if(name=="one.fff")return font_fixture();std::string s;
 if(name=="ok.cfg")s="ConfigureInterface() { CreateFont(\"Title\") { File(\"one.fff\"); } CreateFont(\"Small\") { File(\"one.fff\"); } Metrics() { TitleFont(\"Title\"); ThinTitleFont(\"Small\"); ButtonWidth(93); ShadowAlpha(77); TipDelay(-1); UnknownMetric(5); ButtonHeight(3.5); } }";
 else if(name=="bad.cfg")s="Metrics() { SliderWidth(); }";
 else if(name=="top.cfg")s="Metrics() { SliderWidth(19); }";
 else throw std::runtime_error("Missing test config");return std::vector<std::uint8_t>(s.begin(),s.end());};
 reign::FontRegistry fonts(loader);check(!metrics.font("TitleFont",fonts));check(fonts.configure("ok.cfg",&metrics)==2);
 check(metrics.font("titlefont",fonts)==fonts.find("TITLE"));check(metrics.font("ThinTitleFont",fonts)==fonts.find("Small"));
 check(metrics.get("buttonwidth")==93&&metrics.get("ShadowAlpha")==77&&metrics.get("TipDelay")==-1);
 check(metrics.get("ButtonHeight")==24); // Original unsupported floating argument logs and retains its old value.
 check(fonts.configure("top.cfg",&metrics)==0&&metrics.get("SliderWidth")==19);
 metrics.resize(1692,666);check(metrics.viewport().width==1692&&metrics.viewport().height==666&&metrics.get("ButtonWidth")==93);
 rejects([&]{metrics.resize(0,10);});check(metrics.viewport().width==1692);
 rejects([&]{metrics.get("unknown");});rejects([&]{metrics.get(std::string("TipDelay\0x",10));});rejects([&]{metrics.font("ButtonWidth",fonts);});
 rejects([&]{fonts.configure("bad.cfg",&metrics);});check(metrics.get("SliderWidth")==19);
 reign::InterfaceMetrics other(32,32);check(other.get("ButtonWidth")==80&&other.viewport().width==32);
 fonts.reload();check(metrics.font("TitleFont",fonts)==fonts.find("Title"));fonts.clear();check(!metrics.font("TitleFont",fonts));
 std::cout<<"Original IFace metric defaults/configuration/font CRC/resize/isolation: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
