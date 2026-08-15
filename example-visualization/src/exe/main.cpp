#include <engine/Host.hpp>
#include "View.hpp"

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

  engine::GameHost host(config);
  host.init();

  host.setFrameCallback([&host](float delta, const engine::RawInput& raw) {
    (void)raw;
    auto* view = host.viewBackend().as<View>();
    if (!view) return;
    view->render(nullptr, delta);
  });

  host.run();
  host.destroy();
  return 0;
}
