include(FetchContent)
FetchContent_GetProperties(illutrum_render)
FetchContent_GetProperties(illutrum_assets)
FetchContent_GetProperties(rpc_cpp)

if(NOT RUNTIME_ROOT)
  set(RUNTIME_ROOT ${CMAKE_SOURCE_DIR})
endif()

set(FGFX_ROOT ${illutrum_render_SOURCE_DIR})
set(ASSETS_ROOT ${illutrum_assets_SOURCE_DIR})
set(ENGINE_ROOT ${RUNTIME_ROOT}/engine)
set(ENGINE_INCLUDE ${ENGINE_ROOT}/include)

function(apply_emscripten_webgl2_target TARGET)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  set(_exports "_main,_malloc,_free,_fgfx_render,_fgfx_io_fileLoaded,_fgfx_io_fileFailed,_fgfx_getEmptySprite,_fgfx_key_event,_fgfx_mouse_move,_fgfx_mouse_button")
  if(EMSCRIPTEN_EXTRA_EXPORTED_FUNCTIONS)
    set(_exports "${_exports},${EMSCRIPTEN_EXTRA_EXPORTED_FUNCTIONS}")
  endif()
  target_link_options(${TARGET} PRIVATE
    -sUSE_WEBGL2=1
    -sMIN_WEBGL_VERSION=2
    -sMAX_WEBGL_VERSION=2
    "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,allocate,intArrayFromString,HEAPU8"
    "-sEXPORTED_FUNCTIONS=${_exports}"
    "--post-js=${FGFX_ROOT}/js/emscripten-glue.js"
  )
endfunction()

function(ensure_emscripten_webgpu)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  if(TARGET webgpu)
    return()
  endif()
  set(WEBGPU_BACKEND "EMDAWNWEBGPU" CACHE STRING "" FORCE)
  set(_saved_exe_linker_flags "${CMAKE_EXE_LINKER_FLAGS}")
  add_subdirectory(
    ${FGFX_ROOT}/webgpu
    ${CMAKE_BINARY_DIR}/_deps/webgpu-build
  )
  # project(webgpu) re-inits and can clear linker flags.
  set(CMAKE_EXE_LINKER_FLAGS "${_saved_exe_linker_flags}"
      CACHE STRING "exe linker flags" FORCE)
endfunction()

function(apply_emscripten_webgpu_target TARGET)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  ensure_emscripten_webgpu()
  target_compile_definitions(${TARGET} PRIVATE FGFX_BACKEND_WEBGPU)
  target_link_libraries(${TARGET} PRIVATE webgpu)
  set(_exports "_main,_malloc,_free,_fgfx_render,_fgfx_io_fileLoaded,_fgfx_io_fileFailed,_fgfx_getEmptySprite,_fgfx_createTextureRGBA,_fgfx_key_event,_fgfx_mouse_move,_fgfx_mouse_button")
  if(EMSCRIPTEN_EXTRA_EXPORTED_FUNCTIONS)
    set(_exports "${_exports},${EMSCRIPTEN_EXTRA_EXPORTED_FUNCTIONS}")
  endif()
  target_link_options(${TARGET} PRIVATE
    "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,allocate,intArrayFromString,HEAPU8"
    "-sEXPORTED_FUNCTIONS=${_exports}"
    "--post-js=${FGFX_ROOT}/js/emscripten-glue.js"
  )
endfunction()

function(apply_emscripten_backend_target TARGET)
  if(NOT EMSCRIPTEN)
    return()
  endif()
  if(FGFX_BACKEND STREQUAL "WebGPU")
    apply_emscripten_webgpu_target(${TARGET})
  else()
    apply_emscripten_webgl2_target(${TARGET})
  endif()
endfunction()

function(runtime_apply_p2996 TARGET)
  if(COMMAND apply_p2996_target)
    apply_p2996_target(${TARGET})
  endif()
  if(COMMAND apply_p2996_rpath_origin)
    apply_p2996_rpath_origin(${TARGET})
  elseif(NOT EMSCRIPTEN)
    target_link_options(${TARGET} PRIVATE "-Wl,-rpath,$ORIGIN")
  endif()
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
  else()
    target_sources(${TARGET} PRIVATE ${ENGINE_ROOT}/src/WasmInputBinding.cpp)
  endif()

  target_include_directories(${TARGET} PRIVATE
    ${ENGINE_INCLUDE}
    ${ENGINE_ROOT}/src
    ${FGFX_ROOT}/include
    ${FGFX_ROOT}/src
    ${FGFX_ROOT}/glm
    ${ASSETS_ROOT}/include
    ${ASSETS_ROOT}/src
    ${ASSETS_ROOT}/ui
    ${ASSETS_ROOT}/fgfx/include
    ${ASSETS_ROOT}/fgfx
    ${RUNTIME_ROOT}/hotswap/include
    ${RUNTIME_ROOT}/hotswap/assets
    ${rpc_cpp_SOURCE_DIR}/src
  )

  if(NOT EMSCRIPTEN)
    target_compile_definitions(${TARGET} PRIVATE __USE_OPENGL __USE_GLFW)
    target_link_libraries(${TARGET} PRIVATE engine_runtime)
    runtime_apply_p2996(${TARGET})
    if(GAME_DEV_HOTSWAP)
      target_compile_definitions(${TARGET} PRIVATE __HOTSWAP)
      target_link_libraries(${TARGET} PRIVATE hotswap dl)
    endif()
  else()
    target_sources(${TARGET} PRIVATE
      ${ASSETS_ROOT}/src/Asset.cpp
      ${ASSETS_ROOT}/src/AssetTypeRegistry.cpp
      ${ASSETS_ROOT}/src/AssetRequest.cpp
      ${ASSETS_ROOT}/src/AssetsManager.cpp
      ${ASSETS_ROOT}/src/LoadTask.cpp
      ${ASSETS_ROOT}/fgfx/TextureAsset.cpp
      ${ASSETS_ROOT}/fgfx/MSDFFontAsset.cpp
      ${ASSETS_ROOT}/fgfx/JsonAsset.cpp
      ${ASSETS_ROOT}/ui/LoadingScreen.cpp
    )
    aux_source_directory(${FGFX_ROOT}/src FGFX_ENGINE_SOURCES)
    list(APPEND FGFX_ENGINE_SOURCES ${FGFX_ROOT}/src/io/FileLoader.cpp)
    list(APPEND FGFX_ENGINE_SOURCES ${FGFX_ROOT}/src/backend/WebGLBackend.cpp)
    if(FGFX_BACKEND STREQUAL "WebGPU")
      list(APPEND FGFX_ENGINE_SOURCES
        ${FGFX_ROOT}/src/backend/WebGPUBackend.cpp
        ${FGFX_ROOT}/src/backend/WebGPUBackendWasm.cpp)
    endif()
    target_sources(${TARGET} PRIVATE ${FGFX_ENGINE_SOURCES})
    apply_emscripten_backend_target(${TARGET})
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
    ${rpc_cpp_SOURCE_DIR}/src
    ${FGFX_ROOT}/glm
    ${FGFX_ROOT}/include
    ${ASSETS_ROOT}/include
    ${ASSETS_ROOT}/fgfx
  )

  if(HP_HEADERS_PATH)
    target_include_directories(${TARGET} PRIVATE ${HP_HEADERS_PATH})
  endif()

  if(COMMAND apply_p2996_plugin_target)
    target_compile_options(${TARGET} PRIVATE
      -fmodules
      -fcxx-modules
      -fmodules-cache-path=${CMAKE_BINARY_DIR}/clang-modules
      -fmodule-map-file=${HP_MODULE_MAP}
      -fno-autolink
    )
    apply_p2996_plugin_target(${TARGET})
  endif()

  if(APPLE)
    set_target_properties(${TARGET} PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
    )
  endif()

  set_target_properties(${TARGET} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
    BUILD_RPATH "$ORIGIN"
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
