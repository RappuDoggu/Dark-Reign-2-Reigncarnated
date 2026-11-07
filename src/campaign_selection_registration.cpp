#include "game_campaignselection.h"
#include "icdroplist.h"
#include "reign/campaign_selection.hpp"
namespace {
IControl *campaign(U32,IControl *parent,U32){return new Game::CampaignSelection(parent);}
IControl *dropList(U32,IControl *parent,U32){return new ICDropList(parent);}
}
namespace reign {
void CampaignSelectionServices::register_classes(){IFace::RegisterControlClass("Game::CampaignSelection",campaign);IFace::RegisterControlClass("DropList",dropList);}
}
