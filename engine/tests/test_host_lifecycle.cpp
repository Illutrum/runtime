#include <doctest/doctest.h>
#include <engine/Backend.hpp>

namespace {
  void* createFn() { return nullptr; }
  void destroyFn(void*) {}
}

TEST_CASE("ModelBackend typed access") {
  int value = 42;
  engine::ModelBackend backend;
  backend.set(&value, createFn, destroyFn);
  CHECK(backend.as<int>() == &value);
}
