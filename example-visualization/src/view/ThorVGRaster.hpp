#ifndef EXAMPLE_VISUALIZATION_THORVG_RASTER_HPP
#define EXAMPLE_VISUALIZATION_THORVG_RASTER_HPP

namespace fgfx {
  class Texture;
}

namespace viz {

  struct ThorVGRaster {
    static void init();
    static void term();
    static bool rasterTo(fgfx::Texture& tex);
  };

}

#endif
