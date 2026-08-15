#include <engine/Host.hpp>

#include "Input.hpp"
#include "Model.hpp"
#include "View.hpp"

#ifndef __HOTSWAP
extern "C" {
  void* view_create();
  void view_init(void* instance);
  void* model_create();
  void model_init(void* instance);
}
#endif

#include <filesystem>
#include <iostream>
#include <string>

namespace {

  Input buildInput(const engine::RawInput& raw) {
    Input input;
    input.moveX = 0.0f;
    input.moveY = 0.0f;
    if (raw.keys.test(262) || raw.keys.test(68)) input.moveX += 1.0f;
    if (raw.keys.test(263) || raw.keys.test(65)) input.moveX -= 1.0f;
    if (raw.keys.test(265) || raw.keys.test(87)) input.moveY += 1.0f;
    if (raw.keys.test(264) || raw.keys.test(83)) input.moveY -= 1.0f;
    input.reset = raw.resetRequested;
    return input;
  }

}

int main() {
  std::string buildDir = GAME_BUILD_DIR;
  std::string cmakeBuildDir = GAME_CMAKE_BINARY_DIR;
  std::string gameRoot = GAME_ROOT_DIR;

  engine::GameHostConfig config;
  config.gameSourceDir = gameRoot + "/src";
  config.gameBuildDir = cmakeBuildDir;
  config.cmakeSourceDir = GAME_CMAKE_SOURCE_DIR;
  config.modelSoPath = buildDir + "/libmodel.so";
  config.viewSoPath = buildDir + "/libview.so";
  config.manifestPath = buildDir + "/hotswap.manifest";
  config.assetsDir = buildDir + "/assets";

#ifdef EMSCRIPTEN
  engine::GameHost& host = *new engine::GameHost(config);
#else
  engine::GameHost host(config);
#endif
  host.init();

#ifndef __HOTSWAP
  void* modelInstance = model_create();
  model_init(modelInstance);
  host.modelBackend().attach(modelInstance);
  void* viewInstance = view_create();
  view_init(viewInstance);
  host.viewBackend().attach(viewInstance);
#endif

  host.setFrameCallback([&host](float delta, const engine::RawInput& raw) {
    auto* model = host.modelBackend().as<Model>();
    auto* view = host.viewBackend().as<View>();
    if (!model || !view) return;

    Input input = buildInput(raw);
    if (input.reset || host.resetRequested()) {
      model->init();
      host.clearResetRequest();
    }

    model->update(input, delta);
    view->render(model->snapshot(), delta);
  });

#ifndef EMSCRIPTEN
  host.run();
  host.destroy();
#endif
  return 0;
}
