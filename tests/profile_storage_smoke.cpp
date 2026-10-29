#include "reign/profile_storage.hpp"
#include "reign/platform.hpp"
#include "user.h"
#include "campaigns.h"
#include <iostream>
int main(){try{
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok)throw std::runtime_error("Profile check "+std::to_string(checks));};
 auto root=std::filesystem::temp_directory_path()/("reign-profile-"+std::to_string(reign::monotonic_ns()));
 std::filesystem::create_directory(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::filesystem::remove_all(p);}} cleanup{root};
 Campaigns::Init();struct CampaignCleanup{~CampaignCleanup(){Campaigns::Done();}} campaigns;
 reign::DisabledAudioSettings audio;
 {reign::ProfileStorage profiles(root,audio);profiles.run([&]{
 check(!User::LoggedIn());check(User::Create(FileIdent("Test Pilot")));check(User::LoggedIn());
 User::SampleStatCount("Logins");User::SampleStatCount("Logins");check(User::QueryStatCount("Logins").GetCount()==2);
 Campaigns::GetProgress().GetMission("fixture","first").RecordWin();check(User::Save());
 User::SampleStatStat("Time",2.5f);check(User::QueryStatStat("Time").GetLast()==2.5f);
 User::AppendAddon(GameIdent("fixture-addon"));bool missing=false;try{User::InitializeAddons();}catch(const std::exception &){missing=true;}check(missing);User::ClearAddons();
 bool mismatch=false;try{User::QueryStatStat("Logins");}catch(const std::exception &){mismatch=true;}check(mismatch);
 bool rejected=false;try{reign::profile_detail::remove_profile("users/../outside");}catch(const std::exception &){rejected=true;}check(rejected);
 });profiles.close();}
 {reign::ProfileStorage profiles(root,audio);profiles.run([&]{
 check(User::LoggedIn());check(std::string(User::GetName())=="Test Pilot");check(User::QueryStatCount("Logins").GetCount()==2);check(User::QueryStatStat("Time").GetLast()==2.5f);
 auto *record=Campaigns::GetProgress().FindMission("fixture","first");check(record&&record->MissionCompleted());
 check(User::Create(FileIdent("Second Pilot")));check(User::GetExistingUsers().GetCount()==2);
 check(User::Delete(FileIdent("Second Pilot")));check(User::Login(User::Find(FileIdent("Test Pilot"))));
 });profiles.close();}
 std::cout<<"Original profiles binary persistence, statistics, campaign progress and deletion: PASS ("<<checks<<" checks)\n";
 }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
