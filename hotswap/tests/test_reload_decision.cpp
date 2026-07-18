#include <doctest/doctest.h>
#include <hotswap/ReloadManager.hpp>

TEST_CASE("ReloadManager ignores spurious manifest") {
  hotswap::ReloadManager manager;
  int calls = 0;
  manager.setReloadCallback([&](hotswap::ReloadAction action) {
    (void)action;
    calls++;
  });

  hotswap::Manifest first{"a", "b"};
  manager.handleManifest(first);
  CHECK(calls == 0);

  manager.handleManifest(first);
  CHECK(calls == 0);
}

TEST_CASE("ReloadManager triggers on view change") {
  hotswap::ReloadManager manager;
  hotswap::ReloadAction last = hotswap::ReloadAction::None;
  manager.setReloadCallback([&](hotswap::ReloadAction action) {
    last = action;
  });

  manager.handleManifest({"a", "b"});
  manager.handleManifest({"a", "c"});
  CHECK(last == hotswap::ReloadAction::ReloadViewOnly);
}

TEST_CASE("ReloadManager triggers model reload for model change") {
  hotswap::ReloadManager manager;
  hotswap::ReloadAction last = hotswap::ReloadAction::None;
  manager.setReloadCallback([&](hotswap::ReloadAction action) {
    last = action;
  });

  manager.handleManifest({"a", "b"});
  manager.handleManifest({"x", "b"});
  CHECK(last == hotswap::ReloadAction::ReloadModelAndView);
}
