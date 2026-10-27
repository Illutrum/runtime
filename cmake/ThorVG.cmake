find_program(MESON_EXECUTABLE meson HINTS /usr/bin REQUIRED)
find_program(NINJA_EXECUTABLE ninja HINTS /usr/bin REQUIRED)

if(NOT RUNTIME_ROOT)
  set(RUNTIME_ROOT ${CMAKE_SOURCE_DIR})
endif()
set(THORVG_SOURCE_DIR ${RUNTIME_ROOT}/thorvg)
set(THORVG_BUILD_DIR ${CMAKE_BINARY_DIR}/_deps/thorvg-build)
set(THORVG_INSTALL_DIR ${CMAKE_BINARY_DIR}/_deps/thorvg-install)
set(THORVG_LIB ${THORVG_INSTALL_DIR}/lib/libthorvg-1.a)

if(NOT EXISTS ${THORVG_SOURCE_DIR}/meson.build)
  message(FATAL_ERROR "ThorVG submodule missing at ${THORVG_SOURCE_DIR}. Run: git submodule update --init thorvg")
endif()

if(FGFX_BACKEND STREQUAL "WebGPU")
  set(THORVG_ENGINES "wg")
else()
  set(THORVG_ENGINES "gl")
endif()

if(NOT EMSCRIPTEN)
  if(DEFINED P2996_CXX_INCLUDE)
    set(THORVG_CXXFLAGS "-nostdinc++ -I${P2996_CXX_INCLUDE_TRIPLET} -I${P2996_CXX_INCLUDE} -std=c++17 -stdlib=libc++")
  elseif(DEFINED LIBCXX_INCLUDE_DIR_ARCH)
    set(THORVG_CXXFLAGS "-nostdinc++ -I${LIBCXX_INCLUDE_DIR_ARCH} -I${LIBCXX_INCLUDE_DIR_GEN} -std=c++17 -stdlib=libc++")
  else()
    set(THORVG_CXXFLAGS "-std=c++17")
  endif()
  set(THORVG_EXTRA "")
else()
  set(THORVG_CXXFLAGS "-std=c++17")
  if(FGFX_BACKEND STREQUAL "WebGPU")
    set(THORVG_EXTRA "")
    if(DEFINED emdawnwebgpu_SOURCE_DIR)
      set(_emdawn_inc "${emdawnwebgpu_SOURCE_DIR}/webgpu/include")
    else()
      set(_emdawn_inc "${CMAKE_BINARY_DIR}/_deps/emdawnwebgpu-src/webgpu/include")
    endif()
    set(THORVG_CXXFLAGS "${THORVG_CXXFLAGS} -I${_emdawn_inc}")
    set(THORVG_CPP_ARGS "-I${_emdawn_inc}")
  else()
    set(THORVG_EXTRA "opengl_es")
  endif()
  if(NOT DEFINED EMSCRIPTEN_ROOT_PATH)
    get_filename_component(EMSCRIPTEN_ROOT_PATH ${CMAKE_CXX_COMPILER} DIRECTORY)
  endif()
  set(THORVG_EM_CONFIG "$ENV{EM_CONFIG}")
  if(NOT THORVG_EM_CONFIG)
    set(THORVG_EM_CONFIG "${CMAKE_SOURCE_DIR}/cpp-compiler/sysroot/.emscripten")
  endif()
  set(THORVG_LLVM_BIN "${CMAKE_SOURCE_DIR}/cpp-compiler/sysroot/usr/local/bin")
  set(THORVG_CROSS_FILE ${CMAKE_BINARY_DIR}/thorvg-emscripten.cross)
  file(WRITE ${THORVG_CROSS_FILE}
"[binaries]
c = '${CMAKE_C_COMPILER}'
cpp = '${CMAKE_CXX_COMPILER}'
ar = '${EMSCRIPTEN_ROOT_PATH}/emar'
ranlib = '${EMSCRIPTEN_ROOT_PATH}/emranlib'

[built-in options]
cpp_std = 'c++17'

[host_machine]
system = 'emscripten'
cpu_family = 'wasm32'
cpu = 'wasm32'
endian = 'little'
")
endif()

if(NOT EMSCRIPTEN AND FGFX_BACKEND STREQUAL "WebGPU")
  # dawn::webgpu_dawn is IMPORTED (directory-scoped). Use the cache path
  # published by FetchDawnPrecompiled instead.
  if(NOT WEBGPU_RUNTIME_LIB)
    message(FATAL_ERROR "ThorVG WebGPU requires Dawn (WEBGPU_RUNTIME_LIB). Include Dawn.cmake first.")
  endif()
  get_filename_component(_dawn_libdir "${WEBGPU_RUNTIME_LIB}" DIRECTORY)
  get_filename_component(_dawn_prefix "${_dawn_libdir}" DIRECTORY)
  set(_dawn_inc "${_dawn_prefix}/include")
  if(NOT EXISTS "${_dawn_inc}/webgpu/webgpu.h")
    message(FATAL_ERROR "Dawn webgpu.h not found at ${_dawn_inc}/webgpu/webgpu.h")
  endif()
  file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/_deps)
  file(WRITE ${CMAKE_BINARY_DIR}/_deps/wgpu_native.pc
"Name: wgpu_native
Description: WebGPU C API (Dawn)
Version: 1.0.0
Cflags: -I${_dawn_inc}
Libs: -L${_dawn_libdir} -lwebgpu_dawn
")
  set(THORVG_CXXFLAGS "${THORVG_CXXFLAGS} -I${_dawn_inc}")
endif()

set(THORVG_MESON_SETUP
  ${MESON_EXECUTABLE} setup
  ${THORVG_BUILD_DIR}
  ${THORVG_SOURCE_DIR}
  --prefix=${THORVG_INSTALL_DIR}
  --libdir=lib
  --default-library=static
  --backend=ninja
  -Dcpp_std=c++17
  -Dengines=${THORVG_ENGINES}
  -Dloaders=svg
  "-Dsavers="
  "-Dbindings="
  "-Dtools="
  "-Dextra=${THORVG_EXTRA}"
  -Dtests=false
  -Dfile=false
  -Dthreads=false
  -Dstatic=true
)
if(DEFINED THORVG_CPP_ARGS)
  list(APPEND THORVG_MESON_SETUP "-Dcpp_args=${THORVG_CPP_ARGS}")
endif()
if(EXISTS "${THORVG_BUILD_DIR}/meson-info")
  list(APPEND THORVG_MESON_SETUP --reconfigure)
endif()
if(NOT EMSCRIPTEN AND FGFX_BACKEND STREQUAL "WebGPU")
  list(APPEND THORVG_MESON_SETUP --pkg-config-path=${CMAKE_BINARY_DIR}/_deps)
endif()
if(EMSCRIPTEN)
  list(APPEND THORVG_MESON_SETUP --cross-file=${THORVG_CROSS_FILE})
endif()

set(THORVG_ENV
  ${CMAKE_COMMAND} -E env
    "CC=${CMAKE_C_COMPILER}"
    "CXX=${CMAKE_CXX_COMPILER}"
    "CXXFLAGS=${THORVG_CXXFLAGS}"
    "CFLAGS=-std=c17"
)
if(NOT EMSCRIPTEN AND FGFX_BACKEND STREQUAL "WebGPU")
  list(APPEND THORVG_ENV "PKG_CONFIG_PATH=${CMAKE_BINARY_DIR}/_deps:$ENV{PKG_CONFIG_PATH}")
endif()
if(EMSCRIPTEN)
  list(APPEND THORVG_ENV
    "EM_CONFIG=${THORVG_EM_CONFIG}"
    "PATH=${THORVG_LLVM_BIN}:$ENV{PATH}"
  )
endif()

include(ExternalProject)
ExternalProject_Add(thorvg_ep
  SOURCE_DIR ${THORVG_SOURCE_DIR}
  BINARY_DIR ${THORVG_BUILD_DIR}
  INSTALL_DIR ${THORVG_INSTALL_DIR}
  DOWNLOAD_COMMAND ""
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ${THORVG_ENV} ${THORVG_MESON_SETUP}
  BUILD_COMMAND ${THORVG_ENV} ${MESON_EXECUTABLE} compile -C ${THORVG_BUILD_DIR}
  INSTALL_COMMAND ${THORVG_ENV} ${MESON_EXECUTABLE} install -C ${THORVG_BUILD_DIR}
  BUILD_BYPRODUCTS ${THORVG_LIB}
)

add_library(thorvg STATIC IMPORTED GLOBAL)
set_target_properties(thorvg PROPERTIES
  IMPORTED_LOCATION ${THORVG_LIB}
  INTERFACE_INCLUDE_DIRECTORIES ${THORVG_SOURCE_DIR}/inc
  INTERFACE_COMPILE_DEFINITIONS TVG_STATIC
)
add_dependencies(thorvg thorvg_ep)
