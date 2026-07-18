#include <hotswap/Manifest.hpp>

#include <fstream>
#include <sstream>

namespace hotswap {

  ReloadAction compareManifest(const Manifest& loaded, const Manifest& incoming) {
    if (loaded == incoming) return ReloadAction::None;
    if (loaded.modelChecksum != incoming.modelChecksum) {
      return ReloadAction::ReloadModelAndView;
    }
    if (loaded.viewChecksum != incoming.viewChecksum) {
      return ReloadAction::ReloadViewOnly;
    }
    return ReloadAction::None;
  }

  Manifest readManifestFile(const std::string& path) {
    Manifest manifest;
    std::ifstream in(path);
    if (!in.is_open()) return manifest;
    std::getline(in, manifest.modelChecksum);
    std::getline(in, manifest.viewChecksum);
    return manifest;
  }

  bool writeManifestFile(const std::string& path, const Manifest& manifest) {
    std::string tmp = path + ".tmp";
    {
      std::ofstream out(tmp);
      if (!out.is_open()) return false;
      out << manifest.modelChecksum << "\n";
      out << manifest.viewChecksum << "\n";
    }
    std::rename(tmp.c_str(), path.c_str());
    return true;
  }

}
