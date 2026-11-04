#pragma once
#include <string>
namespace NativeEngine {
enum class AudioEvent {requested,started,missing,decode_failed,suppressed,repeat};
void NoteAudio(const char *resource,AudioEvent);
std::string DigitalDiagnostics();
}
