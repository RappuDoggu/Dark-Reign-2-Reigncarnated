# Original interface theme initialization

Hash-checked external staging now compiles original default color initialization,
standard/custom color group creation and lookup, color configuration, group
inheritance, skin creation/configuration and lookup. Original standard colors and
Color::Set are staged from utiltypes.cpp. The compiler rejected MSVC's `sizeof
ColorGroup` type spelling; staging supplies parentheses. Long color literals use
explicit S32 values to select the original integer overload on LP64.

FrontendTheme owns the original SystemData structure, color tables and trees.
Its scoped, thread-local binding replaces the original global storage references
in the staged functions. Staged controls access IFace::FrontendData(), which
throws outside a bound session. No fallback global object pretends IFace has
started. Theme initialization sets alpha to 1, while back-buffer/draw-variable
startup remains a separate unimplemented obligation. Controls must be destroyed
before their theme. Configuration errors can leave partial original mutations;
discard the failed session rather than continue startup.

26 checks execute original defaults, system references, table bounds, named
colors, group inheritance, all-state skins, borders and independent/nested theme
bindings. Host ASan/UBSan: 40/40 passed. Cached executables again lost their
executable state (one zero-length); these were preserved externally and relinked
before the recorded final suite. Independent CI validation remains essential.

The complete eight-object frontend link, without GC, now has eight unresolved
symbols. This count is for these compiled controls, not the entire game:
Babel::Translate, Console::ProcessCmd, IFace::MsgBox, IFace::Sound::Play,
Input::eventId/lButton/mousePos, and deactivate_control_movie. Resolving them does
not establish Shell/Login initialization or an operational menu renderer.

Next inspected dependencies: original input registers INPUT with EventSys and
stores button/position state (interface/input.cpp); message-box creation must
create real controls; Babel::Translate delegates to the language database.
Do not substitute successful no-ops for these services. Assets/configuration,
live root creation, event dispatch and frame presentation must then be tested
together before an original main-menu APK can be offered for device testing.
