#include "metrics_slice.hpp"
#include "reign/interface_metrics.hpp"
#include "reign/font_registry.hpp"
namespace reign::metric_detail {
namespace {thread_local State *active=nullptr;}
State &state(){if(!active)throw std::runtime_error("Interface metrics not bound");return *active;}
struct Binding {
 State *previous;
 explicit Binding(State &s):previous(active){active=&s;}
 ~Binding(){active=previous;}
};
}
namespace reign {
namespace {void dimensions(unsigned w,unsigned h){if(!w||!h||w>16384||h>16384)throw std::runtime_error("Invalid interface viewport dimensions");}}
struct InterfaceMetrics::Impl {metric_detail::State state;};
InterfaceMetrics::InterfaceMetrics(unsigned w,unsigned h):impl_(std::make_unique<Impl>()){
 dimensions(w,h);auto &s=impl_->state;s.width=w;s.height=h;metric_detail::Binding bind(s);metric_detail::InitMetrics();
}
InterfaceMetrics::~InterfaceMetrics()=default;
std::int32_t InterfaceMetrics::get(const std::string &name)const{
 if(name.empty()||name.find('\0')!=std::string::npos)throw std::runtime_error("Invalid interface metric name");
 auto &s=impl_->state;const auto crc=Crc::CalcStr(name.c_str());metric_detail::Binding bind(s);
 for(U32 i=0;i<metric_detail::MAX_CUSTOM_METRICS;++i)if(s.keys[i]==crc)return metric_detail::GetMetric(i);
 throw std::runtime_error("Unknown interface metric: "+name);
}
InterfaceViewport InterfaceMetrics::viewport()const{
 metric_detail::Binding bind(impl_->state);return {metric_detail::GetMetric(metric_detail::SCREEN_WIDTH),metric_detail::GetMetric(metric_detail::SCREEN_HEIGHT),metric_detail::GetMetric(metric_detail::SCREEN_DEPTH)};
}
void InterfaceMetrics::resize(unsigned w,unsigned h){dimensions(w,h);auto &s=impl_->state;s.width=w;s.height=h;metric_detail::Binding bind(s);metric_detail::UpdateScreenMetrics();}
void InterfaceMetrics::configure_native(void *scope){if(!scope)throw std::runtime_error("Metric configuration scope required");metric_detail::Binding bind(impl_->state);metric_detail::ConfigureMetrics(static_cast<FScope *>(scope));}
std::shared_ptr<const NativeFont> InterfaceMetrics::font(const std::string &metric,const FontRegistry &registry)const{
 // Other numeric metrics are not font references, even if their integer happens to match an alias CRC.
 if(Utils::Stricmp(metric.c_str(),"TitleFont")&&Utils::Stricmp(metric.c_str(),"ThinTitleFont"))throw std::runtime_error("Metric is not an original font reference");
 return registry.find_crc(static_cast<std::uint32_t>(get(metric)));
}
}
