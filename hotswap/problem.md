# Problem — sformułowanie

Precyzyjne sformułowanie problemu instant reload kodu gry, z wymaganiami, napięciami i anty-wzorama. Hotswap **tylko natywnie (GLFW/Linux = dev path)**; web out of scope.

---

## Cel

**Instant reload kodu gry w czasie developmentu, ~1s jak CppCad2, żeby nie stracić focusa.**

Konkretne scenariusze z developowania gier:
- tuning stałych (prędkość czołgu, grawitacja, cooldown)
- testowanie wzorów (krzywe damage, easing, spawn rate)
- gameplay feel (input responsiveness, animation timing)
- dodawanie/mechaniki, sprawdzanie jak wpływa na grywalność
- szybkie retry po błędzie bez restart sesji

To co game devs zwykle rozwiązują scripting language albo konfiguracją — oba podejścia sprawdzone i odrzucone (patrz `historia.md`). Trzecie podejście: **szybka rekompilacja samego C++**, udowodnione w CppCad2 (~1s z clang modules + single-TU + deferred linking).

**Co reloadujemy:** kod gry (logika, entity state, render callbacks, tuning).
**Czego nie reloadujemy:** engine (fgfx), asset-manager, GL context, shader programs, wątki I/O — to są persistent services.

---

## Dlaczego nie scripting language

Lekcja Fox2d era 1 (AngelScript):

| Problem | Mechanizm |
|---------|-----------|
| Kolejna warstwa abstrakcji | script runtime + binding glue ponad C++ |
| Każda metoda C++ do wyprowadzenia manualnie | brak refleksji, template registration `clazz<T>`, `objectMethod` |
| Maintenance burden | 750-loc `AngelScriptBinder.h`, `AS_MAX_PORTABILITY`, `proxy<T>` marshalling |
| Type-safety lost na boundary | `assert(r >= 0)`, runtime null-pointer, value marshalling |
| Build complexity | opcjonalny `USE_SCRIPTS` ciągnie ~65k LOC runtime |
| Cost >> benefit | 65k LOC + 1.4k LOC glue = **0 skryptów w assets**, 2 bindingi zarejestrowane |
| Preprocessor na preprocessorze | `Code` adds `#include`/`#ifdef` przed AS compilation |
| Brak shipped debug story | compiler errors via MessageCallback, TODOs |

**Podsumowanie:** scripting language wprowadza kolejną skomplikowaną warstwę, wymaga wyprowadzania każdej metody przez API, i daje type-safety loss — dla gainu iteracyjnego który można osiągnąć szybką rekompilacją C++ bez tych kosztów.

---

## Dlaczego nie configuration

Lekcja Fox2d era 2 (`.fox` DSL):

| Problem | Mechanizm |
|---------|-----------|
| Puchnie w pseudojęzyk | `Transform [ translate() scale() ]`, `KeyFrame(0.8, [ spawnTank() fire() ])`, `@`-chain particle |
| Brak toolingu | brak LSP, syntax highlight, static analysis dla `.fox` |
| Schema drift | `FoxTank.cpp` ~40 manualnych czytań pól, każdy consumer czyta inne klucze |
| Brak walidacji | runtime exceptions z debug dumpami |
| Każda komenda = C++ registry | `spawnTank`, `fire`, `setCamera` — każdy hand-written lambda |
| Combinatorial explosion | 64 pre-rejestrowane particle template combos |
| Custom parser do utrzymania | hand-rolled lexer `Value.cpp` ~560 LOC, `/// TODO: escape sequences` |
| Niezaimplementowane features | `symbolicLink` pole exists, nigdy nie ustawiane — referencje zaplanowane, porzucone |
| Duplikacja | `model/buttBoilerTank.fox` + `view/buttBoilerTank.fox` osobno |

**Podsumowanie:** konfiguracja ma tendencję do powolnego zmieniania się we własny język skryptowy — bez toolingu, bez walidacji, z schema drift i combinatorial explosion. Każda nowa komenda/moduł wymaga C++ registry **i** config name matching — dwa miejsca do utrzymania.

---

## Dlaczego rekompilacja C++ jest atrakcyjna

| Zaleta | Wyjaśnienie |
|--------|-------------|
| Pełna type safety | kompilator łapie błędy, brak boundary type-erasure |
| Pełna moc języka | templates, lambdas, ranges, concepts — bez ograniczeń script VM |
| Refactor tools | IDE rename/extract/find usages działa na kodzie gry |
| Debugger | gdb/ldb breakpoints, step-through, inspect — na real kodzie |
| Brak dodatkowej warstwy | jeden język, jedno API, brak binding glue |
| Brak schema drift | kod = definicja, nie osobna reprezentacja |
| Deterministic compile | każdy ma ten sam kompilator → ten sam behavior |

**Problem:** rekompilacja C++ jest **wolna**. Pełny rebuild fgfx + game = minuty, nie sekundy. To blokuje iterację.

**CppCad2 udowodnił:** ~1s achievable z:
1. **clang header modules** — ciężkie includes (OpenCascade, fgfx, glm) precompiled w PCM cache, reused across rebuilds
2. **Single TU** — user code = jeden `.cpp`; Ninja recompile tylko tego
3. **Deferred linking** — plugin `.so` nie linkuje ciężkich libs; host linkuje i exportuje symbole (`-export-dynamic` + `RTLD_GLOBAL`)

To działa bo większość czasu kompilacji to parsing headers, nie user code. Header modules cachują parsing.

---

## Wymagania

1. **Tylko native (GLFW/Linux)** — dev path; web nie potrzebuje hotswap (pełny rebuild OK tam)
2. **Engine/serwisy persistent, kod gry reloadable** — `.so` + `dlopen` split
3. **Stan gry ma przetrwać reload** — tu wchodzi serializacja p2996 (cpp-rpc)
4. **Integracja z asset-manager** — już persistent, name-based handles, kontrakt udokumentowany
5. **Integracja z p2996 serialization** — repo nazywa się waos-serialization, mamy gotowy reflection-based serializer
6. **Uporządkowane, uogólnione** — nie hardcoded mangled symbol jak CppCad2 (`_Z13renderProjectv`)
7. **~1s iteracji** — clang header modules + single-TU + deferred linking
8. **Brak zapcc dependency** — używamy clang p2996 fork (cpp-compiler)

---

## Konflikty / napięcia

### 1. raw memory preservation vs typed serialization

Dwie strategie migracji stanu:
- **Raw memory** (fixed-address arena, `dlopen` zachowuje wskaźniki): zero-cost, natychmiast — ale **fragile** przy layout change (struct doda pole → wskaźniki się przesuną → UB)
- **Typed serialize/deserialize** (cpp-rpc p2996): bezpieczne, version-tolerant — ale **overhead** (serialize wszystkich pól, alloc fresh)

**Tradeoff:** szybkość vs odporność. CppCad2 wybrał reconstruct (brak migracji) bo CAD jest stateless. Gra **ma** stan (entity positions, health, score) — trzeba migrować.

### 2. asset-manager contract vs obecny kod

`loading_screen_example.cpp` łamie udokumentowany kontrakt — trzyma `logoSprite`, layers, `fontAsset->font` w `DemoState` (raw payload pointers przez reload). Fine dla demo, ale design reload musi to traktować jako non-surviving. Kod gry musi re-requestować przez `manager.request<T>(path)` po reload.

### 3. game-dev nie używa p2996 toolchainu

Dziś game-dev builduje się stock clang (patrz [fgfx/CMakeLists.txt](game-dev/fgfx/CMakeLists.txt), `p2996-toolchain.cmake` nie używany). Włączenie p2996 dla `game.so` = zmiana build config:
- `CMAKE_CXX_COMPILER` → `cpp-compiler/sysroot/usr/local/bin/clang++`
- flags: `-freflection -fparameter-reflection -fannotation-attributes -fexpansion-statements -std=c++26`
- sysroot includes/libs

Nie fork, nie rewrite — config change. Ale dotyka całego toolchainu game-dev.

### 4. vtable / shared_ptr layout-stability przez reload

C++ objects z vtable'ami przekraczające `.so` boundary są **fragile** — reload invaliduje code pointers z old mapping. `shared_ptr` do typu zdefiniowanego w reloadable `.so` → po reload control block wskazuje na zwolniony kod.

**Strategie:**
- Layout-stable interfaces: host i plugin share headers, ABI stabilne przez reload
- Raw pointer / value types na boundary (jak CppCad2: `renderProject()` zwraca `Project*`, host kopiuje)
- GameState jako **value type** (POD-ish) serializable — nie object graph z vtable'ami

### 5. thread lifecycle w reloadable `.so`

Każda reloadable `.so` z wątkami musi je joinować w `stop`/`dispose` (leak/crash otherwise). InrementalCpp to miał manual. fgfx `fgfx::io` thread pool żyje w hoście (persistent) — OK. Ale game code z własnymi wątkami = dyscyplina.

### 6. clang header modules = native only

PCM cache to native clang feature. Web (Emscripten) ma inny module story — ale **web out of scope**, więc to nie problem. Native path: `module.modulemap` umbrella dla fgfx+asset-manager+glm → PCM cache.

### 7. co jest "game code" vs "engine code"

Granica nie zawsze czysta. Np. custom layery (`createSpriteLayer`) tworzone przez game — one są `shared_ptr` w game struct, po reload starą warstwę trzeba dropnąć. Kto owns GL buffer — engine czy layer? Dziś layer tworzy buffer, po reload game tworzy nowy layer (stary ewentualnie leakuje jeśli nie dropnięty).

**Zasada:** engine owns named resources (sprites/textures/fonts w `Engine` maps). Game owns własne layery/entity state — re-creates po reload, re-requestuje assety.

---

## Czego unikamy (anty-wzory)

Z Fox2d + InrementalCpp + CppCad2:

| Anty-wzór | Skąd | Dlaczego |
|-----------|------|----------|
| Hardcoded mangled symbol | CppCad2 `_Z13renderProjectv` | fragile, łamie się przy namespace/signature change |
| No state migration | CppCad2, InrementalCpp | gra traci stan, nieakceptowalne dla długich sesji |
| RTLD_GLOBAL symbol pollution | InrementalCpp | duplicate symbol definitions, subtle bugs |
| Per-`.so` coarse granularity bez migracji | InrementalCpp | cały target reload, brak partial |
| Zapcc-style dead fork dependency | InrementalCpp | fork obsolete, hardcoded paths |
| Config-as-language | Fox2d `.fox` | puchnie, brak toolingu, schema drift |
| Scripting binding bloat | Fox2d AngelScript | cost >> benefit, manual API registration |
| Crude "nuke and recreate" reload | Fox2d model-side | pełny reset sesji, nie in-place swap |
| `system("ninja")` bez error reporting | CppCad2, InrementalCpp | brak IDE diagnostics integration |
| Config cache blokuje reload | Fox2d `loadedConfigurations` | reload nie invaliduje |
| Race: observer przed viewer skonstruowane | CppCad2 `app/main.cpp` | null deref |

---

## Nie-met cele (out of scope)

- Web hotswap (Emscripten) — pełny rebuild OK tam
- Screenshot / pixel-diff regression tests — visual QA manual
- Production hot-reload (shipped build) — tylko dev
- Multiplayer state sync przez reload — osobny problem
- Generic plugin system dla third-party — tylko nasza gra

---

## Kryteria sukcesu

- Zmiana stałej w kodzie gry → widzę efekt w < 2s na natywnym build
- Stan gry (entity positions, score) przetrwa reload
- Assety nie przeładowują się (asset-manager cache hit)
- Compile error → czytelny komunikat, nie crash hosta
- Brak ZapCC dependency
- Brak hardcoded symboli — interface discovered przez reflection lub stabilny `extern "C"`
- Działa na Linux + GLFW (dev environment)

Jak to osiągnąć → `propozycje-rozwiazan.md`.
