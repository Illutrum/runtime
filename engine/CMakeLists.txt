cmake_minimum_required(VERSION 3.17)

set(ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(ENGINE_INCLUDE ${ENGINE_ROOT}/include)
set(ENGINE_SRC ${ENGINE_ROOT}/src)

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
  )
  target_link_libraries(test_engine PRIVATE doctest::doctest engine_runtime)
  runtime_apply_p2996(test_engine)
  set_target_properties(test_engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    BUILD_RPATH "$ORIGIN"
  )
  add_test(NAME EngineTests COMMAND $<TARGET_FILE:test_engine>)
endif()
