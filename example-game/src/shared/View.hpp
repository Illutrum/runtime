#ifndef EXAMPLE_GAME_VIEW_HPP
#define EXAMPLE_GAME_VIEW_HPP

namespace fgfx { class FrameBuilder; }

class View {
public:
  virtual ~View() {}
  virtual void init() = 0;
  // New render-graph API: the view records into a FrameBuilder.
  virtual void render(fgfx::FrameBuilder& builder, const void* snapshot, float delta) = 0;
};

#endif
