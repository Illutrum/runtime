#include <fgfx/thorvg.hpp>

#include "Texture.hpp"
#include <fastgfx.hpp>
#include <thorvg.h>

#ifdef EMSCRIPTEN
#include <emscripten/html5_webgl.h>
#endif

namespace fgfx::thorvg {
namespace {

  tvg::GlCanvas* canvas = nullptr;
  bool initialized = false;
  GLuint stencilRbo = 0;
  void* glContext = nullptr;
  Texture* frameTex = nullptr;
#ifndef EMSCRIPTEN
  void (*clearColorFn)(float, float, float, float) = nullptr;
  void (*clearFn)(unsigned int) = nullptr;
#endif

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

  void attachStencil(Texture& tex) {
    if (stencilRbo == 0) {
      glGenRenderbuffers(1, &stencilRbo);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, stencilRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, tex.width, tex.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRbo);
  }

  bool bindTarget(Texture& tex) {
    tvg::Result result = canvas->target(
      nullptr,
      nullptr,
      glContext,
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
      glContext,
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

  void abortFrame() {
    if (frameTex) {
      frameTex->endRender(false);
      frameTex = nullptr;
    }
  }

  void term() {
    if (!initialized) {
      return;
    }
    abortFrame();
    delete canvas;
    canvas = nullptr;
    glContext = nullptr;
#ifndef EMSCRIPTEN
    clearColorFn = nullptr;
    clearFn = nullptr;
#endif
    if (stencilRbo != 0) {
      glDeleteRenderbuffers(1, &stencilRbo);
      stencilRbo = 0;
    }
    tvg::Initializer::term();
    initialized = false;
  }

  bool ensureInitialized() {
    if (initialized) {
      return canvas != nullptr;
    }

    tvg::Result result = tvg::Initializer::init(0);
    if (result != tvg::Result::Success) {
      fgfx_log("ThorVG Initializer::init failed (%s)", resultName(result));
      return false;
    }
    canvas = tvg::GlCanvas::gen();
    if (!canvas) {
      fgfx_log("ThorVG GlCanvas::gen failed");
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
      fgfx_log("ThorVG missing current GL context");
      delete canvas;
      canvas = nullptr;
      tvg::Initializer::term();
      return false;
    }
#ifndef EMSCRIPTEN
    if (!clearColorFn || !clearFn) {
      fgfx_log("ThorVG missing glClear/glClearColor");
      delete canvas;
      canvas = nullptr;
      glContext = nullptr;
      tvg::Initializer::term();
      return false;
    }
#endif

    initialized = true;
    fgfx::addShutdownHook(&term);
    fgfx_log("ThorVG initialized (lazy)");
    return true;
  }

}

  bool begin(Texture& tex) {
    if (!ensureInitialized()) {
      return false;
    }
    if (frameTex) {
      fgfx_log("ThorVG begin skipped: frame already open");
      return false;
    }

    tex.beginRender();
    if (!bindTarget(tex)) {
      tex.endRender(false);
      return false;
    }

    clearRgba(0.0f, 0.0f, 0.0f, 0.0f);
    frameTex = &tex;
    return true;
  }

  void addFilledCircle(float cx, float cy, float rx, float ry,
                       uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (!frameTex || !canvas) {
      fgfx_log("ThorVG addFilledCircle skipped: no open frame");
      return;
    }
    tvg::Shape* circle = tvg::Shape::gen();
    circle->appendCircle(cx, cy, rx, ry);
    circle->fill(r, g, b, a);
    tvg::Result added = canvas->add(circle);
    if (added != tvg::Result::Success) {
      fgfx_log("ThorVG canvas->add failed (%s)", resultName(added));
      tvg::Paint::rel(circle);
    }
  }

  bool submit() {
    if (!frameTex || !canvas) {
      fgfx_log("ThorVG submit skipped: no open frame");
      return false;
    }

    tvg::Result updated = canvas->update();
    if (updated != tvg::Result::Success) {
      fgfx_log("ThorVG update failed (%s)", resultName(updated));
      abortFrame();
      return false;
    }
    tvg::Result drawn = canvas->draw(false);
    if (drawn != tvg::Result::Success) {
      fgfx_log("ThorVG draw failed (%s)", resultName(drawn));
      abortFrame();
      return false;
    }
    tvg::Result synced = canvas->sync();
    if (synced != tvg::Result::Success) {
      fgfx_log("ThorVG sync failed (%s)", resultName(synced));
      abortFrame();
      return false;
    }

    frameTex->endRender(false);
    frameTex = nullptr;
    return true;
  }

}
