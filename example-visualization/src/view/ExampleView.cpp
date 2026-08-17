#ifndef EMSCRIPTEN
#include <GL/glew.h>
#endif
#include "View.hpp"
#include "EngineHeaders.hpp"
#include <fgfx/thorvg.hpp>

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

    vectorTex_ = fgfx::engine->createEmptyTexture(512, 512, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
    vectorSprite_ = fgfx::engine->getEmptySprite("thorvg_circle");
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

    if (vectorTex_ && vectorSprite_ && !vectorReady_) {
      if (fgfx::thorvg::begin(*vectorTex_)) {
        float w = static_cast<float>(vectorTex_->width);
        float h = static_cast<float>(vectorTex_->height);
        fgfx::thorvg::addFilledCircle(w * 0.5f, h * 0.5f, w * 0.35f, h * 0.35f, 80, 180, 255, 255);
        fgfx::thorvg::addFilledCircle(w * 0.8f, h * 0.5f, w * 0.35f, h * 0.35f, 255, 180, 255, 255);
        vectorReady_ = fgfx::thorvg::submit();
      }
      if (vectorReady_) {
        vectorSprite_->setTextureFragment(
          vectorTex_->texture,
          0.0f,
          1.0f,
          1.0f,
          0.0f,
          vectorTex_->width,
          vectorTex_->height,
          true
        );
      }
    }

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

    spriteLayer_->reset();
    if (vectorReady_ && vectorSprite_) {
      spriteLayer_->bufferSprite(
        vectorSprite_,
        glm::vec2(-8.0f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        6.0f,
        0.0f
      );
    }
    auto texture = textureRequest_ ? textureRequest_->get() : nullptr;
    if (texture && texture->texture) {
      auto sprite = fgfx::engine->getSprite("assets/logo.png");
      spriteLayer_->bufferSprite(
        sprite,
        glm::vec2(0.0f, 0.0f),
        glm::vec4(0.4f, 0.9f, 0.5f, 1.0f),
        6.0f,
        0.0f
      );
    }
    spriteLayer_->upload();
    spriteLayer_->render(mvp);

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
  std::shared_ptr<fgfx::Texture> vectorTex_;
  std::shared_ptr<fgfx::Sprite> vectorSprite_;
  bool vectorReady_ = false;
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
