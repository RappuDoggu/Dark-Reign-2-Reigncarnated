#pragma once
#include <memory>
#include <string>
#include <cstdint>
class FScope;
namespace reign {
class KeyNames {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 KeyNames();~KeyNames();
 KeyNames(const KeyNames&)=delete;KeyNames &operator=(const KeyNames&)=delete;
 void configure(FScope *);
 std::uint32_t scan_code(const std::string &)const;
 std::uint32_t modifier_flags(const std::string &)const;
 unsigned key_count()const;unsigned modifier_count()const;
};
}
