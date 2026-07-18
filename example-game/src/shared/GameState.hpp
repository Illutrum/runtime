#ifndef EXAMPLE_GAME_GAME_STATE_HPP
#define EXAMPLE_GAME_GAME_STATE_HPP

struct GameState {
  float x = 0.0f;
  float y = 0.0f;
  float speed = 120.0f;
  int counter = 0;
  float renderColorR = 0.4f;
  float renderColorG = 0.9f;
  float renderColorB = 0.5f;
};

#endif
