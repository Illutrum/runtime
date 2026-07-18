#include <engine/Host.hpp>
#include "Input.hpp"
#include "Model.hpp"
#include "View.hpp"

int main() {
  engine::GameHostConfig config;
  config.gameSourceDir = GAME_ROOT_DIR "/src";
  config.gameBuildDir = GAME_CMAKE_BINARY_DIR;
  config.modelSoPath = GAME_BUILD_DIR "/libmodel.so";
  config.viewSoPath = GAME_BUILD_DIR "/libview.so";
  config.manifestPath = GAME_BUILD_DIR "/hotswap.manifest";
  config.assetsDir = GAME_ROOT_DIR "/assets";

  engine::GameHost host(config);
  host.init();
  host.setFrameCallback([&host](float delta, const engine::RawInput& raw) {
    auto* model = host.modelBackend().as<Model>();
    auto* view = host.viewBackend().as<View>();
    if (!model || !view) return;
    Input input;
    input.reset = raw.resetRequested;
    model->update(input, delta);
    view->render(model->snapshot(), delta);
  });
  host.run();
  host.destroy();
  return 0;
}
