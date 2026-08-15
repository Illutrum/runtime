#include "View.hpp"
#include "EngineHeaders.hpp"

#include <memory>
#include <string>

using namespace asset_manager;

class ExampleView : public View {
public:
  void init() override {
    TextureAsset::registerAssetType();
    textureRequest_ = manager.request<TextureAsset>("assets/logo.png");
    if (textureRequest_ && textureRequest_->asset) {
      textureRequest_->asset->start();
    }
  }

  void render(const void* snapshot, float delta) override {
    (void)snapshot;
    (void)delta;

    float aspectRatio = 1.0f * fgfx::engine->width / fgfx::engine->height;
    fgfx::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();

    glClearColor(0.31f, 0.52f, 0.16f, 1);
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
        glm::vec2(0.0f, 0.0f),
        glm::vec4(0.4f, 0.9f, 0.5f, 1.0f),
        6.0f,
        0.0f
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
