# Hotswap (native dev)

Native-only hot-reload infrastructure for the Engine / Model / View architecture.

## Components

- `Manifest` / `ManifestWriter` — atomic `hotswap.manifest` with sha256 checksums
- `BuildOrchestrator` — efsw + incremental cmake/ninja rebuild
- `ReloadManager` / `ManifestWatcher` — compare checksums, reload model+view or view-only
- `ModelModuleAsset` / `ViewModuleAsset` — dlopen through asset-manager
- `AssetWatcher` — reload changed asset files without recompiling code

Incremental plugin rebuilds use Clang header modules and `ninja -C` on only the changed target. See [docs/clang-header-modules.md](../../docs/clang-header-modules.md).

## Manual dev checklist

Run from build dir: `cd build/examples && ./example_game`

1. **View-only reload** — change color in `ExampleView.cpp`, wait ~1s, model counter unchanged
2. **Model reload (schema match)** — change speed constant, state preserved after reload
3. **Model reload (schema differ)** — add field to `GameState.hpp`, model resets
4. **Asset reload** — replace `assets/logo.png`, texture refreshes without rebuild
5. **Shared header change** — edit `GameState.hpp`, manifest updates atomically with both `.so`
6. **Failed build** — introduce syntax error, old version keeps running

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target example_game test_hotswap test_engine
ctest --test-dir build
```

Requires p2996 sysroot at `cpp-compiler/sysroot` for `model.so` / `view.so`.
