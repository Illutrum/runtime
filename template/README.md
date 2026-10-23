# Template

Scaffold for a host, a model, and a view.

1. Copy this directory.
2. `add_subdirectory` the copy from your own list. Do not add it beside `example-game` in this repository. Both declare targets named `model` and `view`.
3. Implement `StubModel` and `StubView`.
4. Keep `Input` serializable.

Rules:

- Do not define asset types inside a reloadable `.so`.
- Do not hold an `AssetRequest` across reload. Request it again in `init()`.
- A view must not cache a snapshot pointer past the frame.
- The host collects raw input. The game builds a serializable `Input`.
