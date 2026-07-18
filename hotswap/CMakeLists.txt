cmake_minimum_required(VERSION 3.17)

include(FetchContent)
FetchContent_Declare(
  efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
  GIT_TAG 1.4.1
)
FetchContent_GetProperties(efsw)
if(NOT efsw_POPULATED)
  FetchContent_Populate(efsw)
  add_subdirectory(${efsw_SOURCE_DIR} ${efsw_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(HOTSWAP_SOURCES
  src/Manifest.cpp
  src/ManifestWriter.cpp
  src/ManifestWatcher.cpp
  src/BuildOrchestrator.cpp
  src/AssetWatcher.cpp
  assets/ModelModuleAsset.cpp
  assets/ViewModuleAsset.cpp
)

add_library(hotswap STATIC ${HOTSWAP_SOURCES})
target_include_directories(hotswap PUBLIC
  include
  assets
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src
)
target_link_libraries(hotswap PUBLIC engine_runtime efsw-static dl)
target_compile_definitions(hotswap PUBLIC __HOTSWAP)
apply_p2996_target(hotswap)

add_executable(test_hotswap
  tests/test_hotswap.cpp
  tests/test_manifest.cpp
  tests/test_reload_decision.cpp
  tests/test_manifest_atomic.cpp
  tests/test_schema_hash.cpp
)
target_link_libraries(test_hotswap PRIVATE hotswap doctest::doctest dl)
apply_p2996_target(test_hotswap)
apply_p2996_rpath_origin(test_hotswap)
set_target_properties(test_hotswap PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
add_test(NAME HotswapTests COMMAND $<TARGET_FILE:test_hotswap>)
