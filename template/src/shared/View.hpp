#ifndef TEMPLATE_VIEW_HPP
#define TEMPLATE_VIEW_HPP

namespace render { class FrameBuilder; }

class View {
public:
  virtual ~View() {}
  virtual void init() = 0;
  virtual void render(render::FrameBuilder& builder, const void* snapshot, float delta) = 0;
};

#endif
