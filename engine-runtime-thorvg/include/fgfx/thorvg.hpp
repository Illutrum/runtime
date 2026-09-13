#ifndef FGFX_THORVG_HPP
#define FGFX_THORVG_HPP

#include <cstdint>
#include <functional>
#include "backend/BackendTypes.hpp"

namespace fgfx {
  struct RenderPass;

  namespace thorvg {
    bool begin(TextureHandle tex, int width, int height);
    void record(RenderPass& pass, TextureHandle tex, int width, int height,
                std::function<void()> buildShapes);
    void addFilledCircle(float cx, float cy, float rx, float ry,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    bool submit();
  }
}

#endif
