cmake_minimum_required(VERSION 3.17)

set(ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_INCLUDE ${ENGINE_ROOT}/include)
set(ENGINE_SRC ${ENGINE_ROOT}/src)
set(FGFX_ROOT ${CMAKE_SOURCE_DIR}/game-dev/fgfx)

set(ENGINE_RUNTIME_SOURCES
  ${ENGINE_SRC}/GameHost.cpp
  ${ENGINE_SRC}/InputCollector.cpp
  ${ENGINE_SRC}/HostLoadingUI.cpp
)

include(${ENGINE_ROOT}/cmake/EngineHelpers.cmake)

if(NOT EMSCRIPTEN)
  add_executable(test_engine
    tests/test_engine.cpp
    tests/test_host_lifecycle.cpp
    tests/test_input_collect.cpp
    src/InputCollector.cpp
  )
  target_include_directories(test_engine PRIVATE
    ${ENGINE_INCLUDE}
    ${ENGINE_SRC}
    ${FGFX_ROOT}/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/ui
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx
    ${CMAKE_SOURCE_DIR}/game-dev/glm
    ${CMAKE_SOURCE_DIR}/cpp-rpc/src
    ${CMAKE_SOURCE_DIR}/game-dev/hotswap/include
    ${CMAKE_SOURCE_DIR}/game-dev/hotswap/assets
  )
  target_link_libraries(test_engine PRIVATE doctest::doctest engine_runtime)
  apply_p2996_target(test_engine)
  apply_p2996_rpath_origin(test_engine)
  set_target_properties(test_engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
  add_test(NAME EngineTests COMMAND $<TARGET_FILE:test_engine>)
endif()
