#include "registry_slice.hpp"
#include "ptree.h"
namespace reign::registry_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Font registry not bound");return *active;}
struct Binding {
 State *previous;
 explicit Binding(State &s,bool cleanup=false):previous(active){if(active&&!cleanup)throw std::runtime_error("Font registry cannot reenter");active=&s;}
 ~Binding(){active=previous;}
};
Bool Font::Read(const char *file){
 const auto selected=file?std::string(file):path;
 auto replacement=std::make_shared<NativeFont>(state().loader(selected));
 path=selected;native=std::move(replacement);return TRUE;
}
}
namespace reign {
namespace {
void identifier(const std::string &s){if(s.empty()||s.size()>=64||s.find('\0')!=std::string::npos)throw std::runtime_error("Font registry identifier must contain 1..63 bytes without NUL");}
}
struct FontRegistry::Impl {
 registry_detail::State state;
 explicit Impl(ConfigLoader loader){if(!loader)throw std::runtime_error("Font registry loader required");state.loader=std::move(loader);registry_detail::Binding binding(state);registry_detail::Init();}
 ~Impl(){registry_detail::Binding binding(state,true);registry_detail::DeleteAll();registry_detail::Done();}
};
FontRegistry::FontRegistry(ConfigLoader loader):impl_(std::make_unique<Impl>(std::move(loader))){}
FontRegistry::~FontRegistry()=default;
void FontRegistry::create(const std::string &alias,const std::string &file){identifier(alias);identifier(file);registry_detail::Binding binding(impl_->state);registry_detail::Create(alias.c_str(),file.c_str());}
unsigned FontRegistry::configure(const std::string &file){
 registry_detail::Binding binding(impl_->state);FileSys::Session session(impl_->state.loader);PTree tree;
 if(!tree.AddFile(file.c_str()))throw FileSys::FileNotFound(tree.LastError());
 unsigned count=0;auto *root=tree.GetGlobalScope();root->InitIterators();
 auto create_scope=[&](FScope *scope){
  // Check identifiers before original fixed-size GameIdent storage can truncate them.
  scope->InitIterators();const std::string alias=scope->NextArgString();auto *child=scope->GetFunction("File");const std::string path=child->NextArgString();identifier(alias);identifier(path);
  scope->InitIterators();child->InitIterators();registry_detail::Create(scope);++count;
 };
 while(auto *scope=root->NextFunction()){
  if(scope->NameCrc()==Crc::CalcStr("CreateFont"))create_scope(scope);
  else if(scope->NameCrc()==Crc::CalcStr("ConfigureInterface")){
   scope->InitIterators();while(auto *child=scope->NextFunction())if(child->NameCrc()==Crc::CalcStr("CreateFont"))create_scope(child);
  }
 }
 return count;
}
std::shared_ptr<const NativeFont> FontRegistry::find(const std::string &alias)const{identifier(alias);registry_detail::Binding binding(impl_->state);auto *font=registry_detail::GetFont(Crc::CalcStr(alias.c_str()));return font?font->native:nullptr;}
unsigned FontRegistry::alias_count()const{return impl_->state.fontItems.GetCount();}
unsigned FontRegistry::file_count()const{return impl_->state.fontFiles.GetCount();}
void FontRegistry::reload(){registry_detail::Binding binding(impl_->state);registry_detail::OnModeChange();}
void FontRegistry::clear(){registry_detail::Binding binding(impl_->state);registry_detail::DeleteAll();}
}
