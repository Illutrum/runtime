#ifndef FGFX_THORVG_HPP
#define FGFX_THORVG_HPP

#include <cstdint>

namespace fgfx {
  class Texture;

  namespace thorvg {
    bool begin(Texture& tex);
    void addFilledCircle(float cx, float cy, float rx, float ry,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    bool submit();
  }
}

#endif
