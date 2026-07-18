#include <doctest/doctest.h>

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <string>

#ifndef MODEL_SO_PATH
#define MODEL_SO_PATH "libmodel.so"
#endif

namespace {

  std::string readSchemaChecksum(void* instance, size_t (*schemaFn)(void*, char**)) {
    char* buffer = nullptr;
    size_t length = schemaFn(instance, &buffer);
    if (!buffer || length == 0) return "";
    std::string result(buffer, length);
    std::free(buffer);
    return result;
  }

}

TEST_CASE("model schema checksum is stable for unchanged GameState") {
  void* handle = dlopen(MODEL_SO_PATH, RTLD_NOW | RTLD_LOCAL);
  REQUIRE(handle != nullptr);

  auto* create = reinterpret_cast<void* (*)()>(dlsym(handle, "model_create"));
  auto* destroy = reinterpret_cast<void (*)(void*)>(dlsym(handle, "model_destroy"));
  auto* schemaFn = reinterpret_cast<size_t (*)(void*, char**)>(dlsym(handle, "model_schema_checksum"));

  REQUIRE(create != nullptr);
  REQUIRE(destroy != nullptr);
  REQUIRE(schemaFn != nullptr);

  void* first = create();
  void* second = create();
  REQUIRE(first != nullptr);
  REQUIRE(second != nullptr);

  std::string firstHash = readSchemaChecksum(first, schemaFn);
  std::string secondHash = readSchemaChecksum(second, schemaFn);

  CHECK(firstHash == secondHash);
  CHECK_FALSE(firstHash.empty());

  destroy(first);
  destroy(second);
  dlclose(handle);
}
