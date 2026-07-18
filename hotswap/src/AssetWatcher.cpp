#include <hotswap/AssetWatcher.hpp>

#include <efsw/efsw.hpp>
#include <iostream>

namespace hotswap {

  namespace {

    class AssetWatchListener : public efsw::FileWatchListener {
    public:
      explicit AssetWatchListener(AssetWatcher* owner) : owner_(owner) {}

      void handleFileAction(efsw::WatchID watchid, const std::string& dir, const std::string& filename,
                            efsw::Action action, std::string oldFilename) override {
        (void)watchid;
        (void)dir;
        (void)oldFilename;
        if (action != efsw::Action::Modified && action != efsw::Action::Add) return;
        if (owner_) owner_->onAssetFileChanged(filename);
      }

    private:
      AssetWatcher* owner_;
    };

  }

  AssetWatcher::AssetWatcher(std::string assetsDir)
    : assetsDir_(std::move(assetsDir)) {}

  AssetWatcher::~AssetWatcher() {
    stop();
  }

  void AssetWatcher::registerAssetPath(const std::string& relativePath,
                                       std::shared_ptr<asset_manager::Asset> asset) {
    pathToAsset_[relativePath] = std::move(asset);
  }

  void AssetWatcher::onAssetFileChanged(const std::string& filename) {
    auto it = pathToAsset_.find(filename);
    if (it == pathToAsset_.end()) return;
    if (it->second) {
      it->second->startReload();
    }
  }

  void AssetWatcher::start() {
    auto listener = std::make_unique<AssetWatchListener>(this);
    watcher_ = std::make_unique<efsw::FileWatcher>();
    watchId_ = watcher_->addWatch(assetsDir_, listener.get(), true);
    watcher_->watch();
    listener_ = std::move(listener);
  }

  void AssetWatcher::stop() {
    if (watcher_) {
      watcher_->removeWatch(watchId_);
      watcher_.reset();
    }
    listener_.reset();
  }

}
