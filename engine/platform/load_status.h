#pragma once
#include <chrono>
#include <mutex>
#include <sstream>
#include <string>
#include <deque>
#include <filesystem>
#include <fstream>
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
 std::filesystem::path journal;
 std::size_t journalBytes=0;bool journalOk=false;
 void Persist(const std::string &line){if(journal.empty())return;bool truncate=journalBytes>98304;std::ofstream out(journal,truncate?std::ios::trunc:std::ios::app);if(truncate)journalBytes=0;out<<line<<'\n';out.flush();journalOk=bool(out);journalBytes+=line.size()+1;}
public:
 void Reset(){std::lock_guard<std::mutex> lock(mutex);started=updated=Clock::now();history.clear();stage="Native startup";complete=false;journal.clear();journalBytes=0;journalOk=false;}
 void ConfigureJournal(const std::filesystem::path &path){std::lock_guard<std::mutex> lock(mutex);journal=path;{std::ofstream out(journal,std::ios::trunc);out<<"Native mission loading journal\n";}journalBytes=0;Persist(stage);}
 void Update(std::string next){std::lock_guard<std::mutex> lock(mutex);if(next==stage)return;updated=Clock::now();stage=next.substr(0,512);std::ostringstream row;row<<std::chrono::duration_cast<std::chrono::milliseconds>(updated-started).count()<<" ms: "<<stage;history.push_back(row.str());if(history.size()>96)history.pop_front();Persist(row.str());}
 void Complete(){Update("Simulation running");std::lock_guard<std::mutex> lock(mutex);complete=true;}
 std::string Report(){std::lock_guard<std::mutex> lock(mutex);auto now=Clock::now();std::ostringstream out;out<<(complete?"READY":"LOADING")<<"\n"<<stage<<"\nElapsed "<<std::chrono::duration_cast<std::chrono::seconds>(now-started).count()<<" s; last stage change "<<std::chrono::duration_cast<std::chrono::seconds>(now-updated).count()<<" s ago\n";for(auto &row:history)out<<row<<'\n';return out.str();}
};
inline LoadStatus missionLoadStatus;
}
