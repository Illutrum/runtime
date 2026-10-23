# Kontekst — obecny stack i co mamy

Opis obecnego stanu `game-dev/` i pokrewnego infrastruktury w repo, z perspektywy wymagań natywnego hotswap kodu gry. Hotswap potrzebny **tylko natywnie (GLFW/Linux = dev path)**; web (Emscripten) jest targetem shipped bez hotswap. Wszystkie ścieżki względem repo root.

---

## fgfx — renderer

[game-dev/fgfx/](game-dev/fgfx/), docs: [fgfx/docs/architecture.md](game-dev/fgfx/docs/architecture.md).

**Platformy:**
- **Web (Emscripten + WebGL)** — primary shipped target
- **Native Linux desktop (GLFW + GLEW + libpng)** — dev/test path — **tu chcemy hotswap**

**Lifecycle:**

```39:60:game-dev/fgfx/src/fastgfx.cpp
  void destroy() {
#ifdef __USE_GLFW
    glfwTerminate();
#endif
#ifndef EMSCRIPTEN
    finished = true;
    if(loaderThread.joinable()) loaderThread.join();
    render::io::destroy();
#endif
  }

  void init() {
#ifndef EMSCRIPTEN
    finished = false;
    render::io::init();
#endif
#ifdef __USE_GLFW
    // glfw init, window, glewInit, engine = make_shared<Engine>
```

Global engine pointer + named resource maps:

```36:38:game-dev/fgfx/include/fastgfx.hpp
  void init();
  void destroy();
  extern std::shared_ptr<Engine> engine;
```

```31:36:game-dev/fgfx/src/Engine.hpp
    std::map<std::string, std::shared_ptr<Sprite>> sprites;
    std::map<std::string, std::shared_ptr<SpriteFont>> spriteFonts;
    std::map<std::string, std::shared_ptr<MSDFFont>> msdfFonts;
    std::map<std::string, std::shared_ptr<Texture>> textures;
```

**Build native:** static lib `fgfx` + executable linkujący `GL png z glfw GLEW` (patrz [fgfx/CMakeLists.txt](game-dev/fgfx/CMakeLists.txt)). To jest dobre podłoże pod `dlopen` host/plugin — engine może być statycznie zlinkowany w hoście, kod gry w osobnym `.so`.

**Co musi przetrwać reload (engine/host):** `render::engine` + resource maps, GL context + shader programs (init w `Engine` ctor), `render::io` thread pool, GLFW window.
**Co jest reloadable (game):** `main()`, demo/game structs, render lambda body, entity state, game logic.

**Web C API (stable boundary):**

```129:134:game-dev/fgfx/src/fastgfx.cpp
FGFX_API
void fgfx_render(double time, float delta, int widthp, int heightp, double rw, double rh) {
  ...
  render::engine->render(time, delta, widthp, heightp);
```

---

## asset-manager — persistent service

[game-dev/asset-manager/](game-dev/asset-manager/), [README](game-dev/asset-manager/README.md).

**Kontrakt hot-reload już udokumentowany** (wprost wymienia `dlopen`):

```22:32:game-dev/asset-manager/README.md
## Hot-reload boundary

**Code hot-reload is NOT part of this library.** The asset manager is intended to be a **persistent service** that survives dev-time code reload (`dlopen` / wasm module swap via `InrementalCpp`).

Contract for game code across reload:

1. Access assets only through **name-based** `AssetRequest` handles from `AssetsManager::manager`.
2. Do **not** store raw payload pointers (`Texture*`, `MSDFFont*`, decoded buffers) across reload boundaries.
3. After reload, re-request assets by path — cached assets resolve immediately if still within grace period or still referenced by other systems.

Assets survive reload; code does not. The manager outlives the game module.
```

**Lifetime model:** `manager.request<T>(path)` → `shared_ptr<AssetRequestT<T>>`. Po drop ostatniego request → eviction po `graceFrames` (default 120). Dependencies = strong `shared_ptr<Asset>`.

Global singleton:

```5:5:game-dev/asset-manager/src/AssetsManager.cpp
  AssetsManager manager;
```

```36:64:game-dev/asset-manager/src/AssetsManager.cpp
  void AssetsManager::onRequestDied(const AssetId& assetKey) {
    pendingEviction.push_back({assetKey, currentFrame});
  }
  ...
  void AssetsManager::update(long frame) {
    ...
      if (currentFrame - it->frame >= graceFrames) {
        assets.erase(it->assetKey);
```

**Warstwa fgfx** trzyma `shared_ptr<render::Texture>` wewnątrz assetów — OK, bo persistent service. **Kod gry** nie może trzymać raw payloadów przez reload.

```10:12:game-dev/asset-manager/fgfx/TextureAsset.hpp
  class TextureAsset : public TypedAsset<TextureAsset> {
  public:
    std::shared_ptr<render::Texture> texture;
```

**Tension:** `loading_screen_example.cpp` łamie kontrakt (trzyma `logoSprite`, layers, `fontAsset->font` w `DemoState`) — fine dla demo, ale design reload musi to traktować jako non-surviving.

**Dla hotswap:** `manager` singleton żyje w hoście (engine exe). Kod gry re-requestuje assety po reload → cache hit (grace window) → natychmiast. To jest **gotowe podłoże**, nie trzeba budować.

---

## cpp-rpc serialization — p2996 reflection

`cpp-rpc/src/serialization/` — 23 nagłówki, **P2996 reflection-based** binary serialization + RPC + type_registry. To jest serce repo (stąd nazwa "waos-serialization").

**Kluczowe pliki:**

| Plik | Rola |
|------|------|
| `serialize.hpp` / `deserialize.hpp` | compile-time walk po `std::meta::nonstatic_data_members_of` |
| `type_description.hpp` | runtime type graph (Object, Message, Enum, Array, Method) |
| `type_registry.hpp` | name/index registry, serializable as blob |
| `annotations.hpp` | `[[=serialization::dynamic()]]`, `message()`, `policy::ignore`, `read()` |
| `promise.hpp` | async promises + coroutine |
| `rpc.hpp`, `rpc_server.hpp`, `rpc_client.hpp` | request/response protocol |
| `stream_reader.hpp` / `stream_writer.hpp` | I/O backends |
| `circular_buffer_*` | in-memory buffers |

**Reflection walk** (wymaga clang-p2996):

```145:163:cpp-rpc/src/serialization/serialize.hpp
  void serialize_members(W& writer, const T& value) {
    [:expand(std::meta::bases_of(^^B, std::meta::access_context::unchecked())):] >> [&]<std::meta::info base> {
      serialize_members<T, W, typename [:std::meta::type_of(base):]>(writer, value);
    };
    [:expand(std::meta::nonstatic_data_members_of(^^B, std::meta::access_context::unchecked())):] >> [&]<auto dm> {
      if constexpr(!is_member_ignored(dm)) {
        serialize(writer, value.[:dm:]);        
      }
    };    
  }
```

`meta_expand.hpp` dokumentuje workaround dla expansion-statements (P2996 back-and-forth).

**Type registry** buduje schema compile-time, rejestruje RPC methods gdy typy expose `get_rpc_object` / `get_rpc_instance`:

```447:467:cpp-rpc/src/serialization/type_description.hpp
    requires(UserDefinedType<T> && !String<T> && !ContainerButNotString<T> && !Map<T> && !Pointer<T> && !SimpleException<T>)
    std::shared_ptr<type_description> static create(T value, R& registry) {
      ...
      type = std::make_shared<type_description>(std::string(name), 
        is_message(^^T) 
          ? type_description::Kind::Message 
```

**Wasm module export pattern** (standalone wasm):

```13:28:cpp-rpc/src/utils/module.cpp
extern "C" size_t rpc_description(std::uint32_t socket, char** buffer) {
  ...
  serialize(writer, registry);
  ...
  return output_stream.str().size();
}
```

**Dla hotswap (natywnego):** to jest naturalny kandydat do **migracji stanu gry przez reload**:
- GameState = struct z `[[=serialization::message()]]` annotation
- Reload = `serialize(state)` → `dlclose` → `dlopen` → `game_init` → `deserialize(state)`
- type_registry = schema versioning (hash → detekcja layout change)
- Pointer graphs via `write_pointer`/`read_pointer` z deduplikacją — suitable dla object graphs, nie opaque GL handles

**Stan dziś:** game-dev **nie używa p2996** — tylko consume `serialization::promise` (header-only, no reflection w game code). Włączenie p2996 = zmiana toolchainu (patrz cpp-compiler).

---

## cpp-compiler — clang p2996 fork

`cpp-compiler/` — **nie** prebuilt w git; Docker build + sysroot export.

| Plik | Rola |
|------|------|
| `Dockerfile` | builds [bloomberg/clang-p2996](https://github.com/bloomberg/clang-p2996) @ `p2996`, libc++, Binaryen, Node, Emscripten z p2996 libc++ |
| `build-and-export-sysroot.sh` | `docker build` → `docker run` export |
| `export-sysroot.sh` | kopiuje `/usr`, `/lib`, emscripten, tworzy `emcc-wrapper.sh` / `em++-wrapper.sh` |
| `.gitignore` | ignores `sysroot/` |

**Native toolchain** (`p2996-toolchain.cmake`):

```6:24:p2996-toolchain.cmake
set(COMPILER_SYSROOT "${CMAKE_SOURCE_DIR}/cpp-compiler/sysroot")
set(CMAKE_C_COMPILER "${COMPILER_SYSROOT}/usr/local/bin/clang")
set(CMAKE_CXX_COMPILER "${COMPILER_SYSROOT}/usr/local/bin/clang++")
...
set(CMAKE_CXX_FLAGS_INIT "-std=c++26 -freflection -fparameter-reflection -fannotation-attributes -fexpansion-statements -nostdinc++ ...")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${LIBCXX_LIB_DIR} ... -lc++ -lc++abi -lunwind -stdlib=libc++")
```

**Dla hotswap:** to jest kompilator do **runtime-owej kompilacji pluginu gry**. Invoke: `cpp-compiler/sysroot/usr/local/bin/clang++` + flagi p2996 + sysroot includes/libs. Z clang header modules → ~1s jak CppCad2. game-dev dziś używa stock clang (nie p2996) — włączenie p2996 dla `game.so` to zmiana build config, nie fork.

---

## module-debugger — dev inspection tool

`module-debugger/` — Vue 3 app do inspect/call RPC na standalone wasm modules. Core loader:

```12:40:module-debugger/src/module.ts
  constructor(wasm: WebAssembly.Module) {
    this.wasm = wasm;
    this.memory = new WebAssembly.Memory({ initial: 1024, maximum: 2048 });    
  }
  
  async instantiate() {
    const imports = {
      env: { memory: this.memory },
      waos: { waos_socket_flush: (socket) => { this.handleSocketFlush(socket) } }
    }    
    this.instance = await WebAssembly.instantiate(this.wasm, imports);
    this.instance.exports._initialize();
    this.instance.exports.start();
    ...
    this.rpcDescription = deserializeTypeRegistry(this.readRawRpcDescription());
```

**Dla hotswap (natywnego, opcjonalnie):** ten sam `rpc_description`/type_registry mechanizm można użyć do **inspekcji GameState i callowania API gry w trakcie dev** — live struct editing, wywoływanie update z parametrami, podgląd wartości. Nie web-only — pattern jest reusable native. Opcjonalny dev UI, nie wymagany.

---

## ts-kernel / wasm tooling — out of scope

`ts-kernel/`, `ts-rpc/`, `kernel-shield/`, `wasm-determinism/` — producyjny wasm module lifecycle, `save`/`resurect` = raw memory snapshot (`get_memory()` → `setMemory()`), shared `WebAssembly.Memory`, `STANDALONE_WASM`.

**Out of scope dla natywnego hotswap.** Wspomniane tu dla kompletności: to jest web-side tooling, niepotrzebny gdy hotswap jest tylko natywny. Gdyby kiedyś web hotswap wszedł, to jest infrastruktura — ale nie teraz.

---

## Podsumowanie: co mamy vs czego brakuje

| Komponent | Stan | Relevance |
|-----------|------|-----------|
| fgfx native build (static lib + exe) | gotowe | podłoże pod dlopen host |
| fgfx `init/destroy` + `engine` singleton | gotowe | persistent host state |
| asset-manager persistent service + kontrakt | **gotowe + udokumentowane** | nie trzeba budować |
| cpp-rpc p2996 serialization | gotowe, **nie używane w game-dev** | migracja stanu gry |
| cpp-compiler clang p2996 | gotowe (sysroot gitignored) | runtime compile pluginu |
| module-debugger pattern | gotowe (web) | opcjonalny dev UI |
| dlopen loader / InrementalCpp | **nie implementowane w game-dev** | do zbudowania |
| clang header modules dla fgfx headers | **nie** | do zbudowania (PCM cache) |
| Split engine/game w build | **nie** (monolityczny exe dziś) | do zbudowania |
| p2996 toolchain w game-dev | **nie** (stock clang) | do włączenia dla `game.so` |
| GameState schema + serialization | **nie** | do zdefiniowania |

**Kluczowa obserwacja:** dużo już jest. asset-manager kontrakt jest gotowy i udokumentowany. cpp-rpc + cpp-compiler są gotowe. Główna praca = split engine/game native + clang header modules + dlopen loader + GameState serialization. Detale → `propozycje-rozwiazan.md`.
