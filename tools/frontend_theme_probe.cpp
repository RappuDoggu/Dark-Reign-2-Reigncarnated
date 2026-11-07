#include "reign/frontend_theme.hpp"
#include "reign/frontend_bitmap.hpp"
#include "reign/bitmap_manager.hpp"
#include "reign/frontend_prelude.hpp"
#include "reign/config.hpp"
#include "reign/archive.hpp"
#include "fscope.h"
#include <iostream>
void copy_node(const reign::ConfigNode &node,FScope &scope){
 for(const auto &arg:node.arguments){
  if(const auto *v=std::get_if<std::int32_t>(&arg))scope.AddArgInteger(*v);
  else if(const auto *v=std::get_if<float>(&arg))scope.AddArgFPoint(*v);
  else scope.AddArgString(std::get<std::string>(arg).c_str());
 }
 for(const auto &child:node.children)copy_node(child,*scope.AddFunction(child.name.c_str()));
 scope.InitIterators();
}
int main(int argc,char **argv){try{
 if(argc<2)throw std::invalid_argument("Usage: frontend_theme_probe base.zwp [config-name ...]");
 reign::Archive archive(argv[1]);unsigned reads=0,groups=0,skins=0,colors=0,skipped=0;
 auto loader=[&](const std::string &name){++reads;return archive.read(name);};
 reign::BitmapManager manager(loader,[&](const std::string &name){return archive.find(name)!=nullptr;});
 reign::FrontendTextures textures(manager);reign::FrontendTheme theme;
 std::vector<std::string> configs;for(int i=2;i<argc;++i)configs.push_back(argv[i]);if(configs.empty())configs.push_back("interface_standard_skin.cfg");
 textures.run([&]{for(const auto &config:configs){
 auto tree=reign::parse_config(loader,config);
 for(const auto &node:tree){
  if(node.crc!=reign::name_crc("ConfigureInterface")){++skipped;continue;}
  for(const auto &child:node.children){
   FScope scope(nullptr,child.name.c_str());copy_node(child,scope);
   if(child.crc==reign::name_crc("Colors")){theme.configure_colors(&scope);++colors;}
   else if(child.crc==reign::name_crc("CreateColorGroup")){theme.configure_group(&scope);++groups;}
   else if(child.crc==reign::name_crc("CreateSkin")){theme.configure_skin(&scope);++skins;}
   else ++skipped;
  }
 }}});
 if(!groups||!skins||!manager.count())throw std::runtime_error("Theme probe did not exercise groups, skins and image loading");
 std::cout<<"Original PTree/FScope/theme/bitmap integration: PASS groups="<<groups<<" skins="<<skins<<" color_tables="<<colors<<" bitmaps="<<manager.count()<<" reads="<<reads<<" other_statements_not_executed="<<skipped<<'\n';
 std::cout<<"Controls/engine startup/menu/gameplay: NOT tested\n";
 }catch(const std::exception &e){std::cerr<<"THEME_PROBE_FAIL "<<e.what()<<'\n';return 1;}}
