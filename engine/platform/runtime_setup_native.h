#pragma once
namespace NativeEngine {
// Call after VarSys::Init, before the original simulation callbacks.
void InitializeRuntimeVariables();
}
