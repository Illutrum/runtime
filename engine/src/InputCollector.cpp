#include <engine/InputCollector.hpp>

namespace engine {

  void InputCollector::beginFrame() {
    current_.keysJustPressed.reset();
    current_.mouseDelta = glm::vec2(0.0f);
    current_.resetRequested = false;
  }

  void InputCollector::keyEvent(int key, int action) {
    if (key < 0 || key >= KEY_COUNT) return;
    if (action == 1) {
      if (!current_.keys.test(key)) {
        current_.keysJustPressed.set(key);
      }
      current_.keys.set(key);
      if (key == 82) {
        current_.resetRequested = true;
      }
    } else if (action == 0) {
      current_.keys.reset(key);
    }
  }

  void InputCollector::mouseMove(double x, double y) {
    glm::vec2 pos(static_cast<float>(x), static_cast<float>(y));
    if (hasMousePos_) {
      current_.mouseDelta = pos - lastMousePos_;
    }
    lastMousePos_ = pos;
    current_.mousePos = pos;
    hasMousePos_ = true;
  }

  void InputCollector::mouseButton(int button, int action) {
    if (button < 0 || button >= KEY_COUNT) return;
    if (action == 1) {
      current_.keysJustPressed.set(button);
      current_.keys.set(button);
    } else if (action == 0) {
      current_.keys.reset(button);
    }
  }

}
