#pragma once
namespace reign {
// Call after VarSys initialization. Bind InterfaceEvents and a live root while
// dispatching these original activation/deactivation/toggle commands.
void register_interface_commands();
// Restricted standalone launch: no host/join arguments are accepted or stored.
// The original multiplayer.login path then only resets already-empty command
// line options. This does not implement multiplayer or automatically log in.
void register_standalone_launch_commands();
}
