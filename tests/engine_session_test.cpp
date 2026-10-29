#include "styxnet_data.h"
#include "session_native.h"
#include "multiplayer.h"
#include "multiplayer_data.h"
#include "stdload.h"
#include "multiplayer_network.h"
#include "runtime_setup_native.h"
#include "varsys.h"
#include "version.h"
#include "tracksys.h"
namespace Main {extern VarInteger elapCap,triCount;}
namespace {U32 received=0, sender=0;
void Receive(CRC from,CRC key,U32 size,const U8 *bytes){if(key!=123||size!=4||bytes[0]!=1)throw std::runtime_error("Invalid message");++received;sender=from;}
}
#include <cstring>
#include <iostream>
#include <stdexcept>
#define check(x) do{if(!(x))throw std::runtime_error("Session check failed at line "+std::to_string(__LINE__));}while(0)
int main(){try{
 TrackSys::Init();VarSys::Init();NativeEngine::InitializeRuntimeVariables();
 check(S32(Main::elapCap)==8 && S32(Main::triCount)==0);
 Main::elapCap=16;Main::triCount=250;check(S32(Main::elapCap)==16&&S32(Main::triCount)==250);
 bool duplicateVars=false;try{NativeEngine::InitializeRuntimeVariables();}catch(const std::logic_error &){duplicateVars=true;}check(duplicateVars);
 check(std::strstr(Version::GetBuildString(),"Reigncarnated native ARM64")!=nullptr);
 VarSys::DeleteItem("multiplayer");VarSys::DeleteItem("sys");VarSys::Done();TrackSys::Done();
 StyxNet::Data data;
 const U8 bytes[]={1,2,3,4};U32 size=0;const U8 *view=nullptr;
 check(data.IsEmpty());check(data.Store(10,20,sizeof(bytes),bytes,TRUE));
 check(!data.IsEmpty()&&data.Get(10,20,size,view)&&size==4&&!std::memcmp(view,bytes,4));
 check(!data.Store(10,20,1,bytes,FALSE));check(!data.Clear(10,20,FALSE));
 // Aliased updates must copy before releasing the old allocation.
 check(data.Store(10,20,3,view+1,TRUE));check(data.Get(10,20,size,view)&&size==3&&view[0]==2&&view[2]==4);
 FScope saved(nullptr,"Session");data.Save(&saved);saved.InitIterators();
 StyxNet::Data restored;restored.Load(&saved);
 check(restored.Get(10,20,size,view)&&size==3&&view[0]==2&&view[2]==4);
 check(!MultiPlayer::Data::HaveData());saved.InitIterators();MultiPlayer::Data::Load(&saved);
 check(MultiPlayer::Data::HaveData()&&MultiPlayer::Data::Get(10,20,size,view)&&size==3);
 MultiPlayer::Data::LocalClear(10,20);check(MultiPlayer::Data::HaveData()); // host protection
 MultiPlayer::Data::LocalFlush();check(!MultiPlayer::Data::HaveData());
 check(restored.Clear(10,20,TRUE)&&restored.IsEmpty());
 MultiPlayer::SetDataCrc(0xfeed1234);check(MultiPlayer::GetDataCrc()==0xfeed1234);
 MultiPlayer::SetDataCrc(0);check(!MultiPlayer::GetDataCrc());
 check(!MultiPlayer::IsHost()&&!MultiPlayer::Data::Online());
 NativeEngine::BeginOfflineSession("2crazed","Local Commander");
 check(MultiPlayer::IsHost()&&!MultiPlayer::Data::Online());
 check(NativeEngine::CurrentOfflineSession()->mission=="2crazed");
 check(MultiPlayer::Data::HaveData()&&MultiPlayer::Network::GetPlayers().GetCount()==1);
 check(!MultiPlayer::IsReady());MultiPlayer::SetReady();check(MultiPlayer::IsReady()&&!MultiPlayer::IsReady());
 MultiPlayer::SetReady();MultiPlayer::ClearReady();check(!MultiPlayer::IsReady());MultiPlayer::Process();
 U32 seq=11,interval=22,time=33;check(!MultiPlayer::Data::ProcessSyncData(seq,interval,time)&&seq==11&&interval==22&&time==33&&!MultiPlayer::Data::GetLag());
 MultiPlayer::Data::RegisterHandler(123,Receive);
 bool duplicateHandler=false;try{MultiPlayer::Data::RegisterHandler(123,Receive);}catch(const std::logic_error &){duplicateHandler=true;}check(duplicateHandler);
 MultiPlayer::Data::Send(123,sizeof(bytes),bytes,TRUE);check(received==1&&sender==MultiPlayer::UserName("Local Commander").crc);
 MultiPlayer::Data::UnRegisterHandler(123);
 bool unhandled=false;try{MultiPlayer::Data::Send(123,sizeof(bytes),bytes,TRUE);}catch(const std::runtime_error &){unhandled=true;}check(unhandled);
 bool longName=false;try{NativeEngine::BeginOfflineSession("2crazed","This name is too long");}catch(const std::invalid_argument &){longName=true;}check(longName&&NativeEngine::CurrentOfflineSession()->player=="Local Commander");
 FScope unsupportedSave(nullptr,"Save");bool saveRejected=false;try{MultiPlayer::Save(&unsupportedSave);}catch(const std::runtime_error &){saveRejected=true;}check(saveRejected);

 FScope empty(nullptr,"Local");MultiPlayer::Load(&empty);
 saved.InitIterators();MultiPlayer::Data::Load(&saved);check(MultiPlayer::Data::HaveData());
 saved.InitIterators();bool rejected=false;
 try{MultiPlayer::Load(&saved);}catch(const std::runtime_error &){rejected=true;}
 check(rejected&&MultiPlayer::Data::HaveData());
 NativeEngine::EndOfflineSession();check(!MultiPlayer::IsHost()&&!MultiPlayer::Data::HaveData());
 check(MultiPlayer::Network::GetPlayers().GetCount()==0);
 bool inactive=false;try{MultiPlayer::SetReady();}catch(const std::logic_error &){inactive=true;}check(inactive);
 // data intentionally remains populated: destructor must own its cleanup.
 std::cout<<"Original session data: host protection, aliased updates, serialization, local store and CRC round-trip pass; offline identity, readiness, local messages and runtime variables; no mission executed.\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
