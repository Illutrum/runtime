#include "View.hpp"
#include "GameState.hpp"
#include "EngineHeaders.hpp"

#include <memory>
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

  void render(render::FrameBuilder& builder, const void* snapshot, float delta) override {
    (void)delta;
    const auto* state = static_cast<const GameState*>(snapshot);
    if (!state) return;

    float aspectRatio = 1.0f * render::engine->width / render::engine->height;
    render::ModelViewProjection mvp = {
      .projectionMatrix = glm::perspective(35.0f, aspectRatio, 0.1f, 1000.0f),
      .viewMatrix = glm::lookAt(glm::vec3(0, 0, 20), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)),
      .modelMatrix = glm::mat4(1.0),
      .eyePosition = glm::vec3(0, 0, 20)
    };
    mvp.multiply();
    builder.context().mvp = mvp;

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

    auto texture = textureRequest_ ? textureRequest_->get() : nullptr;
    if (texture && texture->texture) {
      auto sprite = render::engine->getSprite("assets/logo.png");
      spriteBatch_->reset();
      spriteBatch_->bufferSprite(
        sprite,
        glm::vec2(state->x, state->y),
        glm::vec4(state->renderColorR, state->renderColorG, state->renderColorB, 1.0f),
        5.0f,
        0.0f
      );
      spriteBatch_->record(pass, builder.context());
    }

    auto fontAsset = fontRequest_ ? fontRequest_->get() : nullptr;
    if (fontAsset && fontAsset->font) {
      glyphBatch_->reset();
      fontAsset->font->renderString(
        glyphBatch_.get(),
        glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-40.0f, 70.0f, 0)),
                   glm::vec3(0.11f, 0.11f, 1)),
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
        "FAST GFX"
      );
      glyphBatch_->record(pass, builder.context());
    }

    builder.submitPass();
  }

private:
  std::shared_ptr<AssetRequestT<TextureAsset>> textureRequest_;
  std::shared_ptr<AssetRequestT<MSDFFontAsset>> fontRequest_;
  std::shared_ptr<render::SpriteBatch> spriteBatch_;
  std::shared_ptr<render::GlyphBatch> glyphBatch_;
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
