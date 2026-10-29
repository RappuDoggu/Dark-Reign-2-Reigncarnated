#include "reign/archive.hpp"
#include "reign/font_registry.hpp"
#include "reign/interface_metrics.hpp"
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::runtime_error("Usage: font_registry_probe installation/base.zwp");
 reign::Archive archive(argv[1]);unsigned reads=0,fontReads=0,fonts=0;reign::FontRegistry registry([&](const std::string &name){++reads;if(name.size()>4&&name.substr(name.size()-4)==".fff")++fontReads;return archive.read(name);});
 for(const auto &e:archive.entries())if(e.name.size()>4&&e.name.substr(e.name.size()-4)==".fff"){
  registry.create(e.name,e.name);registry.create("alias"+std::to_string(fonts),e.name);++fonts;
 }
 if(!fonts)throw std::runtime_error("No font entries");
 if(registry.file_count()!=fonts||registry.alias_count()!=fonts*2||reads!=fonts)throw std::runtime_error("Original font sharing mismatch");
 registry.reload();if(reads!=fonts*2)throw std::runtime_error("Mode-change reload count mismatch");
 std::cout<<"Original FontSys: files="<<registry.file_count()<<" aliases="<<registry.alias_count()<<" reads_after_reload="<<reads<<" PASS\n";
 registry.clear();const unsigned before=reads,beforeFonts=fontReads;reign::InterfaceMetrics metrics(1692,666);const auto configured=registry.configure("interface_standard_fonts.cfg",&metrics);
 std::cout<<"CONFIG observed: declarations="<<configured<<" aliases="<<registry.alias_count()<<" files="<<registry.file_count()<<" reads="<<reads-before<<" system_button_shared="<<(registry.find("System")==registry.find("Button"))<<"\n";
 if(configured!=9||registry.alias_count()!=9||registry.file_count()!=6||fontReads-beforeFonts!=6||registry.find("System")!=registry.find("Button")||registry.find("System")!=registry.find("Subtitle"))throw std::runtime_error("Real interface font configuration mismatch");
 if(metrics.font("TitleFont",registry)!=registry.find("WindowTitle")||metrics.font("ThinTitleFont",registry)!=registry.find("System"))throw std::runtime_error("Original configured metric/font resolution mismatch");
 std::cout<<"Original Metrics: TitleFont=WindowTitle ThinTitleFont=System ShadowAlpha="<<metrics.get("ShadowAlpha")<<" screen="<<metrics.viewport().width<<"x"<<metrics.viewport().height<<" PASS\n";
 std::cout<<"Original PTree/FScope -> FontSys real interface_standard_fonts.cfg: declarations="<<configured<<" shared_files="<<registry.file_count()<<" PASS\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
