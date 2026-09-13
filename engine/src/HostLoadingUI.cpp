#include <engine/Host.hpp>

#include <LoadingScreen.hpp>
#include <asset_manager_fgfx.hpp>
#include <fastgfx.hpp>

namespace engine {

  void GameHost::renderLoadingUI(fgfx::FrameBuilder& builder) {
    float aspectRatio = 1.0f * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

    fgfx::ClearValue clear;
    clear.r = 0.1f;
    clear.g = 0.12f;
    clear.b = 0.16f;
    clear.a = 1.0f;
    fgfx::RenderPass& pass = builder.beginPass("loading", builder.surfaceColor(),
                                               fgfx::LoadOp::Clear, clear);

    if (loadingScreen_) {
      loadingScreen_->record(pass, builder.context());
    } else if (moduleFailed_ && barBatch_) {
      barBatch_->reset();
      barBatch_->bufferQuad(
        glm::vec3(-150, -85, 0), glm::vec3(150, -85, 0),
        glm::vec3(150, -80, 0), glm::vec3(-150, -80, 0),
        glm::vec4(0.6f, 0.15f, 0.15f, 0.9f)
      );
      barBatch_->record(pass, builder.context());
    }

    builder.submitPass();
  }

  void GameHost::clearResetRequest() {
    const_cast<RawInput&>(input_.current()).resetRequested = false;
  }

  void GameHost::setErrorMessage(const std::string& message) {
    errorMessage_ = message;
    moduleFailed_ = !message.empty();
  }

}
