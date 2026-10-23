#include <render/thorvg.hpp>

#include <render.hpp>
#include <thorvg.h>
#include "config.hpp"

#ifdef FGFX_BACKEND_WEBGPU
#include "backend/WebGPUBackend.hpp"
#else
#include "backend/WebGLBackend.hpp"
#ifdef EMSCRIPTEN
#include <emscripten/html5_webgl.h>
#endif
#endif

namespace render::thorvg {
namespace {

#ifdef FGFX_BACKEND_WEBGPU
  tvg::WgCanvas* canvas = nullptr;
#else
  tvg::GlCanvas* canvas = nullptr;
  void* glContext = nullptr;
#ifndef EMSCRIPTEN
  void (*clearColorFn)(float, float, float, float) = nullptr;
  void (*clearFn)(unsigned int) = nullptr;
#endif
#endif
  bool initialized = false;
  bool frameOpen = false;

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

#ifndef FGFX_BACKEND_WEBGPU
  void* currentContext() {
#ifdef EMSCRIPTEN
    return reinterpret_cast<void*>(emscripten_webgl_get_current_context());
#else
    return glfwGetCurrentContext();
#endif
  }

  void clearRgba(float r, float g, float b, float a) {
#ifdef EMSCRIPTEN
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
#else
    clearColorFn(r, g, b, a);
    clearFn(GL_COLOR_BUFFER_BIT);
#endif
  }
#endif

  void abortFrame() {
    if (!frameOpen) {
      return;
    }
#ifndef FGFX_BACKEND_WEBGPU
    if (auto* be = dynamic_cast<WebGLBackend*>(engine->getBackend())) {
      be->unbindColorTarget();
    }
#endif
    frameOpen = false;
  }

  void term() {
    if (!initialized) {
      return;
    }
    abortFrame();
    delete canvas;
    canvas = nullptr;
#ifndef FGFX_BACKEND_WEBGPU
    glContext = nullptr;
#ifndef EMSCRIPTEN
    clearColorFn = nullptr;
    clearFn = nullptr;
#endif
#endif
    tvg::Initializer::term();
    initialized = false;
  }

  bool ensureInitialized() {
    if (initialized) {
      return canvas != nullptr;
    }

    tvg::Result result = tvg::Initializer::init(0);
    if (result != tvg::Result::Success) {
      render_log("ThorVG Initializer::init failed (%s)", resultName(result));
      return false;
    }
#ifdef FGFX_BACKEND_WEBGPU
    canvas = tvg::WgCanvas::gen();
    if (!canvas) {
      render_log("ThorVG WgCanvas::gen failed");
      tvg::Initializer::term();
      return false;
    }
#else
    canvas = tvg::GlCanvas::gen();
    if (!canvas) {
      render_log("ThorVG GlCanvas::gen failed");
      tvg::Initializer::term();
      return false;
    }

    glContext = currentContext();
#ifndef EMSCRIPTEN
    clearColorFn = reinterpret_cast<void (*)(float, float, float, float)>(
      glfwGetProcAddress("glClearColor")
    );
    clearFn = reinterpret_cast<void (*)(unsigned int)>(glfwGetProcAddress("glClear"));
#endif
    if (!glContext) {
      render_log("ThorVG missing current GL context");
      delete canvas;
      canvas = nullptr;
      tvg::Initializer::term();
      return false;
    }
#ifndef EMSCRIPTEN
    if (!clearColorFn || !clearFn) {
      render_log("ThorVG missing glClear/glClearColor");
      delete canvas;
      canvas = nullptr;
      glContext = nullptr;
      tvg::Initializer::term();
      return false;
    }
#endif
#endif

    initialized = true;
    render::addShutdownHook(&term);
    render_log("ThorVG initialized (lazy)");
    return true;
  }

}

  void record(RenderPass& pass, TextureHandle tex, int width, int height,
              std::function<void()> buildShapes) {
    pass.kind = PassKind::External;
    pass.colorTarget = tex;
    pass.viewportW = width;
    pass.viewportH = height;
    pass.require(Feature::RasterizeToTexture);
    pass.externalCallback = [tex, width, height,
                             build = std::move(buildShapes)](RenderBackend&) {
      if (!begin(tex, width, height)) {
        return;
      }
      build();
      submit();
    };
  }

  bool begin(TextureHandle tex, int width, int height) {
    if (!ensureInitialized()) {
      return false;
    }
    if (frameOpen) {
      render_log("ThorVG begin skipped: frame already open");
      return false;
    }
#ifdef FGFX_BACKEND_WEBGPU
    auto* be = dynamic_cast<WebGPUBackend*>(engine->getBackend());
    if (!be) {
      render_log("ThorVG begin skipped: WebGPUBackend missing");
      return false;
    }
    WGPUTexture rawTex = be->wgpuTexture(tex);
    if (!rawTex) {
      render_log("ThorVG begin skipped: null WGPUTexture");
      return false;
    }
    tvg::Result result = canvas->target(
      be->wgpuDevice(),
      be->wgpuInstance(),
      rawTex,
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
      tvg::ColorSpace::ABGR8888S,
      1
    );
    if (result != tvg::Result::Success) {
      render_log("ThorVG WgCanvas::target failed (%s)", resultName(result));
      return false;
    }
#else
    auto* be = dynamic_cast<WebGLBackend*>(engine->getBackend());
    if (!be) {
      render_log("ThorVG begin skipped: WebGLBackend missing");
      return false;
    }
    GLuint fbo = be->ensureFramebuffer(tex);
    be->bindColorTarget(tex, width, height);
    be->attachDepthStencil(width, height);
    tvg::Result result = canvas->target(
      nullptr,
      nullptr,
      glContext,
      static_cast<int32_t>(fbo),
      static_cast<uint32_t>(width),
      static_cast<uint32_t>(height),
      tvg::ColorSpace::ABGR8888S
    );
    if (result != tvg::Result::Success) {
      render_log("ThorVG target failed after stencil attach (%s)", resultName(result));
      be->unbindColorTarget();
      return false;
    }
    clearRgba(0.0f, 0.0f, 0.0f, 0.0f);
#endif
    frameOpen = true;
    return true;
  }

  void addFilledCircle(float cx, float cy, float rx, float ry,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!frameOpen || !canvas) {
      render_log("ThorVG addFilledCircle skipped: no open frame");
      return;
    }
    tvg::Shape* circle = tvg::Shape::gen();
    circle->appendCircle(cx, cy, rx, ry);
    circle->fill(r, g, b, a);
    tvg::Result added = canvas->add(circle);
    if (added != tvg::Result::Success) {
      render_log("ThorVG canvas->add failed (%s)", resultName(added));
      tvg::Paint::rel(circle);
    }
  }

  void clear() {
    if (!canvas) {
      return;
    }
    tvg::Result removed = canvas->remove();
    if (removed != tvg::Result::Success) {
      render_log("ThorVG canvas->remove failed (%s)", resultName(removed));
    }
  }

  void addSvg(const char* data, uint32_t size, float w, float h) {
    if (!frameOpen || !canvas) {
      render_log("ThorVG addSvg skipped: no open frame");
      return;
    }
    if (!data || size == 0) {
      render_log("ThorVG addSvg skipped: empty data");
      return;
    }
    tvg::Picture* picture = tvg::Picture::gen();
    tvg::Result loaded = picture->load(data, size, "svg", nullptr, true);
    if (loaded != tvg::Result::Success) {
      render_log("ThorVG Picture::load(svg) failed (%s)", resultName(loaded));
      tvg::Paint::rel(picture);
      return;
    }
    tvg::Result sized = picture->size(w, h);
    if (sized != tvg::Result::Success) {
      render_log("ThorVG Picture::size failed (%s)", resultName(sized));
    }
    tvg::Result added = canvas->add(picture);
    if (added != tvg::Result::Success) {
      render_log("ThorVG canvas->add(svg) failed (%s)", resultName(added));
      tvg::Paint::rel(picture);
    }
  }

  bool submit() {
    if (!frameOpen || !canvas) {
      render_log("ThorVG submit skipped: no open frame");
      return false;
    }

    tvg::Result updated = canvas->update();
    if (updated != tvg::Result::Success) {
      render_log("ThorVG update failed (%s)", resultName(updated));
      abortFrame();
      return false;
    }
    tvg::Result drawn = canvas->draw(false);
    if (drawn != tvg::Result::Success) {
      render_log("ThorVG draw failed (%s)", resultName(drawn));
      abortFrame();
      return false;
    }
    tvg::Result synced = canvas->sync();
    if (synced != tvg::Result::Success) {
      render_log("ThorVG sync failed (%s)", resultName(synced));
      abortFrame();
      return false;
    }

#ifndef FGFX_BACKEND_WEBGPU
    if (auto* be = dynamic_cast<WebGLBackend*>(engine->getBackend())) {
      be->unbindColorTarget();
    }
#endif
    frameOpen = false;
    return true;
  }

}
