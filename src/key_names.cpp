#include "key_names_slice.hpp"
#include "reign/key_names.hpp"
namespace reign::key_name_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Key names not bound");return *active;}
struct Binding {State *previous;explicit Binding(State &s):previous(active){active=&s;}~Binding(){active=previous;}};
}
namespace reign {
struct KeyNames::Impl{key_name_detail::State state;};
KeyNames::KeyNames():impl_(std::make_unique<Impl>()){}
KeyNames::~KeyNames()=default;
void KeyNames::configure(FScope *scope){
 if(!scope)throw std::invalid_argument("Missing key-name scope");key_name_detail::Binding binding(impl_->state);
 auto crc=scope->NameCrc();if(crc==Crc::CalcStr("Scancodes"))key_name_detail::ProcessScanCodes(scope);
 else if(crc==Crc::CalcStr("Modifiers"))key_name_detail::ProcessModifiers(scope);
 else throw std::runtime_error(std::string("Unsupported key configuration: ")+scope->NameStr());
}
std::uint32_t KeyNames::scan_code(const std::string &name)const{auto *item=impl_->state.keysByNameCrc.Find(Crc::CalcStr(name.c_str()));if(!item)throw std::runtime_error("Unknown key name");return item->scan;}
std::uint32_t KeyNames::modifier_flags(const std::string &name)const{auto *item=impl_->state.modifiers.Find(Crc::CalcStr(name.c_str()));if(!item)throw std::runtime_error("Unknown modifier name");return item->keyFlags;}
unsigned KeyNames::key_count()const{return impl_->state.keysByNameCrc.GetCount();}
unsigned KeyNames::modifier_count()const{return impl_->state.modifiers.GetCount();}
}
