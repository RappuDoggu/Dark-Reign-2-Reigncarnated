# Native menu pointer input and original event queue

FrontendInput registers the original INPUT event type in an existing EventLoop.
It supplies mouse position, left-button code and event id through thread-local,
scoped bindings used by the compiled controls. Move/button events use the native
queue's original input payload. Accepted events update stored pointer/button
state; queue-full returns false without changing that state, so the caller must
drain/retry instead of silently treating a dropped click as delivered.

152 checks pass, including signed coordinates, down/move/up payloads and button
state, 128-slot capacity, retry, nested independent loops with identical numeric
ids, exception restoration, and misuse rejection. The producer is serialized on
the menu thread; use one producer per shared loop. It does not install a handler
or claim that original controls have consumed these events. Touch/stylus mapping,
keyboard, double-click recognition, lifecycle cancellation and actual menu event
routing remain integration tasks. The new full host run passes 41/41 tests.

The complete control link now reports five unresolved services: Babel::Translate,
Console::ProcessCmd, IFace::MsgBox, IFace::Sound::Play, and movie deactivation.
This count covers the staged controls only, not the Shell scene/game engine.
