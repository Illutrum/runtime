#ifndef HOTSWAP_ASSET_WATCHER_HPP
#define HOTSWAP_ASSET_WATCHER_HPP

#include <Asset.hpp>
#include <efsw/efsw.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace hotswap {

  class AssetWatcher : public std::enable_shared_from_this<AssetWatcher> {
  public:
    explicit AssetWatcher(std::string assetsDir);
    ~AssetWatcher();

    void start();
    void stop();
    void registerAssetPath(const std::string& relativePath, std::shared_ptr<asset_manager::Asset> asset);
    void onAssetFileChanged(const std::string& filename);

  private:
    std::string assetsDir_;
    std::unique_ptr<efsw::FileWatcher> watcher_;
    long watchId_ = -1;
    std::unique_ptr<efsw::FileWatchListener> listener_;
    std::unordered_map<std::string, std::shared_ptr<asset_manager::Asset>> pathToAsset_;
  };

}

#endif
