#include "View.hpp"
#include "GameState.hpp"

#include <asset_manager.hpp>
#include <asset_manager_fgfx.hpp>
#include <fastgfx.hpp>

#include <glm/glm.hpp>
#include <memory>
#include <string>

using namespace asset_manager;

class ExampleView : public View {
public:
  void init() override {
    TextureAsset::registerAssetType();
    textureRequest_ = manager.request<TextureAsset>("assets/logo.png");
  }

  void render(const void* snapshot, float delta) override {
    (void)delta;
    const auto* state = static_cast<const GameState*>(snapshot);
    if (!state) return;

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

    if (!spriteLayer_) {
      spriteLayer_ = fgfx::engine->createSpriteLayer();
    }

    auto texture = textureRequest_ ? textureRequest_->get() : nullptr;
    if (texture && texture->texture) {
      auto sprite = fgfx::engine->getSprite("assets/logo.png");
      spriteLayer_->reset();
      spriteLayer_->bufferSprite(
        sprite,
        glm::scale(
          glm::translate(glm::mat4(1.0f), glm::vec3(state->x, state->y, 0)),
          glm::vec3(0.35f, 0.35f, 1)
        ),
        glm::vec4(state->renderColorR, state->renderColorG, state->renderColorB, 1.0f)
      );
      spriteLayer_->upload();
      spriteLayer_->render(mvp);
    }
  }

private:
  std::shared_ptr<AssetRequestT<TextureAsset>> textureRequest_;
  std::shared_ptr<fgfx::SpriteLayer> spriteLayer_;
};

extern "C" {

  void* view_create() {
    return new ExampleView();
  }

  void view_destroy(void* instance) {
    delete static_cast<ExampleView*>(instance);
  }

  void view_init(void* instance) {
    static_cast<View*>(instance)->init();
  }

}
