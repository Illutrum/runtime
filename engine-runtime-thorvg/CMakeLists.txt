# Isolated so CMAKE_CXX_FLAGS can drop reflection without affecting plugins / wasm exe.
# Keep c++26 (the public headers pull promise concepts); ThorVG itself is C++17.
if(EMSCRIPTEN AND FGFX_BACKEND STREQUAL "WebGPU")
  include(${CMAKE_SOURCE_DIR}/engine/cmake/EngineHelpers.cmake)
  ensure_emscripten_webgpu()
endif()

include(${CMAKE_SOURCE_DIR}/cmake/ThorVG.cmake)

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
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)
target_include_directories(engine_runtime_thorvg
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(engine_runtime_thorvg PUBLIC engine_runtime)
if(NOT EMSCRIPTEN)
  set_target_properties(engine_runtime_thorvg PROPERTIES
    OUTPUT_NAME engine_runtime_thorvg
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    BUILD_RPATH "$ORIGIN"
  )
  target_compile_definitions(engine_runtime_thorvg PRIVATE __USE_OPENGL __USE_GLFW)
  if(COMMAND apply_p2996_target)
    apply_p2996_target(engine_runtime_thorvg)
  endif()
  if(COMMAND apply_p2996_rpath_origin)
    apply_p2996_rpath_origin(engine_runtime_thorvg)
  endif()
  target_link_options(engine_runtime_thorvg PRIVATE "-Wl,-rpath,$ORIGIN" -Wl,--exclude-libs,ALL)
  target_link_libraries(engine_runtime_thorvg PRIVATE thorvg)
  add_dependencies(engine_runtime_thorvg engine_runtime)
else()
  target_link_libraries(engine_runtime_thorvg PUBLIC thorvg)
endif()

if(FGFX_BACKEND STREQUAL "WebGPU")
  target_compile_definitions(engine_runtime_thorvg PRIVATE FGFX_BACKEND_WEBGPU)
  if(NOT EMSCRIPTEN)
    target_link_libraries(engine_runtime_thorvg PRIVATE webgpu)
  else()
    target_link_libraries(engine_runtime_thorvg PUBLIC webgpu)
  endif()
endif()
