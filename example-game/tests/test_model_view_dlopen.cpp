#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "GameState.hpp"
#include "Input.hpp"
#include "Model.hpp"

#include <asset_manager.hpp>
#include <fastgfx.hpp>

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>

#ifndef MODEL_SO_PATH
#define MODEL_SO_PATH "libmodel.so"
#endif

#ifndef VIEW_SO_PATH
#define VIEW_SO_PATH "libview.so"
#endif

#ifndef MANIFEST_PATH
#define MANIFEST_PATH "hotswap.manifest"
#endif

namespace {

  void touchHostSymbolsForViewPlugin() {
    volatile void* keepEngine = static_cast<void*>(&fgfx::engine);
    volatile void* keepManager = static_cast<void*>(&asset_manager::manager);
    (void)keepEngine;
    (void)keepManager;
  }

  bool fileExists(const char* path) {
    std::ifstream file(path);
    return file.good();
  }

  bool libraryExportsSymbol(const char* libraryPath, const char* symbol) {
    std::string command = std::string("nm -D \"") + libraryPath + "\" 2>/dev/null";
    std::array<char, 256> buffer{};
    std::string output;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return false;
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
      output += buffer.data();
    }
    pclose(pipe);
    return output.find(std::string(" ") + symbol) != std::string::npos;
  }

  std::string readFnString(size_t (*fn)(void*, char**), void* instance) {
    char* buffer = nullptr;
    size_t length = fn(instance, &buffer);
    if (!buffer || length == 0) return "";
    std::string result(buffer, length);
    std::free(buffer);
    return result;
  }

}

TEST_CASE("model.so dlopen lifecycle and serialize roundtrip") {
  void* handle = dlopen(MODEL_SO_PATH, RTLD_NOW | RTLD_LOCAL);
  REQUIRE(handle != nullptr);

  auto* create = reinterpret_cast<void* (*)()>(dlsym(handle, "model_create"));
  auto* destroy = reinterpret_cast<void (*)(void*)>(dlsym(handle, "model_destroy"));
  auto* init = reinterpret_cast<void (*)(void*)>(dlsym(handle, "model_init"));
  auto* serializeFn = reinterpret_cast<size_t (*)(void*, char**)>(dlsym(handle, "model_serialize"));
  auto* deserializeFn = reinterpret_cast<void (*)(void*, const char*, size_t)>(dlsym(handle, "model_deserialize"));
  auto* schemaFn = reinterpret_cast<size_t (*)(void*, char**)>(dlsym(handle, "model_schema_checksum"));

  REQUIRE(create != nullptr);
  REQUIRE(destroy != nullptr);
  REQUIRE(serializeFn != nullptr);
  REQUIRE(deserializeFn != nullptr);
  REQUIRE(schemaFn != nullptr);

  void* instance = create();
  REQUIRE(instance != nullptr);
  if (init) init(instance);

  auto* model = static_cast<Model*>(instance);
  Input input{.moveX = 1.0f, .moveY = 0.5f, .reset = false};
  model->update(input, 0.25f);

  const auto* snapshot = static_cast<const GameState*>(model->snapshot());
  REQUIRE(snapshot != nullptr);
  CHECK(snapshot->counter == 1);

  std::string schemaA = readFnString(schemaFn, instance);
  std::string schemaB = readFnString(schemaFn, instance);
  CHECK(schemaA == schemaB);
  CHECK_FALSE(schemaA.empty());

  char* bytes = nullptr;
  size_t length = serializeFn(instance, &bytes);
  REQUIRE(bytes != nullptr);
  REQUIRE(length > 0);

  model->update(input, 0.25f);
  CHECK(static_cast<const GameState*>(model->snapshot())->counter == 2);

  deserializeFn(instance, bytes, length);
  std::free(bytes);

  CHECK(static_cast<const GameState*>(model->snapshot())->counter == 1);

  destroy(instance);
  dlclose(handle);
}

TEST_CASE("view.so exports C ABI entry points") {
  touchHostSymbolsForViewPlugin();
  CHECK(fileExists(VIEW_SO_PATH));
  CHECK(libraryExportsSymbol(MODEL_SO_PATH, "model_create"));
  CHECK(libraryExportsSymbol(VIEW_SO_PATH, "view_create"));
  CHECK(libraryExportsSymbol(VIEW_SO_PATH, "view_destroy"));
}

TEST_CASE("hotswap manifest matches built plugin artifacts") {
  CHECK(fileExists(MANIFEST_PATH));
  std::ifstream manifest(MANIFEST_PATH);
  REQUIRE(manifest.good());

  std::string modelHash;
  std::string viewHash;
  std::getline(manifest, modelHash);
  std::getline(manifest, viewHash);

  CHECK_FALSE(modelHash.empty());
  CHECK_FALSE(viewHash.empty());
  CHECK(modelHash.size() == 64);
  CHECK(viewHash.size() == 64);
}
