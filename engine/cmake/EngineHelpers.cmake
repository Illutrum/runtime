set(FGFX_ROOT ${CMAKE_SOURCE_DIR}/game-dev/fgfx)
set(ENGINE_ROOT ${CMAKE_SOURCE_DIR}/game-dev/engine)
set(ENGINE_INCLUDE ${ENGINE_ROOT}/include)

function(apply_emscripten_webgl2_target TARGET)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  target_link_options(${TARGET} PRIVATE
    -sUSE_WEBGL2=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,allocate,intArrayFromString"
    "-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_fgfx_render,_fgfx_io_fileLoaded,_fgfx_io_fileFailed,_fgfx_getEmptySprite"
    "--post-js=${FGFX_ROOT}/js/emscripten-glue.js"
  )
endfunction()

function(add_engine_to_game TARGET)
  set(options "")
  set(oneValueArgs GAME_DIR)
  set(multiValueArgs "")
  cmake_parse_arguments(AEG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  target_sources(${TARGET} PRIVATE
    ${ENGINE_ROOT}/src/GameHost.cpp
    ${ENGINE_ROOT}/src/InputCollector.cpp
    ${ENGINE_ROOT}/src/HostLoadingUI.cpp
  )
  if(NOT EMSCRIPTEN)
    target_sources(${TARGET} PRIVATE ${ENGINE_ROOT}/src/GlfwInputBinding.cpp)
  endif()

  target_include_directories(${TARGET} PRIVATE
    ${ENGINE_INCLUDE}
    ${ENGINE_ROOT}/src
    ${FGFX_ROOT}/include
    ${FGFX_ROOT}/src
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/ui
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx
    ${CMAKE_SOURCE_DIR}/game-dev/hotswap/include
    ${CMAKE_SOURCE_DIR}/game-dev/hotswap/assets
    ${CMAKE_SOURCE_DIR}/game-dev/glm
    ${CMAKE_SOURCE_DIR}/cpp-rpc/src
  )

  if(NOT EMSCRIPTEN)
    target_compile_definitions(${TARGET} PRIVATE __USE_OPENGL __USE_GLFW)
    target_link_libraries(${TARGET} PRIVATE engine_runtime)
    apply_p2996_target(${TARGET})
    apply_p2996_rpath_origin(${TARGET})
    if(GAME_DEV_HOTSWAP)
      target_compile_definitions(${TARGET} PRIVATE __HOTSWAP)
      target_link_libraries(${TARGET} PRIVATE hotswap dl)
    endif()
  else()
    target_sources(${TARGET} PRIVATE
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src/Asset.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src/AssetTypeRegistry.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src/AssetRequest.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src/AssetsManager.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/src/LoadTask.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/TextureAsset.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/MSDFFontAsset.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx/JsonAsset.cpp
      ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/ui/LoadingScreen.cpp
    )
    aux_source_directory(${FGFX_ROOT}/src FGFX_ENGINE_SOURCES)
    list(APPEND FGFX_ENGINE_SOURCES ${FGFX_ROOT}/src/io/FileLoader.cpp)
    target_sources(${TARGET} PRIVATE ${FGFX_ENGINE_SOURCES})
    apply_emscripten_webgl2_target(${TARGET})
  endif()
endfunction()

function(hotswap_plugin TARGET)
  set(options "")
  set(oneValueArgs MODULE_MAP HEADERS_PATH)
  set(multiValueArgs SOURCES INCLUDE_DIRS)
  cmake_parse_arguments(HP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  add_library(${TARGET} SHARED ${HP_SOURCES})
  target_include_directories(${TARGET} PRIVATE
    ${HP_INCLUDE_DIRS}
    ${ENGINE_INCLUDE}
    ${CMAKE_SOURCE_DIR}/cpp-rpc/src
    ${CMAKE_SOURCE_DIR}/game-dev/glm
    ${FGFX_ROOT}/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/include
    ${CMAKE_SOURCE_DIR}/game-dev/asset-manager/fgfx
  )

  if(HP_HEADERS_PATH)
    target_include_directories(${TARGET} PRIVATE ${HP_HEADERS_PATH})
  endif()

  target_compile_options(${TARGET} PRIVATE
    -fmodules
    -fcxx-modules
    -fmodules-cache-path=${CMAKE_BINARY_DIR}/clang-modules
    -fmodule-map-file=${HP_MODULE_MAP}
    -fno-autolink
  )

  if(APPLE)
    set_target_properties(${TARGET} PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
    )
  endif()

  set_target_properties(${TARGET} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
  )
endfunction()

function(hotswap_write_manifest MODEL_SO VIEW_SO MANIFEST_PATH)
  add_custom_command(
    OUTPUT ${MANIFEST_PATH}
    COMMAND ${CMAKE_COMMAND}
      -DMODEL_SO=${MODEL_SO}
      -DVIEW_SO=${VIEW_SO}
      -DMANIFEST_PATH=${MANIFEST_PATH}
      -P ${ENGINE_ROOT}/cmake/WriteManifest.cmake
    DEPENDS ${MODEL_SO} ${VIEW_SO}
    COMMENT "Writing hotswap manifest"
  )
endfunction()
