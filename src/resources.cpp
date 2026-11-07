#include "reign/resources.hpp"
#include "reign/platform.hpp"
#include "reign/engine_file.hpp"
#include <algorithm>
#include <stdexcept>
namespace reign {
namespace {
std::string fold(std::string name) {
 for(char &c:name){if(c=='\\')c='/';if(c>='A'&&c<='Z')c+='a'-'A';}
 return name;
}
std::string relative_name(std::string name) {
 name=fold(std::move(name));
 if(name.empty()||name.find('\0')!=std::string::npos||name.find(':')!=std::string::npos||name[0]=='/')throw std::runtime_error("Invalid resource path");
 const std::filesystem::path p(name);
 for(const auto &part:p)if(part=="..")throw std::runtime_error("Resource parent traversal rejected");
 return p.lexically_normal().generic_string();
}
}
ResourceStore::ResourceStore(std::filesystem::path root):root_(std::filesystem::canonical(root)) {
 for(auto it=std::filesystem::recursive_directory_iterator(root_);it!=std::filesystem::recursive_directory_iterator();++it){
  const auto rel=it->path().lexically_relative(root_);
  if(fold(rel.begin()->string())=="users"){if(it->is_directory())it.disable_recursion_pending();continue;}
  if(it->is_regular_file()){
   auto key=fold(rel.generic_string());
   if(!paths_.emplace(key,rel).second)throw std::runtime_error("Ambiguous case-folded resource path");
  }
 }
}
std::filesystem::path ResourceStore::resolve(const std::string &name)const {
 const auto key=relative_name(name);auto it=paths_.find(key);
 if(it==paths_.end())throw FileSys::FileNotFound(name);
 const auto path=std::filesystem::canonical(root_/it->second);
 auto base=root_.begin(),p=path.begin();
 for(;base!=root_.end();++base,++p)if(p==path.end()||*base!=*p)throw std::runtime_error("Resource escapes root");
 return path;
}
bool ResourceStore::add_directory(const std::string &stream,const std::string &name,bool recursive){
 auto directory=relative_name(name);if(directory==".")directory.clear();std::vector<std::filesystem::path> directories;
 for(const auto &[key,rel]:paths_){
  auto parent=fold(rel.parent_path().generic_string());
  if(parent==directory || (recursive&&(directory.empty() || (parent.size()>directory.size()&&parent.compare(0,directory.size()+1,directory+"/")==0))))
   directories.push_back(rel.parent_path());
 }
 std::sort(directories.begin(),directories.end());directories.erase(std::unique(directories.begin(),directories.end()),directories.end());
 for(const auto &dir:directories){Directory src;
  for(const auto &[key,rel]:paths_)if(rel.parent_path()==dir){
   const auto crc=name_crc(rel.filename().string().c_str());
   if(!src.files.emplace(crc,rel).second)throw std::runtime_error("Ambiguous directory filename CRC");
  }
  streams_[fold(stream)].emplace_back(std::move(src));
 }
 return !directories.empty();
}
bool ResourceStore::add_archive(const std::string &stream,const std::string &relative){
 try{streams_[fold(stream)].emplace_back(std::make_shared<Archive>(resolve(relative)));return true;}
 catch(const FileSys::FileNotFound &){return false;}
}
void ResourceStore::add_stream(const std::string &stream,const std::string &target){streams_[fold(stream)].emplace_back(fold(target));}
std::vector<std::uint8_t> ResourceStore::search(const std::string &stream,const std::string &name,std::vector<std::string> &visiting)const{
 if(visiting.size()>=64||std::find(visiting.begin(),visiting.end(),stream)!=visiting.end())throw std::runtime_error("Resource stream cycle/depth limit");
 auto it=streams_.find(stream);if(it==streams_.end())throw FileSys::FileNotFound(name);
 visiting.push_back(stream);
 struct Pop{std::vector<std::string>&v;~Pop(){v.pop_back();}}pop{visiting};
 for(const auto &source:it->second){
  if(const auto *directory=std::get_if<Directory>(&source)){
   auto p=directory->files.find(name_crc(name.c_str()));if(p!=directory->files.end())return read_asset(root_,p->second);
  }else if(const auto *pack=std::get_if<std::shared_ptr<Archive>>(&source)){
   if((*pack)->find(name))return (*pack)->read(name);
  }else{
   try{return search(std::get<std::string>(source),name,visiting);}catch(const FileSys::FileNotFound &){}
  }
 }
 throw FileSys::FileNotFound(name);
}
std::vector<std::uint8_t> ResourceStore::load(const std::string &stream,const std::string &name)const{
 const auto normalized=relative_name(name);
 if(normalized.find('/')!=std::string::npos)return read_asset(root_,resolve(normalized).lexically_relative(root_));
 std::vector<std::string> visiting;return search(fold(stream),normalized,visiting);
}
}
