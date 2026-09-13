#if !defined(EMSCRIPTEN) && !defined(FGFX_BACKEND_WEBGPU)
#include <GL/glew.h>
#endif
#include "View.hpp"
#include "EngineHeaders.hpp"
#include <fgfx/thorvg.hpp>

#include <algorithm>
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

#ifdef FGFX_BACKEND_WEBGPU
    fgfx::TextureDesc desc;
    desc.width = 512;
    desc.height = 512;
    // ThorVG's WgCanvas blit pipeline is compiled for BGRA8Unorm (context.format).
    desc.format = fgfx::PixelFormat::BGRA8Unorm;
    desc.generateMipmaps = false;
    vectorTex_ = fgfx::engine->getBackend()->createTexture(desc);
    vectorTexW_ = 512;
    vectorTexH_ = 512;
#else
    vectorTex_ = fgfx::engine->createEmptyTexture(512, 512, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
#endif
    vectorSprite_ = fgfx::engine->getEmptySprite("thorvg_circle");
  }

  void render(fgfx::FrameBuilder& builder, const void* snapshot, float delta) override {
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
    builder.context().mvp = mvp;

    int rasterSize = std::clamp(std::max(fgfx::engine->width, fgfx::engine->height), 256, 2048);
#ifdef FGFX_BACKEND_WEBGPU
    if (vectorTex_ != fgfx::kNullTexture &&
        (vectorTexW_ != rasterSize || vectorTexH_ != rasterSize)) {
      fgfx::engine->getBackend()->destroyTexture(vectorTex_);
      fgfx::TextureDesc desc;
      desc.width = rasterSize;
      desc.height = rasterSize;
      desc.format = fgfx::PixelFormat::BGRA8Unorm;
      desc.generateMipmaps = false;
      vectorTex_ = fgfx::engine->getBackend()->createTexture(desc);
      vectorTexW_ = rasterSize;
      vectorTexH_ = rasterSize;
      vectorReady_ = false;
    }

    if (vectorTex_ != fgfx::kNullTexture && vectorSprite_ && !vectorReady_) {
      if (fgfx::thorvg::begin(vectorTex_, rasterSize, rasterSize)) {
        float w = static_cast<float>(rasterSize);
        float h = static_cast<float>(rasterSize);
        fgfx::thorvg::addFilledCircle(w * 0.5f, h * 0.5f, w * 0.35f, h * 0.35f, 80, 180, 255, 255);
        fgfx::thorvg::addFilledCircle(w * 0.8f, h * 0.5f, w * 0.35f, h * 0.35f, 255, 180, 255, 255);
        vectorReady_ = fgfx::thorvg::submit();
      }
      if (vectorReady_) {
        vectorSprite_->setTextureFragment(
          static_cast<int>(vectorTex_),
          0.0f,
          1.0f,
          1.0f,
          0.0f,
          rasterSize,
          rasterSize,
          true
        );
      }
    }
#else
    if (vectorTex_ && (vectorTex_->width != rasterSize || vectorTex_->height != rasterSize)) {
      vectorTex_->resize(rasterSize, rasterSize);
      vectorReady_ = false;
    }

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
#endif

    if (!spriteBatch_) {
      spriteBatch_ = std::make_shared<fgfx::SpriteBatch>(fgfx::engine.get());
    }
    if (!glyphBatch_) {
      glyphBatch_ = std::make_shared<fgfx::GlyphBatch>(fgfx::engine.get());
    }

    fgfx::ClearValue clear;
    clear.r = 0.31f;
    clear.g = 0.52f;
    clear.b = 0.16f;
    clear.a = 1.0f;
    fgfx::RenderPass& pass = builder.beginPass("scene", builder.surfaceColor(),
                                               fgfx::LoadOp::Clear, clear);

    spriteBatch_->reset();
    if (vectorReady_ && vectorSprite_) {
      spriteBatch_->bufferSprite(
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
      spriteBatch_->bufferSprite(
        sprite,
        glm::vec2(0.0f, 0.0f),
        glm::vec4(0.4f, 0.9f, 0.5f, 1.0f),
        6.0f,
        0.0f
      );
    }
    spriteBatch_->record(pass, builder.context());

    auto fontAsset = fontRequest_ ? fontRequest_->get() : nullptr;
    if (fontAsset && fontAsset->font) {
      std::ostringstream fpsText;
      fpsText << "FPS: " << std::fixed << std::setprecision(0) << fps_;
      glyphBatch_->reset();
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 70.0f, 0)), glm::vec3(0.11f, 0.11f, 1)),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        fpsText.str()
      );
      std::ostringstream debugText;
      debugText << "resolution: " << fgfx::engine->width << "x" << fgfx::engine->height;
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 62.0f, 0)), glm::vec3(0.07f, 0.07f, 1)),
        glm::vec4(0.85f, 1.0f, 0.7f, 1.0f),
        debugText.str()
      );
      glyphBatch_->record(pass, builder.context());
    }

    builder.submitPass();
  }

private:
  std::shared_ptr<AssetRequestT<TextureAsset>> textureRequest_;
  std::shared_ptr<AssetRequestT<MSDFFontAsset>> fontRequest_;
  std::shared_ptr<fgfx::SpriteBatch> spriteBatch_;
  std::shared_ptr<fgfx::GlyphBatch> glyphBatch_;
#ifdef FGFX_BACKEND_WEBGPU
  fgfx::TextureHandle vectorTex_ = fgfx::kNullTexture;
  int vectorTexW_ = 0;
  int vectorTexH_ = 0;
#else
  std::shared_ptr<fgfx::Texture> vectorTex_;
#endif
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
