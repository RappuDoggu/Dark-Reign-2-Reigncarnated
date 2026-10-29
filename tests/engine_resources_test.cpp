#include "resource_prelude.h"
#include "filesys.h"
#include "blockfile.h"
#include <array>
#include <filesystem>
#include <iostream>
#include <vector>
#include <unistd.h>
void check(bool ok,const char *what) { if(!ok) throw std::runtime_error(what); }
struct Session {
  Session() { FileSys::Init(); }
  ~Session() { FileSys::Done(); }
};
struct Data {
  FileSys::DataFile *p;
  explicit Data(const char *name):p(FileSys::Open(name)) { check(p,"Open resource"); }
  ~Data() { FileSys::Close(p); }
};
int main(int argc,char **argv) {
 try {
  Session session;
  if(argc!=2) throw std::runtime_error("Usage: engine_resources_test owned-installation");
  auto root=std::filesystem::absolute(argv[1]);
  check(FileSys::AddSrcPack("base",(root/"packs/base.zwp").c_str()),"Register original base archive");
  check(FileSys::AddSrcPack("mission",(root/"missions/multiplayer/2crazed.zwp").c_str()),"Register mission archive");
  check(FileSys::AddSrcStream("mission","base"),"Mission inherits base resources");
  check(FileSys::SetActiveStream("mission"),"Activate mission stream");
  check(FileSys::Exists("GAME.CFG"),"Case-insensitive original archive lookup");
  check(!FileSys::Exists("missing-proof-file.cfg"),"Missing file remains missing");
  {
   Data game("game.cfg");
   const U32 size=game.p->Size(); check(size>8,"Mission configuration size");
   check(game.p->Seek(size-3),"Seek archive tail");
   std::array<U8,64> tail; tail.fill(0xa5);
   check(game.p->Read(tail.data(),tail.size())==3,"Clipped archive read count");
   for(std::size_t i=3;i<tail.size();++i) check(tail[i]==0xa5,"Clipped read preserves destination tail");
   check(!game.p->Seek(size+1),"Reject seek beyond entry");
  }
  check(FileSys::Exists("difficulty.cfg"),"Mission inherits original base lookup");
  { Data direct((root/"missions/multiplayer/group.cfg").c_str()); check(direct.p->Size()>0,"Absolute native path opens through original FileSys"); }
  U32 objects=0;
  {
   PTree game; check(game.AddFile("game.cfg"),"Original PTree reads mission config through FileSys");
   auto world=game.GetGlobalScope()->GetFunction("WorldInfo");
   auto width=world->GetFunction("CellMapX")->NextArgInteger();
   auto height=world->GetFunction("CellMapZ")->NextArgInteger();
   check(width==100&&height==100,"Expected 2crazed dimensions");
   PTree definitions; check(definitions.AddFile("objects.cfg"),"Original object config parser");
   auto scope=definitions.GetGlobalScope(); FScope *next;
   while((next=scope->NextFunction())) if(next->NameCrc()==Crc::CalcStr("CreateObject")) ++objects;
   check(objects==125,"Expected object definitions");
   std::cout<<"PASS original FileSys + PTree: 2crazed, "<<width<<'x'<<height<<", "<<objects<<" object definitions\n";
  }
  {
   BlockFile terrain; check(terrain.Open("terrain.blk",BlockFile::READ),"Original terrain BlockFile opens");
   check(terrain.OpenBlock("terraindata",FALSE),"Terrain block exists");
   check(terrain.GetBlockPtr()!=nullptr,"Original block mapping");
   terrain.CloseBlock(); terrain.Close();
  }
  check(FileSys::DeleteStream("mission"),"Release mission mappings and tracked stream");
  check(FileSys::SetActiveStream("base"),"Base stream survives mission unload");
  check(!FileSys::Exists("objects.cfg"),"Unloaded mission no longer resolves");
  std::cout<<"PASS original resource unload; gameplay simulation is not linked by this test\n";
 } catch(const std::exception &e) { std::cerr<<e.what()<<'\n'; return 1; }
}
