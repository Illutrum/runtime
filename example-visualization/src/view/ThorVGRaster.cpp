#include "ThorVGRaster.hpp"

#include "Texture.hpp"
#include <thorvg.h>

namespace viz {
namespace {

  tvg::GlCanvas* canvas = nullptr;
  bool initialized = false;
  bool sceneBuilt = false;
  GLuint stencilRbo = 0;

  const char* resultName(tvg::Result result) {
    switch (result) {
      case tvg::Result::Success: return "Success";
      case tvg::Result::InvalidArguments: return "InvalidArguments";
      case tvg::Result::InsufficientCondition: return "InsufficientCondition";
      case tvg::Result::FailedAllocation: return "FailedAllocation";
      case tvg::Result::MemoryCorruption: return "MemoryCorruption";
      case tvg::Result::NonSupport: return "NonSupport";
      default: return "Unknown";
    }
  }

  void attachStencil(fgfx::Texture& tex) {
    if (stencilRbo == 0) {
      glGenRenderbuffers(1, &stencilRbo);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, stencilRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tex.width, tex.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRbo);
  }

  bool bindTarget(fgfx::Texture& tex) {
    tvg::Result result = canvas->target(
      nullptr,
      nullptr,
      nullptr,
      static_cast<int32_t>(tex.framebuffer),
      static_cast<uint32_t>(tex.width),
      static_cast<uint32_t>(tex.height),
      tvg::ColorSpace::ABGR8888S
    );
    if (result == tvg::Result::Success) {
      return true;
    }
    fgfx_log("ThorVG target failed (%s), attaching stencil RBO", resultName(result));
    attachStencil(tex);
    result = canvas->target(
      nullptr,
      nullptr,
      nullptr,
      static_cast<int32_t>(tex.framebuffer),
      static_cast<uint32_t>(tex.width),
      static_cast<uint32_t>(tex.height),
      tvg::ColorSpace::ABGR8888S
    );
    if (result != tvg::Result::Success) {
      fgfx_log("ThorVG target failed after stencil attach (%s)", resultName(result));
      return false;
    }
    return true;
  }

}

  void ThorVGRaster::init() {
    if (initialized) {
      return;
    }
    tvg::Result result = tvg::Initializer::init(0);
    if (result != tvg::Result::Success) {
      fgfx_log("ThorVG Initializer::init failed (%s)", resultName(result));
      return;
    }
    canvas = tvg::GlCanvas::gen();
    if (!canvas) {
      fgfx_log("ThorVG GlCanvas::gen failed");
      tvg::Initializer::term();
      return;
    }
    initialized = true;
  }

  void ThorVGRaster::term() {
    if (!initialized) {
      return;
    }
    delete canvas;
    canvas = nullptr;
    sceneBuilt = false;
    if (stencilRbo != 0) {
      glDeleteRenderbuffers(1, &stencilRbo);
      stencilRbo = 0;
    }
    tvg::Initializer::term();
    initialized = false;
  }

  bool ThorVGRaster::rasterTo(fgfx::Texture& tex) {
    if (!initialized || !canvas) {
      fgfx_log("ThorVG raster skipped: not initialized");
      return false;
    }

    tex.beginRender();
    if (!bindTarget(tex)) {
      tex.endRender(false);
      return false;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (!sceneBuilt) {
      tvg::Shape* circle = tvg::Shape::gen();
      float cx = static_cast<float>(tex.width) * 0.5f;
      float cy = static_cast<float>(tex.height) * 0.5f;
      float radius = static_cast<float>(tex.width) * 0.35f;
      circle->appendCircle(cx, cy, radius, radius);
      circle->fill(80, 180, 255, 255);
      tvg::Result added = canvas->add(circle);
      if (added != tvg::Result::Success) {
        fgfx_log("ThorVG canvas->add failed (%s)", resultName(added));
        tvg::Paint::rel(circle);
        tex.endRender(false);
        return false;
      }
      sceneBuilt = true;
    }

    tvg::Result updated = canvas->update();
    if (updated != tvg::Result::Success) {
      fgfx_log("ThorVG update failed (%s)", resultName(updated));
      tex.endRender(false);
      return false;
    }
    tvg::Result drawn = canvas->draw(false);
    if (drawn != tvg::Result::Success) {
      fgfx_log("ThorVG draw failed (%s)", resultName(drawn));
      tex.endRender(false);
      return false;
    }
    tvg::Result synced = canvas->sync();
    if (synced != tvg::Result::Success) {
      fgfx_log("ThorVG sync failed (%s)", resultName(synced));
      tex.endRender(false);
      return false;
    }

    tex.endRender(false);
    return true;
  }

}
