set(GAME_SHARED ${GAME_ROOT}/src/shared)
set(ENGINE_ROOT ${CMAKE_SOURCE_DIR}/game-dev/engine)
set(SHARED_MODULE_MAP ${GAME_SHARED}/module.modulemap)
set(ENGINE_MODULE_MAP ${ENGINE_ROOT}/modulemap/module.modulemap)

function(configure_hotswap_plugin TARGET)
  target_include_directories(${TARGET} PRIVATE
    ${GAME_SHARED}
    ${GAME_ROOT}/src/model
    ${GAME_ROOT}/src/view
    ${ENGINE_ROOT}/include
    ${CMAKE_SOURCE_DIR}/cpp-rpc/src
    ${CMAKE_SOURCE_DIR}/game-dev/glm
    ${CMAKE_SOURCE_DIR}/game-dev/fgfx/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx
    ${ENGINE_ROOT}/modulemap
  )
  target_compile_options(${TARGET} PRIVATE
    -fmodule-map-file=${SHARED_MODULE_MAP}
    -fmodule-map-file=${ENGINE_MODULE_MAP}
    -fmodules-cache-path=${CMAKE_BINARY_DIR}/clang-modules
  )
  apply_p2996_plugin_target(${TARGET})
  apply_p2996_rpath_origin(${TARGET})
  set_target_properties(${TARGET} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    PREFIX "lib"
  )
endfunction()
