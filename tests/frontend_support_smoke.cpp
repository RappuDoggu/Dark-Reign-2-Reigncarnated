#include "reign/frontend_support.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
int main(){try{
 unsigned n=0;auto check=[&](bool b){++n;if(!b)throw std::runtime_error("Frontend support check "+std::to_string(n));};
 CH text[]={0x41,0xE9,0xD83D,0xDE00,0};
 check(Utils::Strlen(text)==4); // UTF-16 units, not Unicode scalar count.
 std::unique_ptr<CH[]> copy(Utils::Strdup(text));check(copy.get()!=text&&copy[1]==0xE9&&copy[3]==0xDE00&&copy[4]==0);
 CH dest[8];for(auto &c:dest)c=0xFFFF;
 check(Utils::Strmcpy(dest,text,8)==dest);check(dest[3]==0xDE00&&dest[4]==0&&dest[7]==0);
 for(auto &c:dest)c=0xFFFF;
 Utils::Strmcpy(dest,text,3);check(dest[0]==0x41&&dest[1]==0xE9&&dest[2]==0&&dest[3]==0xFFFF);
 Utils::Strmcpy(dest,text,1);check(dest[0]==0);
 bool rejected=false;try{Utils::Strmcpy(dest,text,0);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 for(U32 alignment:{0u,1u,8u,16u,64u}){
  void *p=Debug::Memory::Aligning::AligningAlloc(37,alignment);check(p!=nullptr&&(!alignment||reinterpret_cast<std::uintptr_t>(p)%alignment==0));
  static_cast<unsigned char*>(p)[36]=0xAB;Debug::Memory::Aligning::AligningFree(p);
 }
 check(Debug::Memory::Aligning::AligningAlloc(0,16)==nullptr);Debug::Memory::Aligning::AligningFree(nullptr);
 rejected=false;try{Debug::Memory::Aligning::AligningAlloc(1,3);}catch(const std::invalid_argument&){rejected=true;}check(rejected);
 std::cout<<"Native UTF-16 buffers and original Array allocation support: PASS ("<<n<<" checks)\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
