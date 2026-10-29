#include "reign/mission_catalog.hpp"
#include "reign/mission_fs.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include "reign/archive.hpp"
#include "missions.h"
#include "campaigns.h"
#include "difficulty.h"
#include <algorithm>
#include <cstring>
namespace fs=std::filesystem;
namespace {
thread_local const fs::path *root=nullptr;
bool owned=false;
std::string fold(std::string s){for(auto &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
fs::path resolve(const std::string &name){
 if(!root)throw std::runtime_error("Mission filesystem not bound");
 std::string normalized=name;std::replace(normalized.begin(),normalized.end(),'\\','/');fs::path relative(normalized);
 if(relative.empty()||relative.is_absolute()||normalized.find(':')!=std::string::npos)throw std::invalid_argument("Invalid mission path");
 fs::path result=*root;
 for(const auto &part:relative){if(part=="..")throw std::invalid_argument("Mission traversal rejected");if(part==".")continue;
  auto next=result/part;
  if(!fs::exists(next)&&fs::is_directory(result)){unsigned matches=0;for(const auto &entry:fs::directory_iterator(result))if(fold(entry.path().filename().string())==fold(part.string())){next=entry.path();++matches;}if(matches>1)throw std::runtime_error("Ambiguous case-insensitive mission path");}
  result=next;
  if(fs::exists(result)){auto canonical=fs::canonical(result);auto ri=root->begin(),ci=canonical.begin();for(;ri!=root->end();++ri,++ci)if(ci==canonical.end()||*ri!=*ci)throw std::runtime_error("Mission symlink escapes installation");result=canonical;}
 }
 return result;
}
struct Binding {const fs::path *previous;explicit Binding(const fs::path &p):previous(root){root=&p;}~Binding(){root=previous;}};
}
namespace reign::mission_fs {
void with_root(const fs::path &path,const std::function<void()> &call){auto canonical=fs::canonical(path);if(!fs::is_directory(canonical)||!call)throw std::invalid_argument("Invalid native filesystem binding");Binding binding(canonical);call();}
fs::path resolve_path(const std::string &name){return resolve(name);}
namespace File {bool Exists(const char *p){return fs::exists(resolve(p));}}
namespace Dir {
bool FindFirst(Find &out,const char *path,const char *pattern,unsigned flags){
 if(std::strcmp(pattern,"*")||flags&~File::Attrib::SUBDIR)throw std::invalid_argument("Unsupported mission enumeration filter");out={};auto directory=resolve(path);if(!fs::is_directory(directory))return false;
 for(const auto &entry:fs::directory_iterator(directory)){auto name=entry.path().filename().string();if(name.size()>=sizeof(Find::Entry::name))throw std::length_error("Mission filename exceeds original limit");auto isdir=entry.is_directory();if(flags&&!isdir)continue;if(!isdir&&!entry.is_regular_file())continue;Find::Entry e;e.attrib=isdir?File::Attrib::SUBDIR:0;std::memcpy(e.name,name.c_str(),name.size()+1);out.entries.push_back(e);}
 std::sort(out.entries.begin(),out.entries.end(),[](const auto &a,const auto &b){return fold(a.name)<fold(b.name);});return FindNext(out);
}
bool FindNext(Find &out){if(out.index>=out.entries.size())return false;out.finddata=out.entries[out.index++];return true;}
void FindClose(Find &out){out={};}
void PathMake(FilePath &out,const char *a,const char *b){std::string value=a?a:"";if(!value.empty()&&b&&*b&&value.back()!='\\')value+='\\';if(b)value+=b;if(value.size()>=270)throw std::length_error("Mission path exceeds original limit");out=value.c_str();}
void PathMake(FilePath &out,const char *drive,const char *path,const char *name,const char *ext){if(drive||ext)throw std::invalid_argument("Unsupported mission drive/extension");PathMake(out,path,name);}
}
}
namespace reign {
struct MissionCatalog::Impl {
 fs::path installation;ConfigLoader fallback;bool campaigns=false,missions=false,difficulty=false;
 std::vector<std::uint8_t> read(const std::string &name){auto p=resolve(name);if(fs::is_regular_file(p))return read_asset(installation,fs::relative(p,installation));if(name.find_first_of("/\\")==std::string::npos&&fallback)return fallback(name);throw FileSys::FileNotFound(name);}
 void close(){if(difficulty)Difficulty::Done();if(campaigns)Campaigns::Done();if(missions||Missions::Initialized())Missions::Done();difficulty=campaigns=missions=false;}
};
MissionCatalog::MissionCatalog(const fs::path &path,ConfigLoader fallback):impl_(std::make_unique<Impl>()){
 if(owned)throw std::runtime_error("Only one original mission catalog may be active");impl_->installation=fs::canonical(path);if(!fs::is_directory(impl_->installation))throw std::invalid_argument("Missing installation directory");impl_->fallback=std::move(fallback);owned=true;
 try{run([&]{Campaigns::Init();impl_->campaigns=true;Missions::Init();impl_->missions=true;Difficulty::Init();impl_->difficulty=true;});}catch(...){impl_->close();owned=false;throw;}
}
MissionCatalog::~MissionCatalog(){impl_->close();owned=false;}
void MissionCatalog::run(const std::function<void()> &call){if(!call)throw std::invalid_argument("Missing mission callback");Binding binding(impl_->installation);FileSys::Session files([&](const std::string &name){return impl_->read(name);});call();}
ConfigLoader MissionCatalog::mission_loader(const std::string &group,const std::string &mission)const {
 if(!Missions::FindMission(mission.c_str(),group.c_str()))throw std::invalid_argument("Unknown mission resource request");
 Binding binding(impl_->installation);auto directory=resolve(group+"\\"+mission);
 if(fs::is_directory(directory))return [directory](const std::string &name){Binding scope(directory);auto file=resolve(name);if(!fs::is_regular_file(file))throw FileSys::FileNotFound(name);return read_asset(directory,fs::relative(file,directory));};
 auto pack=resolve(group+"\\"+mission+".zwp");auto archive=std::make_shared<Archive>(pack);
 return [archive](const std::string &name){if(!archive->find(name))throw FileSys::FileNotFound(name);return archive->read(name);};
}

}
