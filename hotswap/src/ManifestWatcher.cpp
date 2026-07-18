#include <hotswap/ManifestWatcher.hpp>

namespace hotswap {

  ManifestWatcher::ManifestWatcher(std::string manifestPath)
    : manifestPath_(std::move(manifestPath)) {}

  bool ManifestWatcher::poll() {
    namespace fs = std::filesystem;
    if (!fs::exists(manifestPath_)) return false;
    auto writeTime = fs::last_write_time(manifestPath_);
    if (!hasLastWrite_ || writeTime != lastWrite_) {
      lastWrite_ = writeTime;
      hasLastWrite_ = true;
      return true;
    }
    return false;
  }

  Manifest ManifestWatcher::readManifest() const {
    return readManifestFile(manifestPath_);
  }

}
