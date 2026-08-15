#include <doctest/doctest.h>
#include <hotswap/ManifestWriter.hpp>
#include <fstream>

TEST_CASE("writeManifestAtomic writes both checksum lines") {
  auto modelPath = std::string("/tmp/hotswap_test_model.so");
  auto viewPath = std::string("/tmp/hotswap_test_view.so");
  auto manifestPath = std::string("/tmp/hotswap_test_manifest_atomic.txt");

  {
    std::ofstream modelOut(modelPath, std::ios::binary);
    modelOut << "model-bytes";
  }
  {
    std::ofstream viewOut(viewPath, std::ios::binary);
    viewOut << "view-bytes";
  }

  CHECK(hotswap::writeManifestAtomic(manifestPath, modelPath, viewPath));

  std::ifstream in(manifestPath);
  std::string modelLine;
  std::string viewLine;
  std::getline(in, modelLine);
  std::getline(in, viewLine);
  CHECK(modelLine.size() == 64);
  CHECK(viewLine.size() == 64);
}

TEST_CASE("writeManifestAtomic allows empty model path") {
  auto viewPath = std::string("/tmp/hotswap_test_view_only.so");
  auto manifestPath = std::string("/tmp/hotswap_test_manifest_view_only.txt");

  {
    std::ofstream viewOut(viewPath, std::ios::binary);
    viewOut << "view-bytes";
  }

  CHECK(hotswap::writeManifestAtomic(manifestPath, "", viewPath));

  std::ifstream in(manifestPath);
  std::string modelLine;
  std::string viewLine;
  std::getline(in, modelLine);
  std::getline(in, viewLine);
  CHECK(modelLine.empty());
  CHECK(viewLine.size() == 64);
}
