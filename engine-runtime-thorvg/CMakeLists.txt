# Isolated so CMAKE_CXX_FLAGS can drop reflection without affecting plugins / wasm exe.
# Keep c++26 (Texture.hpp pulls promise.hpp concepts); only ThorVG itself is compiled as C++17.
include(${CMAKE_SOURCE_DIR}/game-dev/cmake/ThorVG.cmake)

set(_thorvg_cxx_flags "${CMAKE_CXX_FLAGS}")
foreach(_flag -freflection -fparameter-reflection -fannotation-attributes -fexpansion-statements)
  string(REPLACE "${_flag}" "" _thorvg_cxx_flags "${_thorvg_cxx_flags}")
endforeach()
set(CMAKE_CXX_FLAGS "${_thorvg_cxx_flags}")

if(EMSCRIPTEN)
  add_library(engine_runtime_thorvg OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thorvg.cpp
  )
else()
  add_library(engine_runtime_thorvg SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/thorvg.cpp
  )
endif()
set_target_properties(engine_runtime_thorvg PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(engine_runtime_thorvg
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_SOURCE_DIR}/game-dev/fgfx/src
    ${CMAKE_SOURCE_DIR}/game-dev/fgfx/include
    ${CMAKE_SOURCE_DIR}/game-dev/glm
    ${CMAKE_SOURCE_DIR}/cpp-rpc/src
)
if(NOT EMSCRIPTEN)
  set_target_properties(engine_runtime_thorvg PROPERTIES
    OUTPUT_NAME engine_runtime_thorvg
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
  target_compile_definitions(engine_runtime_thorvg PRIVATE __USE_OPENGL __USE_GLFW)
  apply_p2996_target(engine_runtime_thorvg)
  apply_p2996_rpath_origin(engine_runtime_thorvg)
  target_link_options(engine_runtime_thorvg PRIVATE -Wl,--exclude-libs,ALL)
  target_link_libraries(engine_runtime_thorvg PRIVATE engine_runtime thorvg)
  add_dependencies(engine_runtime_thorvg engine_runtime)
else()
  target_link_libraries(engine_runtime_thorvg PUBLIC thorvg)
endif()
