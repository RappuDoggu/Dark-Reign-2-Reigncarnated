#include "localization_slice.hpp"
#include "reign/localization.hpp"
namespace reign::localization_detail {
namespace {thread_local State *active = nullptr;}
State &state() {if (!active) throw std::runtime_error("Language context not bound");return *active;}
struct Binding {
 State *previous;
 explicit Binding(State &s):previous(active){active=&s;}
 ~Binding(){active=previous;}
};
}
namespace reign {
struct Localization::Impl {localization_detail::State state;};
Localization::Localization(std::string language):impl_(std::make_unique<Impl>()) {
 if (language.empty() || language.size()>64) throw std::invalid_argument("Invalid language name");
 for (unsigned char c:language)
  if (!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))
   throw std::invalid_argument("Invalid language name");
 impl_->state.language=std::move(language);
}
Localization::~Localization()=default;
void Localization::run(const std::function<void()> &call) {
 if (!call) throw std::invalid_argument("Missing localization callback");
 localization_detail::Binding binding(impl_->state);call();
}
}
