#ifndef HOTSWAP_RELOAD_MANAGER_HPP
#define HOTSWAP_RELOAD_MANAGER_HPP

#include <hotswap/Manifest.hpp>
#include <functional>

namespace hotswap {

  class ReloadManager {
  public:
    using ReloadCallback = std::function<void(ReloadAction)>;

    void setReloadCallback(ReloadCallback callback) { callback_ = std::move(callback); }

    void handleManifest(const Manifest& incoming) {
      if (incoming.empty()) return;
      if (loaded_.empty()) {
        loaded_ = incoming;
        return;
      }
      auto action = compareManifest(loaded_, incoming);
      if (action == ReloadAction::None) return;
      loaded_ = incoming;
      if (callback_) callback_(action);
    }

    const Manifest& loaded() const { return loaded_; }

  private:
    Manifest loaded_;
    ReloadCallback callback_;
  };

}

#endif
