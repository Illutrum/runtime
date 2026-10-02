#ifndef ENGINE_WASM_INPUT_BINDING_HPP
#define ENGINE_WASM_INPUT_BINDING_HPP

namespace engine {
  class GameHost;

#ifdef EMSCRIPTEN
  // runtime.js calls fgfx_key_event / fgfx_mouse_* into the same InputCollector.
  void bindWasmInput(GameHost* host);
#else
  inline void bindWasmInput(GameHost*) {}
#endif
}

#endif
