#ifndef HOTSWAP_MANIFEST_WATCHER_HPP
#define HOTSWAP_MANIFEST_WATCHER_HPP

#include <hotswap/Manifest.hpp>
#include <filesystem>
#include <string>

namespace hotswap {

  class ManifestWatcher {
  public:
    explicit ManifestWatcher(std::string manifestPath);

    bool poll();
    Manifest readManifest() const;

  private:
    std::string manifestPath_;
    std::filesystem::file_time_type lastWrite_{};
    bool hasLastWrite_ = false;
  };

}

#endif
