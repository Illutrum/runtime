#include <engine/Host.hpp>

#include <LoadingScreen.hpp>
#include <asset_manager_fgfx.hpp>
#include <fastgfx.hpp>

namespace engine {

  void GameHost::renderLoadingUI() {
    float aspectRatio = 1.0f * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();

    glClearColor(0.1f, 0.12f, 0.16f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!loadingScreen_) {
      if (moduleFailed_ && barLayer_) {
        barLayer_->reset();
        barLayer_->bufferQuad(
          glm::vec3(-150, -85, 0), glm::vec3(150, -85, 0),
          glm::vec3(150, -80, 0), glm::vec3(-150, -80, 0),
          glm::vec4(0.6f, 0.15f, 0.15f, 0.9f)
        );
        barLayer_->upload();
        barLayer_->render(mvp);
      }
      return;
    }

    loadingScreen_->render(mvp);

    if (!errorMessage_.empty() && msdfLayer_ && loadingScreen_ && loadingScreen_->font) {
      msdfLayer_->reset();
      loadingScreen_->font->renderString(
        msdfLayer_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-140, 60, 0)), glm::vec3(0.08f, 0.08f, 1)),
        glm::vec4(1.0f, 0.3f, 0.3f, 1.0f),
        errorMessage_
      );
      msdfLayer_->upload();
      msdfLayer_->render(mvp);
    }
  }

  void GameHost::clearResetRequest() {
    const_cast<RawInput&>(input_.current()).resetRequested = false;
  }

  void GameHost::setErrorMessage(const std::string& message) {
    errorMessage_ = message;
    moduleFailed_ = !message.empty();
  }

}
