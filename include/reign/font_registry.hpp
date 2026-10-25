#pragma once
#include "reign/config.hpp"
#include "reign/native_font.hpp"
namespace reign {
class InterfaceMetrics;
// Original FontSys alias/file registration behind native font and file adapters.
// All calls on one registry must be serialized; loaders must not reenter it.
class FontRegistry {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 explicit FontRegistry(ConfigLoader);
 ~FontRegistry();
 FontRegistry(const FontRegistry &)=delete;FontRegistry &operator=(const FontRegistry &)=delete;
 void create(const std::string &alias,const std::string &file);
 // Executes CreateFont scopes, at top level or in ConfigureInterface,
 // through original FontSys::Create(FScope*).
 // Metrics are also executed when a metrics consumer is supplied; other scopes are ignored; this does not initialize or interpret full IFace.
 unsigned configure(const std::string &file,InterfaceMetrics *metrics=nullptr);
 std::shared_ptr<const NativeFont> find(const std::string &alias)const;
 std::shared_ptr<const NativeFont> find_crc(std::uint32_t alias)const;
 unsigned alias_count()const;unsigned file_count()const;
 // Re-read registered files through the original mode-change iteration.
 // Existing shared handles retain their prior immutable fonts; re-query aliases.
 void reload();
 void clear();
};
}
