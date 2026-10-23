#include <engine/Host.hpp>
#include "View.hpp"

#ifndef __HOTSWAP
extern "C" {
  void* view_create();
  void view_init(void* instance);
}
#endif

#include <string>

int main() {
  std::string buildDir = GAME_BUILD_DIR;
  std::string cmakeBuildDir = GAME_CMAKE_BINARY_DIR;
  std::string gameRoot = GAME_ROOT_DIR;

  engine::GameHostConfig config;
  config.gameSourceDir = gameRoot + "/src";
  config.gameBuildDir = cmakeBuildDir;
  config.cmakeSourceDir = GAME_CMAKE_SOURCE_DIR;
  config.viewSoPath = buildDir + "/libviz.so";
  config.manifestPath = buildDir + "/visualization.manifest";
  config.assetsDir = buildDir + "/assets";
  config.viewTarget = "example_visualization_view";
  config.modelTarget.clear();

#ifdef EMSCRIPTEN
  engine::GameHost& host = *new engine::GameHost(config);
#else
  engine::GameHost host(config);
#endif
  host.init();

#ifndef __HOTSWAP
  void* viewInstance = view_create();
  view_init(viewInstance);
  host.viewBackend().attach(viewInstance);
#endif

  host.setFrameCallback([&host](render::FrameBuilder& builder, float delta, const engine::RawInput& raw) {
    (void)raw;
    auto* view = host.viewBackend().as<View>();
    if (!view) return;
    view->render(builder, nullptr, delta);
  });

#ifndef EMSCRIPTEN
  host.run();
  host.destroy();
#endif
  return 0;
}
