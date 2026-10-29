#include "styxnet_data.h"
#include "multiplayer.h"
#include "multiplayer_data.h"
#include "stdload.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#define check(x) do{if(!(x))throw std::runtime_error("Session check failed at line "+std::to_string(__LINE__));}while(0)
int main(){try{
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
 // data intentionally remains populated: destructor must own its cleanup.
 std::cout<<"Original session data: host protection, aliased updates, serialization, local store and CRC round-trip pass; no network/session bootstrap.\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
