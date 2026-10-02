#ifdef EMSCRIPTEN

#include <engine/Host.hpp>
#include <engine/WasmInputBinding.hpp>

#include <emscripten.h>

namespace {

engine::GameHost* g_host = nullptr;

}

namespace engine {

void bindWasmInput(GameHost* host) {
  g_host = host;
}

}

extern "C" {

EMSCRIPTEN_KEEPALIVE
void fgfx_key_event(int key, int action) {
  if (g_host) {
    g_host->input().keyEvent(key, action);
  }
}

EMSCRIPTEN_KEEPALIVE
void fgfx_mouse_move(double x, double y) {
  if (g_host) {
    g_host->input().mouseMove(x, y);
  }
}

EMSCRIPTEN_KEEPALIVE
void fgfx_mouse_button(int button, int action) {
  if (g_host) {
    g_host->input().mouseButton(button, action);
  }
}

}

#endif
