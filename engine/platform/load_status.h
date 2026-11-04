#pragma once
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <deque>
namespace NativeEngine {
// Independent of engine globals and EGL: Android can export this while loading
// is inside a long native call. Never dereference a mission handle on the UI thread.
class LoadStatus {
 using Clock=std::chrono::steady_clock;
 std::mutex mutex;
 Clock::time_point started=Clock::now(),updated=started;
 std::string stage="Waiting for native startup";
 std::deque<std::string> history;
 bool complete=false;
public:
 void Reset(){std::lock_guard<std::mutex> lock(mutex);started=updated=Clock::now();history.clear();stage="Native startup";complete=false;}
 void Update(std::string next){std::lock_guard<std::mutex> lock(mutex);if(next==stage)return;updated=Clock::now();stage=next.substr(0,512);std::ostringstream row;row<<std::chrono::duration_cast<std::chrono::milliseconds>(updated-started).count()<<" ms: "<<stage;history.push_back(row.str());if(history.size()>96)history.pop_front();}
 void Complete(){Update("Simulation running");std::lock_guard<std::mutex> lock(mutex);complete=true;}
 std::string Report(){std::lock_guard<std::mutex> lock(mutex);auto now=Clock::now();std::ostringstream out;out<<(complete?"READY":"LOADING")<<"\n"<<stage<<"\nElapsed "<<std::chrono::duration_cast<std::chrono::seconds>(now-started).count()<<" s; last stage change "<<std::chrono::duration_cast<std::chrono::seconds>(now-updated).count()<<" s ago\n";for(auto &row:history)out<<row<<'\n';return out.str();}
};
inline LoadStatus missionLoadStatus;
}
