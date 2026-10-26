#include "reign/profile_storage.hpp"
#include "reign/engine_file.hpp"
#include "reign/platform.hpp"
#include "user.h"
#include "ptree.h"
#include <algorithm>
#include <cstring>
namespace User {void AbortNative();}
namespace fs=std::filesystem;
namespace {
struct Context {fs::path root;reign::ProfileStorage::AddonHandler addons;};
thread_local Context *active=nullptr;bool owned=false;
Context &context(){if(!active)throw std::runtime_error("Private profile storage not bound");return *active;}
std::string relative_name(const char *name){if(!name)throw std::invalid_argument("Missing profile path");std::string s=name;std::replace(s.begin(),s.end(),'\\','/');if(s.rfind("users/",0)!=0&&s!="users")throw std::invalid_argument("Profile IO outside users directory");return s;}
std::string fold(std::string s){for(char &c:s)if(c>='A'&&c<='Z')c+=32;return s;}
fs::path path(const char *name){auto relative=relative_name(name);auto result=reign::mission_fs::resolve_path(relative);if(fold(result.lexically_relative(context().root).generic_string())!=fold(relative))throw std::runtime_error("Profile alias/symlink rejected");return result;}
struct Binding{Context *previous;Binding(Context &c):previous(active){active=&c;}~Binding(){active=previous;}};
}
namespace reign::profile_detail {
bool exists(const char *name){return fs::exists(path(name));}
bool make_directory(const char *name){auto p=path(name);return fs::is_directory(p)||fs::create_directories(p);}
bool remove_profile(const char *name){auto relative=relative_name(name);auto leaf=relative.substr(6);if(leaf.size()!=8||leaf.find_first_not_of("0123456789aAbBcCdDeEfF")!=std::string::npos)throw std::invalid_argument("Invalid profile directory deletion");auto p=path(name);if(!fs::is_directory(p))return false;return fs::remove_all(p)>0;}
void make_path(char *out,unsigned capacity,const char *a,const char *b){if(!out||!a||!b)throw std::invalid_argument("Missing profile path component");std::string s=std::string(a)+"\\"+b;if(s.size()>=capacity)throw std::length_error("Profile path exceeds original buffer");std::memcpy(out,s.c_str(),s.size()+1);}
bool write_tree(PTree &tree,const char *name){auto destination=path(name);auto temporary=destination;temporary+=".tmp-"+std::to_string(monotonic_ns());struct Cleanup{fs::path p;~Cleanup(){std::error_code e;fs::remove(p,e);}} cleanup{temporary};if(!tree.WriteTreeBinary(temporary.string().c_str()))throw std::runtime_error("Profile binary write failed");fs::rename(temporary,destination);return true;}
void addon(unsigned phase,std::uint32_t crc,void *team){if(!context().addons)throw std::runtime_error("Profile addon services not bound");context().addons(phase,crc,team);}
}
namespace reign {
struct ProfileStorage::Impl {Context context;DisabledAudioSettings &audio;bool closed=false;explicit Impl(DisabledAudioSettings &a):audio(a){}};
ProfileStorage::ProfileStorage(const fs::path &root,DisabledAudioSettings &audio,AddonHandler addons):impl_(std::make_unique<Impl>(audio)){
 if(owned)throw std::runtime_error("Only one original profile session may be active");fs::create_directories(root);impl_->context.root=fs::canonical(root);impl_->context.addons=std::move(addons);owned=true;
 try{run([]{User::Init();});}catch(...){User::AbortNative();owned=false;throw;}
}
ProfileStorage::~ProfileStorage(){if(!impl_->closed){User::AbortNative();owned=false;}}
void ProfileStorage::run(const std::function<void()> &call){if(impl_->closed||!call)throw std::invalid_argument("Closed profile storage or missing dispatch");mission_fs::with_root(impl_->context.root,[&]{Binding binding(impl_->context);FileSys::Session files([&](const std::string &name){auto file=path(name.c_str());if(!fs::is_regular_file(file))throw FileSys::FileNotFound(name);return read_asset(impl_->context.root,file.lexically_relative(impl_->context.root));});impl_->audio.run(call);});}
void ProfileStorage::close(){if(impl_->closed)return;run([]{User::Done();});impl_->closed=true;owned=false;}
}
