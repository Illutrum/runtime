#ifndef ENGINE_HOST_HPP
#define ENGINE_HOST_HPP

#include <engine/Backend.hpp>
#include <engine/InputCollector.hpp>
#include <functional>
#include <memory>
#include <string>

namespace asset_manager {
  class LoadTask;
  class MSDFFontAsset;
}

namespace asset_manager::ui {
  class LoadingScreen;
}

namespace fgfx {
  class GlyphBatch;
  class PolygonBatch;
  class FrameBuilder;
}

#ifdef __HOTSWAP
namespace hotswap {
  enum class ReloadAction;
}
#endif

namespace engine {

  struct GameHostConfig {
    std::string gameSourceDir;
    std::string gameBuildDir;
    std::string cmakeSourceDir;
    std::string modelSoPath;
    std::string viewSoPath;
    std::string manifestPath;
    std::string viewTarget = "view";
    std::string modelTarget = "model";
    std::string assetsDir;
    std::string uiFontPath = "assets/ui";
  };

  class GameHost {
  public:
    using FrameCallback = std::function<void(fgfx::FrameBuilder&, float delta, const RawInput& input)>;

    explicit GameHost(GameHostConfig config);
    ~GameHost();

    void init();
    void run();
    void destroy();

    void setFrameCallback(FrameCallback callback);

    ModelBackend& modelBackend() { return modelBackend_; }
    ViewBackend& viewBackend() { return viewBackend_; }
    InputCollector& input() { return input_; }

    bool isLoading() const { return loading_; }
    bool moduleFailed() const { return moduleFailed_; }
    bool resetRequested() const { return input_.current().resetRequested; }
    void clearResetRequest();

    void renderLoadingUI(fgfx::FrameBuilder& builder);
    void setErrorMessage(const std::string& message);

    void bindBackendsFromAssets();
#ifdef __HOTSWAP
    void pollReload();
    void startBuildOrchestrator();
    void stopBuildOrchestrator();
    void startAssetWatcher();
    void stopAssetWatcher();
    void performReload(hotswap::ReloadAction action);
#endif

  private:
    void setupRenderLoop();
    void setupLoadingUI();
    void startModulesLoadTask();
    void onLoadTaskFailed(const std::shared_ptr<asset_manager::LoadTask>& task);

    GameHostConfig config_;
    FrameCallback frameCallback_;
    ModelBackend modelBackend_;
    ViewBackend viewBackend_;
    InputCollector input_;

    std::shared_ptr<asset_manager::LoadTask> loadTask_;
    std::shared_ptr<asset_manager::LoadTask> bootstrapLoadTask_;
    std::shared_ptr<asset_manager::MSDFFontAsset> uiFontAsset_;
    std::shared_ptr<asset_manager::ui::LoadingScreen> loadingScreen_;
    std::shared_ptr<fgfx::GlyphBatch> glyphBatch_;
    std::shared_ptr<fgfx::PolygonBatch> barBatch_;

    bool loading_ = true;
    bool moduleFailed_ = false;
    bool showScene_ = false;
    long frame_ = 0;
    std::string errorMessage_;

#ifdef __HOTSWAP
    struct HotswapState;
    std::unique_ptr<HotswapState> hotswap_;
#endif
  };

}

#endif
