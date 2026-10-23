if(EMSCRIPTEN)
  return()
endif()

# The shared library contains render and assets. Their static archives are
# absorbed whole so a view .so can resolve those symbols from this one library.
# The anchor keeps the shared target legal. It has no symbols of its own.
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/engine_runtime_anchor.cpp "")
add_library(engine_runtime SHARED ${CMAKE_CURRENT_BINARY_DIR}/engine_runtime_anchor.cpp)
target_link_libraries(engine_runtime PRIVATE
  -Wl,--whole-archive
  render
  asset_manager
  asset_manager_render
  -Wl,--no-whole-archive
)
target_link_libraries(engine_runtime PUBLIC GL png z glfw GLEW dl)
target_compile_definitions(engine_runtime PUBLIC __USE_OPENGL)
target_compile_features(engine_runtime PUBLIC cxx_std_20)
foreach(dep render asset_manager asset_manager_render)
  target_include_directories(engine_runtime PUBLIC
    $<TARGET_PROPERTY:${dep},INTERFACE_INCLUDE_DIRECTORIES>
  )
endforeach()
if(COMMAND apply_p2996_target)
  apply_p2996_target(engine_runtime)
endif()
set_target_properties(engine_runtime PROPERTIES
  OUTPUT_NAME engine_runtime
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  BUILD_RPATH "$ORIGIN"
)
target_link_options(engine_runtime PRIVATE "-Wl,-rpath,$ORIGIN")
