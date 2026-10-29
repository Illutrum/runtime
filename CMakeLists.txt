cmake_minimum_required(VERSION 3.17)
project(runtime LANGUAGES CXX)

# Root of this tree. CMAKE_SOURCE_DIR is the stack root when this file is
# added from the superbuild, so nested projects must not use it for thorvg,
# engine, or cmake/.
set(RUNTIME_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

include(FetchContent)

FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
)
FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
  FetchContent_Populate(doctest)
  add_library(doctest INTERFACE)
  add_library(doctest::doctest ALIAS doctest)
  target_include_directories(doctest INTERFACE ${doctest_SOURCE_DIR})
endif()

enable_testing()

if(NOT TARGET asset_manager)
  set(ASSETS_WITH_RENDER ON CACHE BOOL "" FORCE)
  FetchContent_Declare(illutrum_assets
    GIT_REPOSITORY https://github.com/Illutrum/assets.git
    GIT_TAG v0.1.0
    GIT_SHALLOW TRUE
  )
  FetchContent_GetProperties(illutrum_assets)
  if(NOT illutrum_assets_POPULATED)
    FetchContent_Populate(illutrum_assets)
  endif()
  # Those registrars assign a std::string inside TypedAsset::staticType before
  # that object is constructed. GameHost registers the same types itself.
  # Only the fetched copy is rewritten. The superbuild uses the in-tree assets.
  execute_process(
    COMMAND python3 ${RUNTIME_ROOT}/cmake/strip-asset-registrars.py ${illutrum_assets_SOURCE_DIR}
  )
  add_subdirectory(${illutrum_assets_SOURCE_DIR} ${illutrum_assets_BINARY_DIR})
elseif(NOT illutrum_assets_SOURCE_DIR)
  set(illutrum_assets_SOURCE_DIR ${CMAKE_SOURCE_DIR}/illutrum/assets)
endif()

# FetchContent records these in the assets directory scope. Later targets
# (ThorVG, the wasm host) read them from this directory.
if(NOT illutrum_render_SOURCE_DIR)
  FetchContent_GetProperties(illutrum_render SOURCE_DIR illutrum_render_SOURCE_DIR)
endif()
if(NOT rpc_cpp_SOURCE_DIR)
  FetchContent_GetProperties(rpc_cpp SOURCE_DIR rpc_cpp_SOURCE_DIR)
endif()

option(GAME_DEV_HOTSWAP "Enable native hot-reload (dev)" ON)
if(EMSCRIPTEN)
  set(GAME_DEV_HOTSWAP OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory(engine-runtime)
add_subdirectory(engine-runtime-thorvg)
add_subdirectory(engine)

if(GAME_DEV_HOTSWAP AND NOT EMSCRIPTEN)
  add_subdirectory(hotswap)
endif()

add_subdirectory(example-game)
add_subdirectory(example-visualization)
