# Isolated so CMAKE_CXX_FLAGS can drop reflection without affecting the view plugin / wasm exe.
# Keep c++26 (Texture.hpp pulls promise.hpp concepts); only ThorVG itself is compiled as C++17.
set(_thorvg_cxx_flags "${CMAKE_CXX_FLAGS}")
foreach(_flag -freflection -fparameter-reflection -fannotation-attributes -fexpansion-statements)
  string(REPLACE "${_flag}" "" _thorvg_cxx_flags "${_thorvg_cxx_flags}")
endforeach()
set(CMAKE_CXX_FLAGS "${_thorvg_cxx_flags}")

if(EMSCRIPTEN)
  add_library(viz_thorvg OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThorVGRaster.cpp
  )
else()
  add_library(viz_thorvg STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../ThorVGRaster.cpp
  )
endif()
set_target_properties(viz_thorvg PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(viz_thorvg PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_SOURCE_DIR}/game-dev/fgfx/src
  ${CMAKE_SOURCE_DIR}/game-dev/fgfx/include
  ${CMAKE_SOURCE_DIR}/game-dev/glm
  ${CMAKE_SOURCE_DIR}/cpp-rpc/src
)
if(NOT EMSCRIPTEN)
  target_compile_definitions(viz_thorvg PRIVATE __USE_OPENGL __USE_GLFW)
  apply_p2996_target(viz_thorvg)
endif()
target_link_libraries(viz_thorvg PUBLIC thorvg)
