cmake_minimum_required(VERSION 3.22)
project(ReigncarnatedEnginePort LANGUAGES CXX)
include(CTest)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(REIGN_UPSTREAM "" CACHE PATH "Separate pinned original source checkout")
option(REIGN_SANITIZE "Host runtime validation" OFF)
if(NOT REIGN_UPSTREAM)
 message(FATAL_ERROR "Set REIGN_UPSTREAM")
endif()
if(ANDROID AND NOT ANDROID_ABI STREQUAL "arm64-v8a")
 message(FATAL_ERROR "Engine port target currently validates ARM64 only")
endif()
if(REIGN_SANITIZE)
 if(ANDROID)
  message(FATAL_ERROR "Sanitizer runtime validation is currently host-only")
 endif()
 add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
 add_link_options(-fsanitize=address,undefined)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(REIGN_REPO "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(REIGN_ENGINE_ABI "${REIGN_UPSTREAM}-engine-abi")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${REIGN_REPO}/tools/prepare_engine_abi.py" "${REIGN_REPO}/cmake/engine-abi-source-hashes.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REIGN_REPO}/tools/prepare_engine_abi.py"
 "${REIGN_UPSTREAM}" "${REIGN_ENGINE_ABI}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
 message(FATAL_ERROR "Engine ABI staging failed")
endif()
# Separate build tree: do not mix original global classes with namespaced slices.
# No engine executable or playable mission is claimed by this foundation target.
add_library(dr2_engine_abi INTERFACE)
target_include_directories(dr2_engine_abi INTERFACE "${REIGN_ENGINE_ABI}")
target_compile_definitions(dr2_engine_abi INTERFACE DEVELOPMENT=1)
add_executable(engine_abi_smoke "${REIGN_REPO}/tests/engine_abi_smoke.cpp" "${REIGN_ENGINE_ABI}/random.cpp")
target_compile_options(engine_abi_smoke PRIVATE -include "${REIGN_ENGINE_ABI}/defines.h")
target_link_libraries(engine_abi_smoke PRIVATE dr2_engine_abi)
if(NOT ANDROID)
 add_test(NAME original_global_engine_abi COMMAND engine_abi_smoke)
endif()

add_library(dr2_engine_io STATIC
 "${REIGN_REPO}/engine/platform/file_posix.cpp"
 "${REIGN_ENGINE_ABI}/crc.cpp")
target_link_libraries(dr2_engine_io PUBLIC dr2_engine_abi)
target_include_directories(dr2_engine_io PUBLIC "${REIGN_REPO}/engine/platform")
target_compile_options(dr2_engine_io PRIVATE -include "${REIGN_REPO}/engine/platform/io_prelude.h")
add_executable(engine_io_test "${REIGN_REPO}/tests/engine_io_test.cpp")
target_link_libraries(engine_io_test PRIVATE dr2_engine_io)
if(NOT ANDROID)
 add_test(NAME original_engine_native_io COMMAND engine_io_test)
endif()

set(REIGN_ENGINE_RESOURCES "${REIGN_UPSTREAM}-engine-resources")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${REIGN_REPO}/tools/prepare_engine_resources.py" "${REIGN_REPO}/tools/prepare_config_slice.py"
 "${REIGN_REPO}/cmake/engine-resource-source-hashes.json" "${REIGN_REPO}/cmake/config-source-hashes.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REIGN_REPO}/tools/prepare_engine_resources.py"
 "${REIGN_UPSTREAM}" "${REIGN_ENGINE_RESOURCES}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
 message(FATAL_ERROR "Engine resource staging failed")
endif()
set(RESOURCE_SOURCES filesys filesrcdir filesrcpack filesrcstream dtrack str
 ptree fscope vnode stdparse blockfile tbuf)
set(RESOURCE_CPP)
foreach(name IN LISTS RESOURCE_SOURCES)
 list(APPEND RESOURCE_CPP "${REIGN_ENGINE_RESOURCES}/${name}.cpp")
endforeach()
add_library(dr2_engine_resources STATIC ${RESOURCE_CPP} platform/resource_utils.cpp)
target_include_directories(dr2_engine_resources BEFORE PUBLIC "${REIGN_ENGINE_RESOURCES}")
target_link_libraries(dr2_engine_resources PUBLIC dr2_engine_io)
target_compile_options(dr2_engine_resources PRIVATE -include "${REIGN_REPO}/engine/platform/resource_prelude.h")
set_source_files_properties("${REIGN_ENGINE_RESOURCES}/tbuf.cpp" PROPERTIES COMPILE_OPTIONS "-funsigned-char")
add_executable(engine_resources_test "${REIGN_REPO}/tests/engine_resources_test.cpp")
target_link_libraries(engine_resources_test PRIVATE dr2_engine_resources)
set(REIGN_GAME_DATA "" CACHE PATH "Optional owned installation for original resource integration test")
if(NOT ANDROID AND REIGN_GAME_DATA)
 add_test(NAME original_engine_mission_resources COMMAND engine_resources_test "${REIGN_GAME_DATA}")
endif()

set(REIGN_ENGINE_MISSIONS "${REIGN_UPSTREAM}-engine-missions")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${REIGN_REPO}/tools/prepare_engine_missions.py" "${REIGN_REPO}/cmake/engine-mission-source-hashes.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REIGN_REPO}/tools/prepare_engine_missions.py"
 "${REIGN_UPSTREAM}" "${REIGN_ENGINE_MISSIONS}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
 message(FATAL_ERROR "Engine mission staging failed")
endif()
add_library(dr2_engine_missions STATIC
 "${REIGN_ENGINE_MISSIONS}/missions.cpp" "${REIGN_ENGINE_MISSIONS}/campaigns.cpp"
 "${REIGN_ENGINE_MISSIONS}/difficulty.cpp" "${REIGN_ENGINE_MISSIONS}/stdload.cpp"
 "${REIGN_ENGINE_MISSIONS}/stats.cpp" "${REIGN_ENGINE_MISSIONS}/runcodes.cpp"
 "${REIGN_ENGINE_MISSIONS}/utiltypes.cpp"
 "${REIGN_ENGINE_ABI}/random.cpp")
target_include_directories(dr2_engine_missions BEFORE PUBLIC "${REIGN_ENGINE_MISSIONS}")
target_link_libraries(dr2_engine_missions PUBLIC dr2_engine_resources dr2_engine_type_idents)
target_compile_options(dr2_engine_missions PRIVATE
 -include "${REIGN_REPO}/engine/platform/mission_prelude.h" -ffunction-sections -fdata-sections)
add_executable(engine_mission_start_test "${REIGN_REPO}/tests/engine_mission_start_test.cpp")
target_link_libraries(engine_mission_start_test PRIVATE dr2_engine_missions)
# Saves, full game runtime and Win32 UI remain unlinked. Discard uncalled entry
# points rather than providing success-returning stubs for missing subsystems.
target_link_options(engine_mission_start_test PRIVATE -Wl,--gc-sections)
if(NOT ANDROID AND REIGN_GAME_DATA)
 add_test(NAME original_engine_mission_start COMMAND engine_mission_start_test "${REIGN_GAME_DATA}")
endif()

# Native runtime mechanisms needed by complete gameplay sources.
add_library(dr2_engine_type_idents STATIC platform/type_ident_registry.cpp)
target_link_libraries(dr2_engine_type_idents PUBLIC dr2_engine_resources)
target_include_directories(dr2_engine_type_idents PUBLIC "${REIGN_UPSTREAM}-engine-missions")
add_executable(engine_runtime_test "${REIGN_REPO}/tests/engine_runtime_test.cpp")
target_link_libraries(engine_runtime_test PRIVATE dr2_engine_type_idents)
if(NOT ANDROID)
 add_test(NAME original_engine_native_runtime COMMAND engine_runtime_test)
endif()

set(REIGN_ENGINE_CLOCK "${REIGN_UPSTREAM}-engine-clock")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${REIGN_REPO}/tools/prepare_engine_clock.py" "${REIGN_REPO}/cmake/engine-clock-source-hashes.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REIGN_REPO}/tools/prepare_engine_clock.py"
 "${REIGN_UPSTREAM}" "${REIGN_ENGINE_CLOCK}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
 message(FATAL_ERROR "Native clock staging failed")
endif()
add_library(dr2_engine_clock STATIC "${REIGN_ENGINE_CLOCK}/clock.cpp")
target_include_directories(dr2_engine_clock PUBLIC "${REIGN_ENGINE_CLOCK}")
target_link_libraries(dr2_engine_clock PUBLIC dr2_engine_resources)
target_compile_options(dr2_engine_clock PRIVATE -include "${REIGN_REPO}/engine/platform/resource_prelude.h")
target_link_libraries(engine_runtime_test PRIVATE dr2_engine_clock)
target_sources(dr2_engine_resources PRIVATE platform/numeric_native.cpp)
set_source_files_properties(platform/numeric_native.cpp PROPERTIES COMPILE_OPTIONS "-frounding-math")
target_sources(dr2_engine_resources PRIVATE platform/text_native.cpp)

set(REIGN_ENGINE_EVENTS "${REIGN_UPSTREAM}-engine-events")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${REIGN_REPO}/tools/prepare_engine_events.py" "${REIGN_REPO}/cmake/engine-event-source-hashes.json")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${REIGN_REPO}/tools/prepare_engine_events.py"
 "${REIGN_UPSTREAM}" "${REIGN_ENGINE_EVENTS}" RESULT_VARIABLE status)
if(NOT status EQUAL 0)
 message(FATAL_ERROR "Native event staging failed")
endif()
add_library(dr2_engine_input STATIC "${REIGN_ENGINE_EVENTS}/event.cpp" platform/input_native.cpp)
target_include_directories(dr2_engine_input BEFORE PUBLIC "${REIGN_ENGINE_EVENTS}" "${REIGN_ENGINE_MISSIONS}")
target_link_libraries(dr2_engine_input PUBLIC dr2_engine_resources)
target_compile_options(dr2_engine_input PRIVATE -include "${REIGN_REPO}/engine/platform/events_prelude.h")
add_executable(engine_input_test "${REIGN_REPO}/tests/engine_input_test.cpp")
target_link_libraries(engine_input_test PRIVATE dr2_engine_input)
if(NOT ANDROID)
 add_test(NAME original_engine_native_input COMMAND engine_input_test)
endif()
