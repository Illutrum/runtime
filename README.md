# Runtime

The window, the input, and the frame for Illutrum. This is not a second product beside the engine.

Repository: [Illutrum/runtime](https://github.com/Illutrum/runtime).

## Host

`engine::GameHost` opens the window through `render::init`. The shared library `engine_runtime` contains [Illutrum/render](https://github.com/Illutrum/render) and [Illutrum/assets](https://github.com/Illutrum/assets) at `v0.1.0`. `engine_runtime_thorvg` is the SVG raster beside it.

## Input

`engine::InputCollector` stores the keys and the pointer. On native, `GlfwInputBinding` writes GLFW events into that collector. The game reads `RawInput` once per frame and builds its own serializable input.

## Frame

`GameHost::run` is the frame. The host calls the frame callback with a `render::FrameBuilder`, the delta, and the raw input. A view records passes. It does not own the window.

## How a view is loaded

On native, hotswap is on. The host `dlopen`s `model.so` and `view.so` from the hotswap manifest. Dropping the last asset request starts the grace period in `asset_manager`. The view is loaded again by path. Do not keep raw `render::Texture` or `render::MSDFFont` pointers across that reload.

On the browser build there is no `dlopen`. The model and the view are linked into the wasm module. Hotswap is forced off. The wasm exports keep the names `_fgfx_render` and `_fgfx_io_fileLoaded`. Those are the C ABI of the render library.

## Build

```bash
git submodule update --init
cmake -B build
cmake --build build --target engine_runtime engine_runtime_thorvg example_game example_visualization test_engine test_hotswap
ctest --test-dir build -R 'EngineTests|HotswapTests' --output-on-failure
```

Native OpenGL is the build this repository tests. WebGPU and Emscripten are not that gate. The example executables open a window. `ctest` does not run them.

`template/` is a scaffold. Copy it. Do not add it beside `example-game`.

## License

MIT. Copyright (c) 2026 Magowie Sp. z o.o. ThorVG and efsw are named in `NOTICE`.
