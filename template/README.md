# Template game

Scaffold for a new Engine / Model / View project.

1. `./game-dev/scripts/new-game.sh my-game`
2. Add `add_subdirectory(my-game)` to `game-dev/CMakeLists.txt`
3. Implement `StubModel` / `StubView`
4. Keep `Input` serializable for deterministic multiplayer

Rules:
- Do not define asset types inside reloadable `.so` files
- Do not hold `AssetRequest` across reload — re-request in `init()`
- View must not cache snapshot pointers past the frame
- Engine collects raw input; game builds serializable `Input`
