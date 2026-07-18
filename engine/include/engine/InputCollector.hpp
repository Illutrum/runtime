#ifndef ENGINE_INPUT_COLLECTOR_HPP
#define ENGINE_INPUT_COLLECTOR_HPP

#include <bitset>
#include <glm/glm.hpp>

namespace engine {

  constexpr int KEY_COUNT = 512;

  struct RawInput {
    std::bitset<KEY_COUNT> keys;
    std::bitset<KEY_COUNT> keysJustPressed;
    glm::vec2 mousePos{0.0f};
    glm::vec2 mouseDelta{0.0f};
    bool resetRequested = false;
  };

  class InputCollector {
  public:
    void beginFrame();
    void keyEvent(int key, int action);
    void mouseMove(double x, double y);
    void mouseButton(int button, int action);
    const RawInput& current() const { return current_; }

  private:
    RawInput current_;
    glm::vec2 lastMousePos_{0.0f};
    bool hasMousePos_ = false;
  };

}

#endif
