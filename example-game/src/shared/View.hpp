#ifndef EXAMPLE_GAME_VIEW_HPP
#define EXAMPLE_GAME_VIEW_HPP

class View {
public:
  virtual ~View() {}
  virtual void init() = 0;
  virtual void render(const void* snapshot, float delta) = 0;
};

#endif
