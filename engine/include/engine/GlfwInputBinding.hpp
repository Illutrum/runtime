#ifndef ENGINE_GLFW_INPUT_BINDING_HPP
#define ENGINE_GLFW_INPUT_BINDING_HPP

namespace engine {
  class GameHost;

#ifdef __USE_GLFW
  void bindGlfwInput(GameHost* host);
#else
  inline void bindGlfwInput(GameHost*) {}
#endif
}

#endif
