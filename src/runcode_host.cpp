#include "reign/runcode_host.hpp"
#include "reign/engine_file.hpp"
namespace { thread_local std::function<void(FScope *)> *dispatch=nullptr; }
namespace reign {
struct RunCodeHost::Impl {
 FileSys::Session files;
 std::function<void(FScope *)> callback;
 std::function<void(FScope *)> *previous;
 Impl(ConfigLoader loader,std::function<void(FScope *)> fn):files(std::move(loader)),callback(std::move(fn)),previous(dispatch){
  if(!callback)throw std::runtime_error("Run-code configuration requires command dispatch");dispatch=&callback;
 }
 ~Impl(){dispatch=previous;}
};
RunCodeHost::RunCodeHost(ConfigLoader loader,std::function<void(FScope *)> fn):impl_(std::make_unique<Impl>(std::move(loader),std::move(fn))){}
RunCodeHost::~RunCodeHost()=default;
}
namespace Main {
void ScopeHandler(FScope *scope){
 if(!dispatch)throw std::runtime_error("Run-code command dispatch not configured");
 (*dispatch)(scope);
}
}
