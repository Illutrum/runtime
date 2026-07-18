#include <doctest/doctest.h>
#include <hotswap/Manifest.hpp>

TEST_CASE("compareManifest detects view-only change") {
  hotswap::Manifest loaded{"aaa", "bbb"};
  hotswap::Manifest incoming{"aaa", "ccc"};
  CHECK(hotswap::compareManifest(loaded, incoming) == hotswap::ReloadAction::ReloadViewOnly);
}

TEST_CASE("compareManifest detects model change") {
  hotswap::Manifest loaded{"aaa", "bbb"};
  hotswap::Manifest incoming{"ccc", "bbb"};
  CHECK(hotswap::compareManifest(loaded, incoming) == hotswap::ReloadAction::ReloadModelAndView);
}

TEST_CASE("compareManifest spurious no-op") {
  hotswap::Manifest loaded{"aaa", "bbb"};
  hotswap::Manifest incoming{"aaa", "bbb"};
  CHECK(hotswap::compareManifest(loaded, incoming) == hotswap::ReloadAction::None);
}

TEST_CASE("read and write manifest roundtrip") {
  hotswap::Manifest manifest{"modelhash", "viewhash"};
  auto path = std::string("/tmp/hotswap_test_manifest.txt");
  CHECK(hotswap::writeManifestFile(path, manifest));
  auto read = hotswap::readManifestFile(path);
  CHECK(read.modelChecksum == "modelhash");
  CHECK(read.viewChecksum == "viewhash");
}
