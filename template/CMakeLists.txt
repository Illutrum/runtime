cmake_minimum_required(VERSION 3.17)
project(template_game LANGUAGES CXX)

# Copy this directory, then add_subdirectory on the copy. Do not add it beside
# example-game: both declare targets named model and view.

set(GAME_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_ROOT ${CMAKE_SOURCE_DIR}/engine)

include(${ENGINE_ROOT}/cmake/EngineHelpers.cmake)
include(${CMAKE_SOURCE_DIR}/example-game/cmake/HotswapPlugins.cmake)

if(NOT EMSCRIPTEN)
  add_library(model SHARED ${GAME_ROOT}/src/model/StubModel.cpp)
  add_library(view SHARED ${GAME_ROOT}/src/view/StubView.cpp)
  configure_hotswap_plugin(model)
  configure_hotswap_plugin(view)
  target_link_libraries(view PRIVATE engine_runtime)
  add_dependencies(view model engine_runtime)

  add_executable(template_game ${GAME_ROOT}/src/exe/main.cpp)
  add_engine_to_game(template_game)
  target_compile_definitions(template_game PRIVATE
    GAME_BUILD_DIR="${CMAKE_BINARY_DIR}/examples"
    GAME_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
    GAME_CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GAME_ROOT_DIR="${GAME_ROOT}"
  )
  add_dependencies(template_game model view engine_runtime)
  set_target_properties(template_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
endif()
