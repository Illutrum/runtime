#include "View.hpp"
#include "EngineHeaders.hpp"
#include <render/thorvg.hpp>

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

    auto format = render::PixelFormat::RGBA8Unorm;
    if (render::engine->getBackend() &&
        render::engine->getBackend()->type() == render::BackendType::WebGPU) {
      // WgCanvas blit pipeline is BGRA8Unorm. Sampling a BGRA texture already
      // returns RGB, so no fragment swizzle.
      format = render::PixelFormat::BGRA8Unorm;
    }
    vectorTex_ = render::engine->createEmptyTexture(512, 512, format);
    vectorSprite_ = render::engine->getEmptySprite("thorvg_circle");
  }

  void render(render::FrameBuilder& builder, const void* snapshot, float delta) override {
    (void)snapshot;

    if (delta > 0.0f) {
      float instantFps = 1.0f / delta;
      fps_ = fps_ <= 0.0f ? instantFps : fps_ * 0.9f + instantFps * 0.1f;
    }

    float aspectRatio = 1.0f * render::engine->width / render::engine->height;
    render::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

    int rasterSize = std::clamp(std::max(render::engine->width, render::engine->height), 256, 2048);
    if (vectorTex_ && (vectorTex_->width != rasterSize || vectorTex_->height != rasterSize)) {
      vectorTex_->recreate(rasterSize, rasterSize, vectorTex_->format);
      vectorReady_ = false;
    }

    if (vectorTex_ && vectorSprite_ && !vectorReady_) {
      render::RenderPass& tp = builder.beginPass(
        "thorvg_raster", vectorTex_->handle(),
        render::LoadOp::Clear, render::ClearValue{0, 0, 0, 0});
      tp.opaque = true;
      render::thorvg::record(tp, vectorTex_->handle(), rasterSize, rasterSize, [=] {
        float w = static_cast<float>(rasterSize);
        float h = static_cast<float>(rasterSize);
        render::thorvg::addFilledCircle(w * 0.5f, h * 0.5f, w * 0.35f, h * 0.35f, 80, 180, 255, 255);
        render::thorvg::addFilledCircle(w * 0.8f, h * 0.5f, w * 0.35f, h * 0.35f, 255, 180, 255, 255);
      });
      builder.submitPass();
      vectorReady_ = true;
      vectorSprite_->setTextureFragment(
        static_cast<int>(vectorTex_->handle()),
        0.0f, 1.0f, 1.0f, 0.0f,
        rasterSize, rasterSize, true);
    }

    if (!spriteBatch_) {
      spriteBatch_ = std::make_shared<render::SpriteBatch>(render::engine.get());
    }
    if (!glyphBatch_) {
      glyphBatch_ = std::make_shared<render::GlyphBatch>(render::engine.get());
    }

    render::ClearValue clear;
    clear.r = 0.31f;
    clear.g = 0.52f;
    clear.b = 0.16f;
    clear.a = 1.0f;
    render::RenderPass& pass = builder.beginPass("scene", builder.surfaceColor(),
                                               render::LoadOp::Clear, clear);

    spriteBatch_->reset();
    if (vectorReady_ && vectorSprite_) {
      spriteBatch_->bufferSprite(
        vectorSprite_,
        glm::vec2(-8.0f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        6.0f, 0.0f);
    }
    auto texture = textureRequest_ ? textureRequest_->get() : nullptr;
    if (texture && texture->texture && texture->texture->initialized) {
      auto sprite = render::engine->getSprite("assets/logo.png");
      spriteBatch_->bufferSprite(
        sprite,
        glm::vec2(0.0f, 0.0f),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        6.0f, 0.0f);
    }
    spriteBatch_->record(pass, builder.context());

    auto fontAsset = fontRequest_ ? fontRequest_->get() : nullptr;
    if (fontAsset && fontAsset->font) {
      const bool compute = builder.backend().supports(render::Feature::Compute);
      std::ostringstream fpsText;
      fpsText << "FPS: " << std::fixed << std::setprecision(0) << fps_;
      glyphBatch_->reset();
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 70.0f, 0)),
                   glm::vec3(0.11f, 0.11f, 1)),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        fpsText.str());
      std::ostringstream debugText;
      debugText << "resolution: " << render::engine->width << "x" << render::engine->height;
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 62.0f, 0)),
                   glm::vec3(0.07f, 0.07f, 1)),
        glm::vec4(0.85f, 1.0f, 0.7f, 1.0f),
        debugText.str());
      std::ostringstream featureText;
      featureText << "backend: "
                  << (builder.backend().type() == render::BackendType::WebGPU ? "WebGPU" : "GL")
                  << "  compute: " << (compute ? "yes" : "no");
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 56.0f, 0)),
                   glm::vec3(0.07f, 0.07f, 1)),
        glm::vec4(0.85f, 1.0f, 0.7f, 1.0f),
        featureText.str());
      glyphBatch_->record(pass, builder.context());
    }

    builder.submitPass();

    render::RenderPass& computePass = builder.beginPass(
      "compute_demo", builder.surfaceColor(), render::LoadOp::Load, render::ClearValue{});
    computePass.require(render::Feature::Compute);
    computePass.kind = render::PassKind::External;
    computePass.externalCallback = [](render::RenderBackend&) {};
    builder.submitPass();
  }

private:
  std::shared_ptr<AssetRequestT<TextureAsset>> textureRequest_;
  std::shared_ptr<AssetRequestT<MSDFFontAsset>> fontRequest_;
  std::shared_ptr<render::SpriteBatch> spriteBatch_;
  std::shared_ptr<render::GlyphBatch> glyphBatch_;
  std::shared_ptr<render::Texture> vectorTex_;
  std::shared_ptr<render::Sprite> vectorSprite_;
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
