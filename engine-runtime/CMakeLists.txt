if(EMSCRIPTEN)
  return()
endif()

set(ENGINE_RUNTIME_SOURCES
  ${FGFX_SOURCES}
  ${ASSET_CORE_SOURCES}
  ${ASSET_FGFX_SOURCES}
  ${ASSET_UI_SOURCES}
)

add_library(engine_runtime SHARED ${ENGINE_RUNTIME_SOURCES})

target_include_directories(engine_runtime PUBLIC
  ${CMAKE_SOURCE_DIR}/game-dev/fgfx/include
  ${CMAKE_SOURCE_DIR}/game-dev/fgfx/src
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/include
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx
  ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/ui
  ${CMAKE_SOURCE_DIR}/game-dev/glm
  ${CMAKE_SOURCE_DIR}/cpp-rpc/src
)

target_compile_definitions(engine_runtime PUBLIC __USE_OPENGL)

target_link_libraries(engine_runtime PUBLIC GL png z glfw GLEW dl)

apply_p2996_target(engine_runtime)
apply_p2996_rpath_origin(engine_runtime)

set_target_properties(engine_runtime PROPERTIES
  OUTPUT_NAME engine_runtime
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)

set(ASSET_MANAGER_ROOT ${CMAKE_SOURCE_DIR}/game-dev/asset-manager)
set(FGFX_ROOT ${CMAKE_SOURCE_DIR}/game-dev/fgfx)

include(${CMAKE_SOURCE_DIR}/game-dev/asset-manager/NativeTargets.cmake)
include(${CMAKE_SOURCE_DIR}/game-dev/fgfx/NativeTargets.cmake)
