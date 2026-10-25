#include "varfile_slice.hpp"
#include "reign/config.hpp"
#include "reign/archive.hpp"
#include <iostream>
void copy_node(const reign::ConfigNode &node,FScope &scope){
 for(const auto &arg:node.arguments){if(auto *v=std::get_if<std::int32_t>(&arg))scope.AddArgInteger(*v);else if(auto *v=std::get_if<float>(&arg))scope.AddArgFPoint(*v);else scope.AddArgString(std::get<std::string>(arg).c_str());}
 for(const auto &child:node.children)copy_node(child,*scope.AddFunction(child.name.c_str()));scope.InitIterators();
}
void inspect(VarSys::VarScope *scope,unsigned &entries,std::size_t &units){
 for(BinTree<VarSys::VarItem>::Iterator i(&scope->items);*i;i++){
  auto *item=*i;if(item->type==VarSys::VI_SCOPE){inspect(item->scope.ptr,entries,units);continue;}
  if(item->type!=VarSys::VI_BINARY)throw std::runtime_error("Unexpected non-binary language item");
  const auto size=item->BinarySize();if(size<4||(size&1))throw std::runtime_error("Invalid versioned UTF-16 language value");
  auto *p=item->Binary();bool high=false;
  for(U32 at=4;at<size;at+=2){U16 ch=U16(p[at])|(U16(p[at+1])<<8);
   if(high){if(ch<0xdc00||ch>0xdfff)throw std::runtime_error("Unpaired language surrogate");high=false;}
   else if(ch>=0xd800&&ch<=0xdbff)high=true;
   else if(ch>=0xdc00&&ch<=0xdfff)throw std::runtime_error("Unpaired language surrogate");
  }
  if(high)throw std::runtime_error("Truncated language surrogate");++entries;units+=(size-4)/2;
 }
}
int main(int argc,char **argv){try{
 if(argc!=2)throw std::invalid_argument("Usage: language_database_probe base.zwp");
 reign::Archive archive(argv[1]);auto tree=reign::parse_config([&](const auto &name){return archive.read(name);},"english.cfg");
 struct Session{Session(){TrackSys::Init();VarSys::Init();}~Session(){VarSys::Done();TrackSys::Done();}} session;
 bool loaded=false;for(const auto &node:tree)if(node.crc==reign::name_crc("DataBase")){FScope scope(nullptr,node.name.c_str());copy_node(node,scope);VarFile::Load(&scope,"multilanguage.english");loaded=true;}
 if(!loaded)throw std::runtime_error("Language database scope missing");unsigned entries=0;std::size_t units=0;
 inspect(VarSys::FindVarItem("multilanguage.english")->scope.ptr,entries,units);
 if(!entries)throw std::runtime_error("No language entries loaded");
 std::cout<<"Original PTree/VarFile/VarSys English database: PASS entries="<<entries<<" UTF16_units="<<units<<'\n';
 VarSys::DeleteItem("multilanguage");std::cout<<"Babel formatting/menu startup: NOT tested\n";
 }catch(const std::exception &e){std::cerr<<"LANGUAGE_PROBE_FAIL "<<e.what()<<'\n';return 1;}}
