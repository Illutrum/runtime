#include "View.hpp"
#include "EngineHeaders.hpp"

#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

using namespace asset_manager;

class ExampleView : public View {
public:
  void init() override {
    TextureAsset::registerAssetType();
    MSDFFontAsset::registerAssetType();
    textureRequest_ = manager.request<TextureAsset>("assets/logo.png");
    if (textureRequest_ && textureRequest_->asset) {
      textureRequest_->asset->start();
    }
    fontRequest_ = manager.request<MSDFFontAsset>("assets/ui");
    if (fontRequest_ && fontRequest_->asset) {
      fontRequest_->asset->start();
    }
  }

  void render(const void* snapshot, float delta) override {
    (void)snapshot;

    if (delta > 0.0f) {
      float instantFps = 1.0f / delta;
      fps_ = fps_ <= 0.0f ? instantFps : fps_ * 0.9f + instantFps * 0.1f;
    }

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
    if (!msdfLayer_) {
      msdfLayer_ = fgfx::engine->createMSDFLayer();
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

    auto fontAsset = fontRequest_ ? fontRequest_->get() : nullptr;
    if (fontAsset && fontAsset->font) {
      std::ostringstream fpsText;
      fpsText << "FPS: " << std::fixed << std::setprecision(0) << fps_;
      msdfLayer_->reset();
      fontAsset->font->renderString(
        msdfLayer_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 90.0f, 0)), glm::vec3(0.11f, 0.11f, 1)),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        fpsText.str()
      );
      fontAsset->font->renderString(
        msdfLayer_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 82.0f, 0)), glm::vec3(0.07f, 0.07f, 1)),
        glm::vec4(0.85f, 1.0f, 0.7f, 1.0f),
        "MSDF font overlay"
      );
      msdfLayer_->upload();
      msdfLayer_->render(mvp);
    }
  }

private:
  std::shared_ptr<AssetRequestT<TextureAsset>> textureRequest_;
  std::shared_ptr<AssetRequestT<MSDFFontAsset>> fontRequest_;
  std::shared_ptr<fgfx::SpriteLayer> spriteLayer_;
  std::shared_ptr<fgfx::MSDFLayer> msdfLayer_;
  float fps_ = 0.0f;
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
