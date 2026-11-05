#pragma once
namespace reign {
// An activation is delivered after native event dispatch returns. No desktop
// graphics controls are activated when an Android owner accepts the request.
struct HostOptionsRequest {
 bool pending=false;
 bool take(){bool result=pending;pending=false;return result;}
};
inline thread_local HostOptionsRequest *hostOptionsOwner=nullptr;
struct HostOptionsBinding {
 HostOptionsRequest *previous;
 explicit HostOptionsBinding(HostOptionsRequest &owner):previous(hostOptionsOwner){hostOptionsOwner=&owner;}
 ~HostOptionsBinding(){hostOptionsOwner=previous;}
 HostOptionsBinding(const HostOptionsBinding &)=delete;
 HostOptionsBinding &operator=(const HostOptionsBinding &)=delete;
};
inline bool routeHostOptions(const char *name){
 if(!hostOptionsOwner||!name)return false;
 if(*name=='|')++name;
 const char *expected="sys::options";
 for(;*name&&*expected;++name,++expected){char c=*name;if(c>='A'&&c<='Z')c+=32;if(c!=*expected)return false;}
 if(*name||*expected)return false;
 hostOptionsOwner->pending=true;return true;
}
}
