#ifndef HOTSWAP_MANIFEST_WRITER_HPP
#define HOTSWAP_MANIFEST_WRITER_HPP

#include <string>

namespace hotswap {

  std::string sha256File(const std::string& path);
  bool writeManifestAtomic(const std::string& manifestPath,
                           const std::string& modelSoPath,
                           const std::string& viewSoPath);

}

#endif
