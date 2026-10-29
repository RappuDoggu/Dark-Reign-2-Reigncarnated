#pragma once
#include <functional>
#include <memory>
#include <string>
namespace reign {
// Binds an existing VarSys language database. Does not initialize or reset VarSys.
// Lookup views remain valid only while their VarSys binary item remains unchanged.
class Localization {
 struct Impl; std::unique_ptr<Impl> impl_;
public:
 explicit Localization(std::string language);
 ~Localization();
 Localization(const Localization &) = delete;
 Localization &operator=(const Localization &) = delete;
 void run(const std::function<void()> &);
};
}
