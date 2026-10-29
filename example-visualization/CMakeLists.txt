cmake_minimum_required(VERSION 3.17)

project(example_visualization LANGUAGES CXX)

set(GAME_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
if(NOT RUNTIME_ROOT)
  set(RUNTIME_ROOT ${CMAKE_SOURCE_DIR})
endif()
set(ENGINE_ROOT ${RUNTIME_ROOT}/engine)

include(${ENGINE_ROOT}/cmake/EngineHelpers.cmake)
include(${RUNTIME_ROOT}/example-game/cmake/HotswapPlugins.cmake)

if(NOT EMSCRIPTEN)
  set(VIEW_SO ${CMAKE_BINARY_DIR}/examples/libviz.so)
  set(MANIFEST_PATH ${CMAKE_BINARY_DIR}/examples/visualization.manifest)

  if(GAME_DEV_HOTSWAP)
    add_library(example_visualization_view SHARED ${GAME_ROOT}/src/view/ExampleView.cpp)
    configure_hotswap_plugin(example_visualization_view)
    set_target_properties(example_visualization_view PROPERTIES OUTPUT_NAME viz)
    target_link_libraries(example_visualization_view PRIVATE engine_runtime engine_runtime_thorvg)
    add_dependencies(example_visualization_view engine_runtime engine_runtime_thorvg)

    add_custom_command(
      OUTPUT ${MANIFEST_PATH}
      COMMAND ${CMAKE_COMMAND}
        -DVIEW_SO=${VIEW_SO}
        -DMANIFEST_PATH=${MANIFEST_PATH}
        -P ${ENGINE_ROOT}/cmake/WriteManifest.cmake
      DEPENDS example_visualization_view
      COMMENT "Writing hotswap manifest for example_visualization"
    )
    add_custom_target(example_visualization_manifest DEPENDS ${MANIFEST_PATH})
  endif()

  add_executable(example_visualization ${GAME_ROOT}/src/exe/main.cpp)
  add_engine_to_game(example_visualization)
  target_link_libraries(example_visualization PRIVATE engine_runtime_thorvg)
  target_include_directories(example_visualization PRIVATE ${GAME_ROOT}/src/shared)
  target_compile_definitions(example_visualization PRIVATE
    GAME_BUILD_DIR="${CMAKE_BINARY_DIR}/examples"
    GAME_CMAKE_BINARY_DIR="${CMAKE_BINARY_DIR}"
    GAME_CMAKE_SOURCE_DIR="${RUNTIME_ROOT}"
    GAME_ROOT_DIR="${GAME_ROOT}"
  )
  if(GAME_DEV_HOTSWAP)
    add_dependencies(example_visualization example_visualization_view example_visualization_manifest engine_runtime engine_runtime_thorvg)
  else()
    add_dependencies(example_visualization engine_runtime engine_runtime_thorvg)
  endif()

  add_custom_command(TARGET example_visualization POST_BUILD
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

  set_target_properties(example_visualization PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
else()
  add_executable(example_visualization
    ${GAME_ROOT}/src/exe/main.cpp
    ${GAME_ROOT}/src/view/ExampleView.cpp
  )
  add_engine_to_game(example_visualization)
  target_link_libraries(example_visualization PRIVATE engine_runtime_thorvg)
  target_include_directories(example_visualization PRIVATE
    ${GAME_ROOT}/src/shared
    ${ENGINE_ROOT}/modulemap
  )
  target_compile_definitions(example_visualization PRIVATE
    GAME_BUILD_DIR=""
    GAME_CMAKE_BINARY_DIR=""
    GAME_CMAKE_SOURCE_DIR=""
    GAME_ROOT_DIR=""
  )
  set(VIZ_OUT_DIR ${CMAKE_BINARY_DIR}/examples/example_visualization)
  set_target_properties(example_visualization PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${VIZ_OUT_DIR}
  )
  fgfx_add_runtime_js()
  add_dependencies(example_visualization fgfx_runtime_js)
  if(FGFX_BACKEND STREQUAL "WebGPU")
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      "window.FGFX_BACKEND_WEBGPU = true;\n")
  else()
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      "window.FGFX_BACKEND_WEBGPU = false;\n")
  endif()
  add_custom_command(TARGET example_visualization POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      ${GAME_ROOT}/web/index.html
      ${VIZ_OUT_DIR}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy
      ${FGFX_RUNTIME_JS}
      ${VIZ_OUT_DIR}/runtime-js-compiled.js
    COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/backend.js
      ${VIZ_OUT_DIR}/backend.js
    COMMAND ${CMAKE_COMMAND} -E make_directory ${VIZ_OUT_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/logo.png
      ${VIZ_OUT_DIR}/assets/logo.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.png
      ${VIZ_OUT_DIR}/assets/ui.png
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${FGFX_ROOT}/examples/assets/ui.fnt
      ${VIZ_OUT_DIR}/assets/ui.fnt
  )
endif()
