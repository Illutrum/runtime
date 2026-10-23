cmake_minimum_required(VERSION 3.17)

project(example_game LANGUAGES CXX)

set(GAME_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_ROOT ${CMAKE_SOURCE_DIR}/engine)

include(${ENGINE_ROOT}/cmake/EngineHelpers.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/HotswapPlugins.cmake)

if(NOT EMSCRIPTEN)
  set(MODEL_SO ${CMAKE_BINARY_DIR}/examples/libmodel.so)
  set(VIEW_SO ${CMAKE_BINARY_DIR}/examples/libview.so)
  set(MANIFEST_PATH ${CMAKE_BINARY_DIR}/examples/hotswap.manifest)

  if(GAME_DEV_HOTSWAP)
    add_library(model SHARED ${GAME_ROOT}/src/model/ExampleModel.cpp)
    add_library(view SHARED ${GAME_ROOT}/src/view/ExampleView.cpp)
    configure_hotswap_plugin(model)
    configure_hotswap_plugin(view)
    target_link_libraries(view PRIVATE engine_runtime)
    add_dependencies(view model engine_runtime)

    add_custom_command(
      OUTPUT ${MANIFEST_PATH}
      COMMAND ${CMAKE_COMMAND}
        -DMODEL_SO=${MODEL_SO}
        -DVIEW_SO=${VIEW_SO}
        -DMANIFEST_PATH=${MANIFEST_PATH}
        -P ${ENGINE_ROOT}/cmake/WriteManifest.cmake
      DEPENDS model view
      COMMENT "Writing hotswap manifest for example_game"
    )
    add_custom_target(example_game_manifest DEPENDS ${MANIFEST_PATH})
  endif()

  add_executable(example_game ${GAME_ROOT}/src/exe/main.cpp)
  add_engine_to_game(example_game)
  target_include_directories(example_game PRIVATE ${GAME_ROOT}/src/shared)
  target_compile_definitions(example_game PRIVATE
    GAME_BUILD_DIR="${CMAKE_BINARY_DIR}/examples"
    GAME_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
    GAME_CMAKE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    GAME_ROOT_DIR="${GAME_ROOT}"
  )
  if(GAME_DEV_HOTSWAP)
    add_dependencies(example_game model view example_game_manifest engine_runtime)
  else()
    add_dependencies(example_game engine_runtime)
  endif()

  add_custom_command(TARGET example_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/examples/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/logo.png
      ${CMAKE_BINARY_DIR}/examples/assets/logo.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.png
      ${CMAKE_BINARY_DIR}/examples/assets/ui.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.fnt
      ${CMAKE_BINARY_DIR}/examples/assets/ui.fnt
  )

  set_target_properties(example_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )

  if(GAME_DEV_HOTSWAP)
    add_executable(test_example_game_plugins
      ${GAME_ROOT}/tests/test_model_view_dlopen.cpp
    )
    target_include_directories(test_example_game_plugins PRIVATE
      ${GAME_ROOT}/src/shared
    )
    target_link_libraries(test_example_game_plugins PRIVATE
      doctest::doctest
      engine_runtime
      dl
    )
    runtime_apply_p2996(test_example_game_plugins)
    target_compile_definitions(test_example_game_plugins PRIVATE
      MODEL_SO_PATH="${CMAKE_BINARY_DIR}/examples/libmodel.so"
      VIEW_SO_PATH="${CMAKE_BINARY_DIR}/examples/libview.so"
      MANIFEST_PATH="${CMAKE_BINARY_DIR}/examples/hotswap.manifest"
    )
    add_dependencies(test_example_game_plugins model view engine_runtime)
    set_target_properties(test_example_game_plugins PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    )
    add_test(NAME ExampleGamePluginTests COMMAND $<TARGET_FILE:test_example_game_plugins>)
  endif()
else()
  add_executable(example_game
    ${GAME_ROOT}/src/exe/main.cpp
    ${GAME_ROOT}/src/model/ExampleModel.cpp
    ${GAME_ROOT}/src/view/ExampleView.cpp
  )
  add_engine_to_game(example_game)
  target_include_directories(example_game PRIVATE
    ${GAME_ROOT}/src/shared
    ${ENGINE_ROOT}/modulemap
  )
  target_compile_definitions(example_game PRIVATE
    GAME_BUILD_DIR=""
    GAME_CMAKE_BINARY_DIR=""
    GAME_CMAKE_SOURCE_DIR=""
    GAME_ROOT_DIR=""
  )
  set(GAME_OUT_DIR ${CMAKE_BINARY_DIR}/examples/example_game)
  set_target_properties(example_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${GAME_OUT_DIR}
  )
  if(FGFX_BACKEND STREQUAL "WebGPU")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      "window.FGFX_BACKEND_WEBGPU = true;\n")
  else()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      "window.FGFX_BACKEND_WEBGPU = false;\n")
  endif()
  add_custom_command(TARGET example_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${GAME_ROOT}/web/index.html
      ${GAME_OUT_DIR}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy
      ${FGFX_ROOT}/runtime-js-compiled.js
      ${GAME_OUT_DIR}/runtime-js-compiled.js
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      ${GAME_OUT_DIR}/backend.js
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GAME_OUT_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/logo.png
      ${GAME_OUT_DIR}/assets/logo.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.png
      ${GAME_OUT_DIR}/assets/ui.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.fnt
      ${GAME_OUT_DIR}/assets/ui.fnt
  )
endif()
