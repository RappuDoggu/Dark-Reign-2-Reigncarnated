#pragma once
namespace NativeEngine {
// Call after VarSys::Init, before the original simulation callbacks.
void InitializeRuntimeVariables();
}

namespace GameGod { namespace Loader {
// Bind progress and type-check variables after VarSys startup.
void InitNativeVariables();
}}

namespace GameGod {
// Register original menu/save controls after IFace::Init; unregister before Done.
void InitNativeControls();
void DoneNativeControls();
}
