#include "reign/mission_catalog.hpp"
#include "reign/archive.hpp"
#include "missions.h"
#include "campaigns.h"
#include "difficulty.h"
#include <iostream>
int main(int argc,char **argv){try{
 if(argc!=2)throw std::invalid_argument("Usage: campaign_catalog_probe installation-directory");
 reign::Archive archive(std::filesystem::path(argv[1])/"packs/base.zwp");
 reign::MissionCatalog catalog(argv[1],[&](const std::string &name){return archive.read(name);});
 unsigned missions=0;for(NList<Missions::Group>::Iterator i(&Missions::GetGroups());*i;++i)missions+=(*i)->GetMissions().GetCount();
 std::cout<<"Original read-only mission catalog: PASS\nGroups: "<<Missions::GetGroups().GetCount()<<"\nMissions: "<<missions<<"\nCampaigns: "<<Campaigns::GetCampaigns().GetCount()<<"\nDifficulty settings: "<<Difficulty::GetSettings().GetCount()<<"\nDefault difficulty: "<<Difficulty::GetDefaultSetting().GetName().str<<"\nMission execution, menu runtime and Android APK: NOT tested\n";
}catch(const std::exception &e){std::cerr<<"CATALOG_FAIL "<<e.what()<<'\n';return 1;}}
