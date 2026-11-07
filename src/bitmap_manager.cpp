#include "reign/bitmap_manager_prelude.hpp"
#include "reign/bitmap_loader.hpp"
#include <algorithm>
namespace reign::bitmap_manager_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Bitmap manager not bound");return *active;}
struct Binding {
 State *previous;explicit Binding(State &s,bool cleanup=false):previous(active){if(active&&!cleanup)throw std::runtime_error("Bitmap manager cannot reenter");active=&s;}
 ~Binding(){active=previous;}
};
namespace Utils {
char *FindExt(const char *s){auto *p=const_cast<char *>(s)+std::strlen(s);while(p!=s){if(*p=='.')return p;--p;}return nullptr;}
}
namespace Dir {
void PathExpand(const char *text,FileDrive &drive,FileDir &dir,FileName &file,FileExt &ext){
 std::string s=text;std::size_t start=s.size()>1&&s[1]==':'?2:0;drive=s.substr(0,start).c_str();
 auto slash=s.find_last_of("/\\");if(slash!=std::string::npos){dir=s.substr(start,slash+1-start).c_str();start=slash+1;}else dir="";
 const auto dot=s.find_last_of('.');const bool has=dot!=std::string::npos&&dot>=start;
 file=s.substr(start,has?dot-start:std::string::npos).c_str();ext=has?s.substr(dot).c_str():"";
}
}
namespace FileSys {Bool Exists(const char *name){return state().exists(name);}}
Bitmap::Bitmap(){if(state().tree.GetCount()>=4096)throw std::runtime_error("Bitmap manager entry limit exceeded");}
Bitmap::~Bitmap(){if(treeNode.InUse())state().tree.Unlink(this);ReleaseDD();}
void Bitmap::Read(const char *path){
 auto selected=std::string(path);auto bytes=state().loader(path);auto suffix=selected;std::transform(suffix.begin(),suffix.end(),suffix.begin(),[](unsigned char c){return std::tolower(c);});
 RgbaImage image;
 if(suffix.size()>=4&&suffix.substr(suffix.size()-4)==".tga")image=load_bitmap_tga(bytes);
 else if(suffix.size()>=4&&suffix.substr(suffix.size()-4)==".pic")image=load_bitmap_pic(bytes);
 else if(suffix.size()>=4&&suffix.substr(suffix.size()-4)==".bmp")image=load_bitmap_bmp(bytes);
 else throw std::runtime_error("Bitmap manager requires supported TGA/PIC/BMP data; movie/mipmap paths remain unported");
 unsigned texture=0;if(state().gpu.upload){texture=state().gpu.upload(image);if(!texture)throw std::runtime_error("Bitmap upload returned no texture");}
 ReleaseDD();record->source=std::move(selected);record->image=std::move(image);record->texture=texture;record->resident=true;++record->generation;
}
void Bitmap::ReleaseDD(){auto id=record->texture;record->texture=0;record->resident=false;if(id)state().gpu.destroy(id);}
void Bitmap::ReLoad(){if(!record->resident)Read(record->source.c_str());}
void Bitmap::Manager::ClearTextures(){if(state().gpu.unbind)state().gpu.unbind();}
void Bitmap::Manager::ResetData(){ClearTextures();}
}
namespace reign {
namespace {void filename(const std::string &s){if(s.empty()||s.size()>240||s.find('\0')!=std::string::npos)throw std::runtime_error("Bitmap name must contain 1..240 bytes without NUL");}}
struct BitmapManager::Impl {
 bitmap_manager_detail::State state;
 Impl(ConfigLoader loader,Exists exists,BitmapGpuCallbacks gpu){
  if(!loader||!exists)throw std::runtime_error("Bitmap loader and existence lookup required");
  const unsigned n=bool(gpu.upload)+bool(gpu.destroy)+bool(gpu.unbind);if(n&&n!=3)throw std::runtime_error("Bitmap GPU callbacks must all be present or absent");
  state.loader=std::move(loader);state.exists=std::move(exists);state.gpu=std::move(gpu);
 }
 ~Impl(){bitmap_manager_detail::Binding binding(state,true);bitmap_manager_detail::Bitmap::Manager::ClearTextures();state.tree.DisposeAll();}
};
BitmapManager::BitmapManager(ConfigLoader l,Exists e,BitmapGpuCallbacks g):impl_(std::make_unique<Impl>(std::move(l),std::move(e),std::move(g))){}
BitmapManager::~BitmapManager()=default;
std::shared_ptr<const ManagedBitmap> BitmapManager::load(const std::string &name){
 filename(name);auto &s=impl_->state;bitmap_manager_detail::Binding binding(s);
 try{auto *bmp=bitmap_manager_detail::Bitmap::Manager::FindCreate(0,name.c_str(),0,bitmap_manager_detail::bitmapTEXTURE,0,FALSE,FALSE);for(auto *created:s.pending)if(!created->record->resident)throw std::runtime_error("Bitmap disappeared before loading completed");s.pending.clear();return bmp?bmp->record:nullptr;}
 catch(...){for(auto i=s.pending.rbegin();i!=s.pending.rend();++i)s.tree.Dispose(*i);s.pending.clear();throw;}
}
std::shared_ptr<const ManagedBitmap> BitmapManager::find(const std::string &name)const{filename(name);bitmap_manager_detail::Binding binding(impl_->state);auto *bmp=bitmap_manager_detail::Bitmap::Manager::Find(name.c_str());return bmp?bmp->record:nullptr;}
std::vector<std::shared_ptr<const ManagedBitmap>> BitmapManager::animation(const std::string &name)const{
 filename(name);bitmap_manager_detail::Binding binding(impl_->state);std::vector<std::shared_ptr<const ManagedBitmap>> result;
 auto *first=bitmap_manager_detail::Bitmap::Manager::Find(name.c_str());if(!first)return result;auto *p=first;
 do{if(!p||result.size()>=impl_->state.tree.GetCount())throw std::runtime_error("Invalid original bitmap animation ring");result.push_back(p->record);p=p->next;}while(p!=first);return result;
}
unsigned BitmapManager::count()const{return impl_->state.tree.GetCount();}
void BitmapManager::release(){bitmap_manager_detail::Binding binding(impl_->state);bitmap_manager_detail::Bitmap::Manager::ReleaseDD();}
void BitmapManager::restore(){bitmap_manager_detail::Binding binding(impl_->state);bitmap_manager_detail::Bitmap::Manager::OnModeChange();}
void BitmapManager::abandon_context(){auto &s=impl_->state;bitmap_manager_detail::Binding binding(s);NBinTree<bitmap_manager_detail::Bitmap>::Iterator i(&s.tree);while(auto *p=i++){p->record->texture=0;p->record->resident=false;}}
void BitmapManager::clear(){auto &s=impl_->state;bitmap_manager_detail::Binding binding(s);bitmap_manager_detail::Bitmap::Manager::ClearTextures();s.tree.DisposeAll();}
}
