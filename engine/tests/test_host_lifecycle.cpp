#include <doctest/doctest.h>
#include <engine/Backend.hpp>

TEST_CASE("ModelBackend typed access") {
  int value = 42;
  engine::ModelBackend backend;
  backend.attach(&value);
  CHECK(backend.as<int>() == &value);
  CHECK(backend.loaded());
}

TEST_CASE("Backend attach and clear do not destroy") {
  int destroyCount = 0;
  auto destroy = [&](void*) { destroyCount++; };
  (void)destroy;

  int value = 7;
  engine::ViewBackend backend;
  backend.attach(&value);
  CHECK(backend.as<int>() == &value);

  backend.clear();
  backend.clear();
  CHECK_FALSE(backend.loaded());
  CHECK(backend.instance() == nullptr);
  CHECK(destroyCount == 0);
}

TEST_CASE("asset unload is the only destroy path") {
  int destroyCount = 0;
  void* instance = &destroyCount;
  auto destroyFn = [](void* p) { (*static_cast<int*>(p))++; };

  engine::ViewBackend backend;
  backend.attach(instance);
  backend.clear();
  backend.clear();
  CHECK(destroyCount == 0);

  if (instance) {
    destroyFn(instance);
    instance = nullptr;
  }
  CHECK(destroyCount == 1);
  CHECK(instance == nullptr);
}
