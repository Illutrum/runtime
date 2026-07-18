#include <hotswap/ManifestWriter.hpp>
#include <hotswap/Manifest.hpp>

#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace hotswap {

  std::string sha256File(const std::string& path) {
    std::string command = "sha256sum \"" + path + "\" 2>/dev/null";
    std::array<char, 256> buffer{};
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
      result += buffer.data();
    }
    pclose(pipe);
    if (result.empty()) return "";
    std::istringstream iss(result);
    std::string hash;
    iss >> hash;
    return hash;
  }

  bool writeManifestAtomic(const std::string& manifestPath,
                           const std::string& modelSoPath,
                           const std::string& viewSoPath) {
    Manifest manifest;
    manifest.modelChecksum = sha256File(modelSoPath);
    manifest.viewChecksum = sha256File(viewSoPath);
    if (manifest.modelChecksum.empty() || manifest.viewChecksum.empty()) {
      return false;
    }
    return writeManifestFile(manifestPath, manifest);
  }

}
