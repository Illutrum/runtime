#ifndef HOTSWAP_MANIFEST_HPP
#define HOTSWAP_MANIFEST_HPP

#include <string>

namespace hotswap {

  struct Manifest {
    std::string modelChecksum;
    std::string viewChecksum;

    bool operator==(const Manifest& other) const {
      return modelChecksum == other.modelChecksum && viewChecksum == other.viewChecksum;
    }

    bool empty() const {
      return modelChecksum.empty() && viewChecksum.empty();
    }
  };

  enum class ReloadAction {
    None,
    ReloadViewOnly,
    ReloadModelAndView
  };

  ReloadAction compareManifest(const Manifest& loaded, const Manifest& incoming);

  Manifest readManifestFile(const std::string& path);
  bool writeManifestFile(const std::string& path, const Manifest& manifest);

}

#endif
