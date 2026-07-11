# Historia — iteracyjny development gier w C++

Narracja chronologiczna prób rozwiązania problemu szybkiej iteracji nad kodem gry w C++. Każda era z konkretnymi plikami i cytatami. Wszystkie ścieżki względem repo `/home/m8/IdeaProjects/waos-serialization/`.

---

## Era 1 — Fox2d + AngelScript (scripting language)

Pierwsza próba: język skryptowy jako warstwa logiki gry ponad silnikiem C++.

**Wybór:** AngelScript — vendored w `game-dev/iterative/Game/deps/Fox2d/lib/angelscript/` (~65 000 LOC engine source). Wybrany bo ma syntax podobny do C++ i statyczne typy. Włączony opcjonalnie makrem `USE_SCRIPTS` (`deps/Fox2d/CMakeLists.txt` linie 71–100); web build (`compile-web.sh`) go nie definiuje.

**Warstwa binding:** manualna, template-based, **bez refleksji**.

- `deps/Fox2d/src/Fox2D/bind/AngelScriptBinder.h` (~750 LOC) — `globalFunction`, `clazz<T>`, `objectMethod`, `objectConstructor`, `enumeration`, `value`
- `AS_MAX_PORTABILITY` zawsze włączone → `asCALL_GENERIC` + druga warstwa `generic_*_caller` (~podwaja złożoność)
- Każdy argument przechodzi przez `proxy<T>` na stosie:

```27:37:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/bind/AngelScriptBinder.h
    template <typename T> class proxy {
    public:
      T value;
      proxy(T value) : value(value) {}
      static T cast(void * ptr) {
        return reinterpret_cast<proxy<T> *>(&ptr)->value;
      }
```

**Pipeline assetów skryptów:** `AngelScriptAsset` ładuje `Code` asset, preprocessuje (własny mini-preprocessor `#include`/`#import`/`#asset`/`#ifdef`), kompiluje w `asIScriptModule`:

```32:49:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/assets/script/AngelScriptAsset.cpp
    void AngelScriptAsset::startLoadingSelf() {
      try {
        Code::PreprocessorSettings preprocessorSettings;
        preprocessorSettings.processIfDefs = true;
        code->preprocess(preprocessorSettings);
        addCodeToModule(code);
        int r = module->Build();
        if(r< 0) throw AngelScriptCompilerException();
        handleLoadingFinished();
```

**Wynik:** **zero plików `.as` w `assets/`**. Cała infrastruktura zbudowana, nigdy nie przyjęta w treści gry. Cała zarejestrowana API w `AssetsManager::registerBindings()`:

```71:80:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/assets/AssetsManager.cpp
    void AssetsManager::registerBindings() {
      bind::clazz<Asset>::bindNamespaced("assets", "Asset");
      getFunction getFunc = [this](std::string type, std::string name){
        return getAsset(type, name);
      };
      getFunctionBound = std::make_shared<bind::lambdaFunction<getFunction>>(getFunc);
      getFunctionBound->bindNamespaced("assets", "get");
    }
```

Dwa bindingi. 65k LOC + 1.4k LOC glue dla dwóch bindingów.

**Era 1b — alternatywne backendy:** `LuaBinder.h` = pusta klasa (nigdy nieimplementowana), `DoubleBinder.h` = skelet do rejestracji tej samej API do dwóch binderów naraz. Eksperyment porzucony.

### Dlaczego era 1 zawiodła

| Objaw | Dowód |
|-------|-------|
| Koszt >> korzyść | ~65k LOC AngelScript + ~1.4k LOC binder = 0 skryptów |
| Każda metoda C++ do wyprowadzenia manualnie | brak refleksji, template registration |
| Maintenance burden | 750-loc template header, `AS_MAX_PORTABILITY`, `proxy<T>` |
| Type-safety lost na boundary | `assert(r >= 0)`, runtime null-pointer exceptions |
| Build complexity | opcjonalny `USE_SCRIPTS` ciągnie cały AngelScript |
| Preprocessor na preprocessorze | `Code` dodaje `#include`/`#ifdef` przed kompilacją AS |

---

## Era 2 — `.fox` config DSL (configuration)

Druga próba: zamiast skryptów, data-driven konfiguracja. Custom format `.fox`, parsowana własnym lexerem.

**Format:** custom DSL, **nie** JSON/XML/YAML. Lexer w `deps/Fox2d/src/Fox2D/config/Value.cpp` (~560 LOC). Model wartości w `Value.h`:

| Typ | Znaczenie |
|------|-----------|
| `Number`, `String` | prymitywy |
| `Struct` | `{ key = value ... }` anonimowa |
| `Object` | `ClassName { props }` typowane |
| `Array` / `ArrayObject` | `[ ... ]` |
| `Tuple` / `TupleObject` | `( a, b, c )` — call-like komendy |
| `Symbol` | bare identifier (placeholder referencji) |

**Konsumpcja:** `config::Reader` (typed `property()`/`propertyArray()`), type registries dispatch po nazwie klasy (`readDrawable()`, `ParticleGenerator::read()`, `CommandsMap::runCommand()`).

**Jak config urósł w pseudojęzyk:**

Transform = sekwencyjna lista komend interpretowana w C++:

```1:10:game-dev/iterative/Game/assets/view/buttBoilerTank.fox
hull = Drawable [
  PBRShaderSprite {
    layer = "tankBack"
    sprite = Asset("sprite", "sprites/buttBoilerTank/zebatka.png")
    shader = Asset("shader", "view/buttBoilerTank")
    transform = Transform [
      translate(1.23, 0.432, 0)
      scale(0.50, 0.50, 1)
```

```12:27:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/config/structures.cpp
    glm::mat4 readTransform(const Value &from) {
      glm::mat4 mat;
      for (auto partp : from.elements) {
        auto& part = *partp;
        if (part.clazz == "scale") {
          glm::vec3 vec = part[0].clazz == "Vec3" ? readVec3(part[0]) : readVec3(part);
          mat = glm::scale(mat, vec);
        } else if (part.clazz == "translate") {
```

Trailer/cinematic = komendy w KeyFrame:

```6:17:game-dev/iterative/Game/assets/trailer/Teaser1.fox
scenario = [
  Scene {
    name = "tank_start"
    frames = [
      KeyFrame(0.8, [
        startNewGame()
        setCamera(Vec2(0, 0), 40)
        spawnTank("tank1", "buttBoiler", Vec2(0, -2))
      ])
      KeyFrame(0.23, [
        fire("tank1")
```

Każda komenda = hand-written lambda w C++:

```183:210:game-dev/iterative/Game/src/model/Game.cpp
      commandsMap.setCommand("spawnTank", [this](const fox2d::config::Value& command) {
        std::string name = command[0];
        std::string tankModel = command[1];
        auto position = fox2d::config::readB2vec2(command[2]);
        auto tank = std::make_shared<FoxTank>(position, tankModel, this);
        addObject(tank->getGameObject());
```

Particle composition = `@`-chain mapowane na pre-rejestrowane C++ templates:

```1:7:game-dev/iterative/Game/assets/fx/flameParticle.fx.fox
events = [
  TimePoint{ t = 0.0, commands = [
    EmitParticles {
      count = 30
      layer = "fire"
      particle = Procedural@StaticParameter@LinearColor@StaticAngle@LinearSize@DynamicPosition@RadialExplosion@Base {
```

`particleTypes.cpp` rejestruje **64 explicit `Particle<Procedural<...>>::registerType()` kombinacje** — każda musi istnieć w C++ zanim config ją zreferuje. Dodanie nowego modułu = zmiana w C++ **i** w config name matching.

Asset references jako expressions: `Asset("type", "name")` tuple traversowane automatycznie:

```31:37:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/assets/ConfigAsset.cpp
          config->traverse([this](config::Value& v){
            if(v.type == config::Value::Type::TupleObject && v.clazz == "Asset") {
              addDependency(manager.getAsset(*v.elements[0], *v.elements[1]));
              return false;
            }
            return true;
          });
```

`symbolicLink` pole istnieje w całym `Value` ale **nigdy nie ustawiane przez parser** — referencje/zmienne zaplanowane, nieimplementowane.

### Dlaczego era 2 zawiodła

| Objaw | Dowód |
|-------|-------|
| Custom parser do utrzymania | hand-rolled lexer, `/// TODO: escape sequences` |
| Brak schema/walidacji | runtime exceptions z debug dumpami |
| Schema drift | `FoxTank.cpp` ~40 manualnych czytań pól, `PBRSprite.cpp` czyta inne klucze |
| Combinatorial explosion | 64 pre-rejestrowane particle combos |
| Pseudojęzyk bez toolingu | brak LSP, syntax highlight, static analysis dla `.fox` |
| Każda komenda = C++ registry | `spawnTank`, `fire`, `setCamera`, `EmitParticles` — każdy własna lambda |
| Duplikacja | `model/buttBoilerTank.fox` + `view/buttBoilerTank.fox` osobno |
| Config cache blokuje iterację | `FoxTank::loadedConfigurations` static map, reload nie invaliduje |

---

## Era 2b — hot-reload assetów w Fox2d

Fox2d miał hot-reload **assetów** (nie kodu), ale tylko native i niespójnie.

**File watcher:** efsw na `assets/`, makro `__LIVE_RELOAD` (OSX/Linux, nie web):

```41:52:game-dev/iterative/Game/deps/Fox2d/src/Fox2D/assets/watcher/AssetsFileWatcher.cpp
        case efsw::Action::Modified:
          // ...
            if(it != assetsByPath.end()) {
              for(Asset* asset : it->second) {
                fox_log("FW Reloading Asset %s", asset->name.c_str());
                asset->startReload();
              }
            }
```

**View-side granular reload (działał):** observer na config assetu re-czyta drawable:

```30:39:game-dev/iterative/Game/src/view/FoxTank.cpp
        config = std::dynamic_pointer_cast<fox2d::assets::ConfigAsset>(
            fox2d::assets::manager.getAsset("fox", "view/" + model + "Tank.fox"));
        configObserverId = config->apply([this](auto asset) {
          auto& data = *(config->config);
          hullDrawable = fox2d::view::readDrawable(renderer, data["hull"]);
```

**Model-side crude:** observer inkrementuje licznik → main loop **niszczy cały `ViewManager` i recreates `ModelManager`**:

```119:124:game-dev/iterative/Game/src/game/common.cpp
    if(modelReloadCounter > modelReloadedCounter) {
      viewManager = nullptr;
      modelManager = std::make_shared<tanks::ModelManager>();
      fox2d::ui::manager->root = preloaderUi;
      modelReloadedCounter++;
    }
```

`modelHotswappableLoadTask` = pusty (`model.cpp`). Pełny reset sesji, nie in-place swap. Physics body nie hot-swappable (cache konfigów w konstruktorze). Brak na web.

---

## Era 3 — szybka rekompilacja (CppCad2)

Trzecia próba, najudana: zamiast skryptów albo configu — **szybka rekompilacja samego C++**. Projekt `game-dev/iterative/CppCad2/`. Cel: design fizycznych przedmiotów kodem C++ z OpenCascade, ~1s iteracji.

**Architektura splitu host/plugin:**

| Layer | Role | Rekompilowany? |
|-------|------|----------------|
| User project (`projects/<name>/src/`) | CAD logic → `lib<name>.so` | **Tak** (~1s) |
| Model API (`model/`) | `Project`, `Part`, `Shape`, `modelScript.h` | Nie (headers w plugin) |
| Host binaries (`app`, `view`, `viewer`, `compiler`, `renderer`) | viewer, file watching, OCCT runtime | Nie |

Szybkość = trzy techniki łącznie:

1. **Clang header modules** — OpenCascade + model headers precompiled w ~24MB PCM (`model_headers-*.pcm`), reused across rebuilds
2. **Deferred linking** — plugin `.so` nie linkuje OCCT; host linkuje i exportuje symbole (`-Wl,-export-dynamic` + `dlopen(..., RTLD_GLOBAL)`)
3. **Single translation unit** — user code = jeden `.cpp`; Ninja recompile tylko tego

**Runtime compilation:** `ProjectDir.cpp` nie wywołuje clang bezpośrednio — shell out do CMake + Ninja ze stock `clang`/`clang++`:

```51:108:game-dev/iterative/CppCad2/projectDir/ProjectDir.cpp
void ProjectDir::buildLoop() {
  // ... mkdir, efsw watch na src/ ...
  while(!finished) {
    if(changeNumber == buildNumber) {
      std::this_thread::sleep_for( std::chrono::milliseconds(10) );
      continue;
    }
    if(needToRunCmake) {
      sprintf(cmd, "mkdir -p %s/build; cd %s/build; "
                   "cmake -GNinja %s/src -DHEADERS_PATH=%s -DCMAKE_BUILD_TYPE=Debug"
                   " -DCMAKE_C_COMPILER=%s -DCMAKE_CXX_COMPILER=%s",
          path.c_str(), path.c_str(), path.c_str(), headersPath.c_str(),
          "clang", "clang++");
      system(cmd);
    }
    if(newModTimeSet != modTimesSet) {
      sprintf(cmd, "cd %s/build; ninja", path.c_str());
      auto start = std::chrono::high_resolution_clock::now();
      system(cmd);
      // ... printf Compilation time ...
    }
    buildNumber.store(changeNumber.load());
    notifyObservers();
  }
}
```

**Clang header modules** (nie C++20 `import`):

```1:4:game-dev/iterative/CppCad2/projects/home/src/module.modulemap
module model_headers {
  header "modelHeaders.h"
  export *
}
```

`modelHeaders.h` = umbrella bundle (~15 OCCT headers + `modelScript.h`). CMake plugin:

```16:22:game-dev/iterative/CppCad2/projects/home/src/CMakeLists.txt
add_compile_options(
    -fmodules
    -fcxx-modules
    -fmodules-cache-path=${CMAKE_BINARY_DIR}/clang-modules
    -fmodule-map-file=${CMAKE_SOURCE_DIR}/module.modulemap
    -fno-autolink
)
```

**Reload:** `dlopen` + `dlsym` hardcoded mangled symbol:

```21:72:game-dev/iterative/CppCad2/app/main.cpp
project->addObserver([](std::shared_ptr<ProjectDir> project){
  projectViewer->removeFrameShapes();
  if(libraryHandle) dlclose(libraryHandle);
  std::string libPath = project->path+"/build/lib"+project->name+".so";
  libraryHandle = dlopen(libPath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
  const char* functionName = "_Z13renderProjectv";
  cad::Project* (*function)() = (cad::Project* (*)())dlsym(libraryHandle, functionName);
  cad::Project* newModel = function();
  std::shared_ptr<cad::Project> sharedModel =
      newModel == nullptr ? nullptr : std::make_shared<cad::Project>(*newModel);
```

**Migracja stanu:** brak. `Project model` żyje w `.so`; host deep-copies snapshot przez `std::make_shared<Project>(*newModel)`. Camera persists do `camera.json`. Geometria rebuildowana z `project()`.

**OCCT off hot path:** host linkuje OCCT (`-Wl,-export-dynamic`), plugin tylko include headers (via PCM), `-fno-autolink` zapobiega auto-link pragmas. Link line `libhome.so` = tylko `main.cpp.o`, bez OCCT `.so`.

### Lekcje z CppCad2

1. **Stable host, hot plugin** — rendering, I/O, ciężkie deps w exe; tylko logic w `.so`
2. **Header modules dla ciężkich includes** — umbrella `module.modulemap` + PCM cache
3. **`-fno-autolink` + `-export-dynamic` + `RTLD_GLOBAL`** — plugin nie linkuje ciężkich libs
4. **Single TU** — minimalizuje Ninja work
5. **Ninja incremental** — nie wymyślaj dep tracking, shell out do CMake+Ninja
6. **Explicit entry point** — jeden `extern "C"`/mangled function prostszy niż refleksja
7. **Persist only what matters** — camera do JSON, świat rebuild z kodu

### Luki CppCad2

- Hardcoded mangled `_Z13renderProjectv` — fragile
- `modTimesSet` nigdy nie przypisane po compare → Ninja może biec niepotrzebnie
- Race: observer注册owany przed `projectViewer` skonstruowane
- Brak C++20 modules (to clang **header** modules, nie `import std;`)
- Brak migracji stanu gry (CAD jest stateless — `project()` rebuild)

---

## Era 3b — InrementalCpp (próba uogólnienia)

`game-dev/iterative/InrementalCpp/` — próba zrobienia ogólnego systemu reload modułów. Dwa procesy: **compiler** (watch + build) i **runner** (watch `latest.txt` + reload `.so`).

**Architektura:**

```
compiler (efsw na src/) → CMake+Ninja → build/*.so + latest.txt (SHA1 manifest)
runner (efsw na latest.txt) → dlopen/dlsym/dlclose
```

**Lifecycle C ABI (no macros, no base classes):** dla `libfoo.so`:

```c
extern "C" void foo_init();
extern "C" void foo_dispose();
extern "C" void foo_start();
extern "C" void foo_stop();
extern "C" void foo_loadDependent(std::string name, void *handle);
extern "C" void foo_unloadDependent(std::string name, void *handle);
extern "C" void foo_startUpdate(std::string from, std::string to);
extern "C" void foo_finishUpdate(std::string from, std::string to);
```

Runner `LoadedLibrary` + topologic reload (startUpdate wszystkich → unload w reverse dep order → load w dep order → finishUpdate):

```139:150:game-dev/iterative/InrementalCpp/src/runner.cpp
struct LoadedLibrary {
  std::string name;
  void *handle;
  void (*init)();
  void (*dispose)();
  void (*start)();
  void (*stop)();
  void (*loadDependent)(std::string name, void *handle);
  void (*unloadDependent)(std::string name, void *handle);
  void (*startUpdate)(std::string from, std::string to);
  void (*finishUpdate)(std::string from, std::string to);
};
```

**ZapCC:** referencje **tylko** w fox2d projekcie:

```6:7:game-dev/iterative/InrementalCpp/projects/fox2d/src/CMakeLists.txt
set(CMAKE_C_COMPILER /home/m8/apps/zapcc/bin/zapcc)
set(CMAKE_CPP_COMPILER /home/m8/apps/zapcc/bin/zapcc++)
```

ZapCC = clang fork z cache w RAM (AST, preprocessed headers). System reload nie rozmawia z daemonem ZapCC — tylko benefit pośredni: Ninja compile szybsze. Plain clang re-parsuje headers z dysku każdej invokacji.

**Migracja stanu:** brak. `startUpdate/finishUpdate` = stuby log-only we wszystkich przykładach. Stan przetrwa tylko w **non-reloaded** `.so` (np. `base`), nie w reloaded (`experiment`). Cross-lib coupling przez `extern` data symbols + `RTLD_GLOBAL`:

```15:18:game-dev/iterative/InrementalCpp/projects/fox2d/src/experiment001/src/experiment.cpp
namespace base{
  extern void (*slaveRender)(double renderTime, float delta);
  extern std::shared_ptr<fox2d::ui::Preloader> preloaderUi;
};
```

### Dlaczera InrementalCpp przestarzały

1. **C ABI boundary only** — `dlsym` wymaga `extern "C`; templates/vtables/inline nie reload-primitives
2. **No state migration** — `_startUpdate/finishUpdate` stuby
3. **Vtables/objects across `.so` fragile** — reload invaliduje code pointers z old mapping
4. **No template/header-only reload**
5. **Thread lifecycle manual** — każda reloadable lib musi joinować w `_stop`/`_dispose`
6. **RTLD_GLOBAL symbol pollution**
7. **Coarse granularity** — cały `add_library` target reload
8. **`system("ninja")`** — brak structured error reporting
9. **ZapCC dead** — hardcoded `/home/m8/apps/zapcc/bin/`, fork obsolete
10. **Bugs:** `--local` parsowane tylko w `if(dependency)` bloku; `master_unloadDependent` zła sygnatura

### Co warto zachować z InrementalCpp

| Zachować | Przebudować |
|----------|-------------|
| Two-process split (build watcher / runtime loader) | ZapCC |
| SHA1 selective reload | `extern "C"` + `dlsym` naming |
| Topologic dep load/unload | `RTLD_GLOBAL` cross-lib `extern` |
| `stop` → `dlclose` → `dlopen` → `start` lifecycle | No state migration (refleksja powinna to wziąć) |
| `ProjectDir` efsw + ninja (ewoluowało w CppCad2) | `system()` shell builds |
| fox2d host/slave split (`base` persistent, `experiment` reloadable) | Per-`.so` granularity |

---

## Era 4 (teraz) — fgfx + asset-manager + p2996

Cel: instant reload kodu gry na fgfx **natywnie (dev)**, w sposób uporządkowany, z użyciem serializacji p2996 którą mamy w repo. Web out of scope dla hotswap.

Stosunek do er:
- **Nie** scripting (era 1) — lessons Fox2d
- **Nie** config-as-language (era 2) — lessons Fox2d
- **Tak** szybka rekompilacja C++ (era 3) — CppCad2 udowodnił ~1s achievable
- **Tak** uogólnienie InrementalCpp (era 3b) ale z refleksją zamiast C ABI + ZapCC
- **Nowość:** integracja z p2996 reflection serialization (repo = waos-serialization) dla migracji stanu gry przez reload — czego CppCad2/InrementalCpp nie miały
- **Nowość:** asset-manager już udokumentował hot-reload boundary (persistent service, name-based handles) — gotowe podłoże

Detale obecnego stacku → `kontekst.md`. Sformułowanie problemu → `problem.md`. Propozycje → `propozycje-rozwiazan.md`.
