#pragma once
namespace NativeEngine {
// Call after VarSys::Init, before the original simulation callbacks.
void InitializeRuntimeVariables();
}

namespace GameGod { namespace Loader {
// Bind progress and type-check variables after VarSys startup.
void InitNativeVariables();
}}
