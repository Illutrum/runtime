#include "View.hpp"
#include "GameState.hpp"
#include <fastgfx.hpp>

class StubView : public View {
public:
  void init() override {}
  void render(const void* snapshot, float delta) override {
    (void)snapshot;
    (void)delta;
    glClearColor(0.08f, 0.1f, 0.14f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
  }
};

extern "C" {
  void* view_create() { return new StubView(); }
  void view_destroy(void* instance) { delete static_cast<StubView*>(static_cast<View*>(instance)); }
  void view_init(void* instance) { static_cast<View*>(instance)->init(); }
}
