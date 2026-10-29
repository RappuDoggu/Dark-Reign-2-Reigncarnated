#include "cursor_slice.hpp"
#include "reign/cursors.hpp"
namespace reign::cursor_detail {
thread_local State *active=nullptr;
State &state(){if(!active)throw std::runtime_error("Cursors not bound");return *active;}
struct Binding {State *previous;Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace reign {
struct Cursors::Impl {cursor_detail::State state;};
Cursors::Cursors():impl_(std::make_unique<Impl>()){}
Cursors::~Cursors()=default;
void Cursors::configure(FScope *s){if(!s)throw std::invalid_argument("Missing cursor scope");cursor_detail::Binding binding(impl_->state);if(s->NameCrc()==Crc::CalcStr("CreateCursor"))cursor_detail::ProcessCreateCursor(s);else if(s->NameCrc()==Crc::CalcStr("StandardCursors"))cursor_detail::ProcessStandardCursors(s);else throw std::runtime_error(std::string("Unported interface service: ")+s->NameStr());}
void Cursors::run(const std::function<void()> &call){
 if(!call)throw std::invalid_argument("Missing cursor callback");
 auto old=cursor_bridge::lookup;cursor_bridge::lookup=[this](unsigned id){if(id>=4)throw std::out_of_range("Standard cursor");auto value=impl_->state.standardCrs[id];return value?value:impl_->state.standardCrs[0];};
 struct Restore{std::function<unsigned(unsigned)> old;~Restore(){cursor_bridge::lookup=std::move(old);}} restore{std::move(old)};call();
}
unsigned Cursors::count()const{return impl_->state.cursors.GetCount();}
bool Cursors::select(const char *name){cursor_detail::Binding binding(impl_->state);return cursor_detail::Set(name?cursor_detail::FindByName(name):0);}
void Cursors::draw(int x,int y,unsigned ms){auto *c=impl_->state.current;if(c){c->Simulate(ms);c->Draw(Point<S32>(x,y));}}
}
