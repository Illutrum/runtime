#ifndef FGFX_THORVG_HPP
#define FGFX_THORVG_HPP

#include <cstdint>
#ifdef FGFX_BACKEND_WEBGPU
#include "backend/BackendTypes.hpp"
#endif

namespace fgfx {
  class Texture;

  namespace thorvg {
#ifdef FGFX_BACKEND_WEBGPU
    bool begin(TextureHandle tex, int width, int height);
#else
    bool begin(Texture& tex);
#endif
    void addFilledCircle(float cx, float cy, float rx, float ry,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    bool submit();
  }
}

#endif
