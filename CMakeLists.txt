cmake_minimum_required(VERSION 3.17)
project(runtime LANGUAGES CXX)

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
execute_process(
  COMMAND python3 ${CMAKE_SOURCE_DIR}/cmake/strip-asset-registrars.py ${illutrum_assets_SOURCE_DIR}
)
add_subdirectory(${illutrum_assets_SOURCE_DIR} ${illutrum_assets_BINARY_DIR})

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
