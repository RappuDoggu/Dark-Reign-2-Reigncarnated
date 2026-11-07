#pragma once
#include "reign/config_prelude.hpp"
#include <array>
#include <mutex>
using S16=std::int16_t;
constexpr U32 INFINITE=0xffffffffu;
namespace reign::event_detail {
// Pre/Post pairs must complete on the same thread. EventSys uses zero timeouts.
template<class DATA,int SIZE> class NativeSafeQueue {
 std::array<DATA,SIZE> data_{};unsigned head_=0,tail_=0,count_=0;mutable std::mutex mutex_;
public:
 DATA *AddPre(U32 timeout){if(timeout)throw std::runtime_error("Blocking event queue operation is not implemented");mutex_.lock();if(count_==SIZE){mutex_.unlock();return nullptr;}return &data_[head_];}
 void AddPost(){head_=(head_+1)%SIZE;++count_;mutex_.unlock();}
 DATA *RemovePre(U32 timeout){if(timeout)throw std::runtime_error("Blocking event queue operation is not implemented");mutex_.lock();if(!count_){mutex_.unlock();return nullptr;}return &data_[tail_];}
 void RemovePost(){tail_=(tail_+1)%SIZE;--count_;mutex_.unlock();}
 Bool Full()const{std::lock_guard<std::mutex> l(mutex_);return count_==SIZE;}
 Bool Empty()const{std::lock_guard<std::mutex> l(mutex_);return !count_;}
 void Flush(){std::lock_guard<std::mutex> l(mutex_);head_=tail_=count_=0;}
};
}
