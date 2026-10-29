#include "reign/frontend_bitmap.hpp"
#include "reign/campaign_selection.hpp"
#include "reign/engine_file.hpp"
#include "reign/bitmap_loader.hpp"
#include <cstring>
namespace {
thread_local const reign::CampaignSelectionServices *active=nullptr;
thread_local reign::ConfigLoader *resourceLoader=nullptr;
const reign::CampaignSelectionServices &services(){if(!active)throw std::runtime_error("Campaign services not bound");return *active;}
}
namespace reign {
void CampaignSelectionServices::run(const std::function<void()> &call)const{if(!call)throw std::invalid_argument("Missing campaign dispatch");auto *previous=active;active=this;struct Restore{const CampaignSelectionServices *p;~Restore(){active=p;}} restore{previous};call();}
namespace preview_detail {
struct Resources::Impl {ConfigLoader loader;ConfigLoader *previous;std::unique_ptr<FileSys::Session> files;explicit Impl(ConfigLoader fn):loader(std::move(fn)),previous(resourceLoader){if(!loader)throw std::invalid_argument("Missing mission resource loader");files=std::make_unique<FileSys::Session>(loader);resourceLoader=&loader;}~Impl(){resourceLoader=previous;}};
Resources::Resources(ConfigLoader loader):impl_(std::make_unique<Impl>(std::move(loader))){}Resources::~Resources()=default;
std::unique_ptr<Resources> open(const char *path,const char *mission){auto &s=services();if(!path||!mission||!*path||!*mission||!s.open_mission)throw std::runtime_error("Mission preview resources unavailable");return std::make_unique<Resources>(s.open_mission(path,mission));}
const char *default_side(){auto &s=services();if(!s.default_side)throw std::runtime_error("Default faction service not bound");thread_local std::string side;side=s.default_side();if(side.empty())throw std::runtime_error("Empty default faction");return side.c_str();}
void request_run_code(std::uint32_t crc){auto &s=services();if(!s.request_run_code)throw std::runtime_error("Mission run-code service not bound");s.request_run_code(crc);}
std::unique_ptr<Bitmap> load_texture(const char *name){if(!resourceLoader)throw std::runtime_error("Preview texture resources not bound");std::vector<std::uint8_t> bytes;try{bytes=(*resourceLoader)(name);}catch(const FileSys::FileNotFound &){return nullptr;}
 auto image=load_bitmap_bmp(bytes);auto surface=std::make_shared<BitmapSurface>([](const auto &){throw std::logic_error("Submit preview through UI draw consumer");});surface->Create(image.width,image.height,true);std::memcpy(surface->Lock(),image.pixels.data(),image.pixels.size());surface->UnLock();return std::make_unique<Bitmap>(surface);
}
}
}
