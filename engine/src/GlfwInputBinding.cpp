#include <engine/Host.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <fastgfx.hpp>

namespace engine {

  void bindGlfwInput(GameHost* host) {
    if (!host || !fgfx::glfwWindow) return;
    glfwSetWindowUserPointer(fgfx::glfwWindow, host);
    glfwSetKeyCallback(fgfx::glfwWindow, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
      (void)scancode;
      (void)mods;
      auto* h = static_cast<GameHost*>(glfwGetWindowUserPointer(window));
      if (h) h->input().keyEvent(key, action);
    });
    glfwSetCursorPosCallback(fgfx::glfwWindow, [](GLFWwindow* window, double x, double y) {
      auto* h = static_cast<GameHost*>(glfwGetWindowUserPointer(window));
      if (h) h->input().mouseMove(x, y);
    });
    glfwSetMouseButtonCallback(fgfx::glfwWindow, [](GLFWwindow* window, int button, int action, int mods) {
      (void)mods;
      auto* h = static_cast<GameHost*>(glfwGetWindowUserPointer(window));
      if (h) h->input().mouseButton(button, action);
    });
  }

}
