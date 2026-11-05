#pragma once
namespace NativeEngine {
// Original Mission/Shell requests are delivered to Android after the engine
// frame returns. Never tear down the mission inside a script or simulation tick.
enum class MissionRoute {none, mission, menu};
inline MissionRoute &MissionRouteRequest(){static thread_local MissionRoute request=MissionRoute::none;return request;}
inline void RequestMissionRoute(MissionRoute request){MissionRouteRequest()=request;}
}
