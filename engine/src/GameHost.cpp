#include <engine/Host.hpp>
#include <engine/GlfwInputBinding.hpp>
#include <engine/WasmInputBinding.hpp>

#ifdef __USE_GLFW
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#endif

#include <LoadingScreen.hpp>
#include <asset_manager.hpp>
#include <asset_manager_fgfx.hpp>
#include <render.hpp>
// FrameGraph.hpp (FrameBuilder) is pulled in via fastgfx.hpp.

#ifdef __HOTSWAP
#include <hotswap/BuildOrchestrator.hpp>
#include <hotswap/ManifestWatcher.hpp>
#include <hotswap/ReloadManager.hpp>
#include <hotswap/AssetWatcher.hpp>
#include "ModelModuleAsset.hpp"
#include "ViewModuleAsset.hpp"
#include <cstdlib>
#endif

namespace engine {

#ifdef __HOTSWAP
  struct GameHost::HotswapState {
    std::shared_ptr<hotswap::BuildOrchestrator> buildOrchestrator;
    std::shared_ptr<hotswap::ManifestWatcher> manifestWatcher;
    std::shared_ptr<hotswap::ReloadManager> reloadManager;
    std::shared_ptr<hotswap::AssetWatcher> assetWatcher;
    std::shared_ptr<hotswap::ModelModuleAsset> modelAsset;
    std::shared_ptr<hotswap::ViewModuleAsset> viewAsset;
  };

  namespace {
    std::string readFnString(size_t (*fn)(void*, char**), void* instance) {
      char* buffer = nullptr;
      size_t length = fn(instance, &buffer);
      if (!buffer || length == 0) return "";
      std::string result(buffer, length);
      std::free(buffer);
      return result;
    }
  }
#endif

  GameHost::GameHost(GameHostConfig config)
    : config_(std::move(config)) {
#ifdef __HOTSWAP
    hotswap_ = std::make_unique<HotswapState>();
#endif
  }

  GameHost::~GameHost() {
    destroy();
  }

  void GameHost::init() {
    render::init();

    asset_manager::TextureAsset::registerAssetType();
    asset_manager::MSDFFontAsset::registerAssetType();
    asset_manager::JsonAsset::registerAssetType();

#ifdef __HOTSWAP
    hotswap::ModelModuleAsset::registerAssetType();
    hotswap::ViewModuleAsset::registerAssetType();
#endif

    glyphBatch_ = std::make_shared<render::GlyphBatch>(render::engine.get());
    barBatch_ = std::make_shared<render::PolygonBatch>(render::engine.get());
    setupLoadingUI();
    bindGlfwInput(this);
    bindWasmInput(this);
    setupRenderLoop();

#ifdef __HOTSWAP
    startBuildOrchestrator();
    startAssetWatcher();
#endif
  }

  void GameHost::bindBackendsFromAssets() {
#ifdef __HOTSWAP
    if (!hotswap_->viewAsset) return;
    if (hotswap_->modelAsset) {
      modelBackend_.attach(hotswap_->modelAsset->instance);
    }
    viewBackend_.attach(hotswap_->viewAsset->instance);
#endif
  }

  void GameHost::setupLoadingUI() {
    uiFontAsset_ = asset_manager::manager.getAsset<asset_manager::MSDFFontAsset>(config_.uiFontPath);

    bootstrapLoadTask_ = std::make_shared<asset_manager::LoadTask>("Engine UI");
    bootstrapLoadTask_->addAsset(uiFontAsset_);
    bootstrapLoadTask_->addObserver([this]() {
      if (bootstrapLoadTask_->state == asset_manager::LoadTask::LoadTaskState::Loaded) {
        startModulesLoadTask();
      } else if (bootstrapLoadTask_->state == asset_manager::LoadTask::LoadTaskState::Failed) {
        onLoadTaskFailed(bootstrapLoadTask_);
      }
    });
    bootstrapLoadTask_->start();
  }

  void GameHost::startModulesLoadTask() {
    auto uiFont = uiFontAsset_ ? uiFontAsset_->font : nullptr;
    if (!uiFont) {
      onLoadTaskFailed(bootstrapLoadTask_);
      return;
    }

    loadingScreen_ = std::make_shared<asset_manager::ui::LoadingScreen>(glyphBatch_, uiFont, barBatch_);
    loadTask_ = std::make_shared<asset_manager::LoadTask>("Game modules");

#ifdef __HOTSWAP
    hotswap_->viewAsset = asset_manager::manager.getAsset<hotswap::ViewModuleAsset>(config_.viewSoPath);
    if (!config_.modelSoPath.empty()) {
      hotswap_->modelAsset = asset_manager::manager.getAsset<hotswap::ModelModuleAsset>(config_.modelSoPath);
      hotswap_->viewAsset->addDependency(hotswap_->modelAsset);
      loadTask_->addAsset(hotswap_->modelAsset);
    }
    loadTask_->addAsset(hotswap_->viewAsset);
#endif

    loadingScreen_->show(loadTask_);
    loadTask_->addObserver([this]() {
      if (loadTask_->state == asset_manager::LoadTask::LoadTaskState::Loaded) {
        bindBackendsFromAssets();
        loading_ = false;
        showScene_ = true;
        moduleFailed_ = false;
        errorMessage_.clear();
        loadingScreen_->hide();
      } else if (loadTask_->state == asset_manager::LoadTask::LoadTaskState::Failed) {
        onLoadTaskFailed(loadTask_);
      }
    });
    loadTask_->start();
  }

  void GameHost::onLoadTaskFailed(const std::shared_ptr<asset_manager::LoadTask>& task) {
    task->logErrors();
    loading_ = true;
    showScene_ = false;
    moduleFailed_ = true;
    setErrorMessage(task->collectErrors());

    auto uiFont = uiFontAsset_ ? uiFontAsset_->font : nullptr;
    if (uiFont && !loadingScreen_) {
      loadingScreen_ = std::make_shared<asset_manager::ui::LoadingScreen>(glyphBatch_, uiFont, barBatch_);
    }
    if (loadingScreen_) {
      loadingScreen_->show(task);
    }
  }

  void GameHost::setupRenderLoop() {
    render::engine->setRenderFunction([this](double time, float delta) {
      (void)time;
      (void)delta;
      asset_manager::manager.update(frame_++);

#ifdef __HOTSWAP
      pollReload();
#endif
    });

    render::engine->setSceneRenderFunction([this](render::FrameBuilder& builder, double time, float delta) {
      (void)time;
      if (loading_ || moduleFailed_ || !showScene_) {
        renderLoadingUI(builder);
        input_.beginFrame();
        return;
      }
      if (frameCallback_) {
        frameCallback_(builder, delta, input_.current());
      }
      input_.beginFrame();
    });
  }

  void GameHost::run() {
#ifdef __USE_GLFW
    render::engine->mainLoop();
#endif
  }

  void GameHost::destroy() {
#ifdef __HOTSWAP
    stopAssetWatcher();
    stopBuildOrchestrator();
#endif
    modelBackend_.clear();
    viewBackend_.clear();
#ifdef __HOTSWAP
    if (hotswap_->viewAsset) hotswap_->viewAsset->unload();
    if (hotswap_->modelAsset) hotswap_->modelAsset->unload();
#endif
    loadingScreen_.reset();
    glyphBatch_.reset();
    barBatch_.reset();
    render::destroy();
  }

  void GameHost::setFrameCallback(FrameCallback callback) {
    frameCallback_ = std::move(callback);
  }

#ifdef __HOTSWAP
  void GameHost::startBuildOrchestrator() {
    hotswap::BuildConfig buildConfig;
    buildConfig.sourceDir = config_.gameSourceDir;
    buildConfig.buildDir = config_.gameBuildDir;
    buildConfig.cmakeSourceDir = config_.cmakeSourceDir;
    buildConfig.modelSoPath = config_.modelSoPath;
    buildConfig.viewSoPath = config_.viewSoPath;
    buildConfig.manifestPath = config_.manifestPath;
    buildConfig.viewTarget = config_.viewTarget.empty() ? "view" : config_.viewTarget;
    buildConfig.modelTarget = config_.modelSoPath.empty() ? std::string() : config_.modelTarget;

    hotswap_->buildOrchestrator = std::make_shared<hotswap::BuildOrchestrator>(buildConfig);
    hotswap_->buildOrchestrator->start();
  }

  void GameHost::stopBuildOrchestrator() {
    if (hotswap_->buildOrchestrator) {
      hotswap_->buildOrchestrator->stop();
      hotswap_->buildOrchestrator.reset();
    }
  }

  void GameHost::startAssetWatcher() {
    hotswap_->assetWatcher = std::make_shared<hotswap::AssetWatcher>(config_.assetsDir);
    hotswap_->assetWatcher->registerAssetPath("logo.png", asset_manager::manager.getAsset("texture", "assets/logo.png"));
    hotswap_->assetWatcher->start();
  }

  void GameHost::stopAssetWatcher() {
    if (hotswap_->assetWatcher) {
      hotswap_->assetWatcher->stop();
      hotswap_->assetWatcher.reset();
    }
  }

  void GameHost::performReload(hotswap::ReloadAction action) {
    if (!hotswap_->viewAsset) return;

    loading_ = true;
    showScene_ = false;
    loadingScreen_->show(loadTask_);

    const bool reloadModel = action == hotswap::ReloadAction::ReloadModelAndView && hotswap_->modelAsset;

    std::string snapshot;
    std::string schema;
    if (reloadModel && modelBackend_.loaded()) {
      if (hotswap_->modelAsset->serializeFnPtr()) {
        snapshot = readFnString(hotswap_->modelAsset->serializeFnPtr(), modelBackend_.instance());
      }
      if (hotswap_->modelAsset->schemaFnPtr()) {
        schema = readFnString(hotswap_->modelAsset->schemaFnPtr(), modelBackend_.instance());
      }
    }

    modelBackend_.clear();
    viewBackend_.clear();

    if (reloadModel) {
      hotswap_->modelAsset->startReload();
    }
    hotswap_->viewAsset->startReload();

    bindBackendsFromAssets();

    if (reloadModel && modelBackend_.loaded()) {
      std::string newSchema;
      if (hotswap_->modelAsset->schemaFnPtr()) {
        newSchema = readFnString(hotswap_->modelAsset->schemaFnPtr(), modelBackend_.instance());
      }
      if (!schema.empty() && schema == newSchema && !snapshot.empty() &&
          hotswap_->modelAsset->deserializeFnPtr()) {
        hotswap_->modelAsset->deserializeFnPtr()(
          modelBackend_.instance(), snapshot.data(), snapshot.size()
        );
      } else if (hotswap_->modelAsset->initFnPtr()) {
        hotswap_->modelAsset->initFnPtr()(modelBackend_.instance());
      }
    }

    loading_ = false;
    showScene_ = true;
    moduleFailed_ = false;
    loadingScreen_->hide();
  }

  void GameHost::pollReload() {
    if (!hotswap_->manifestWatcher) {
      hotswap_->manifestWatcher = std::make_shared<hotswap::ManifestWatcher>(config_.manifestPath);
    }
    if (!hotswap_->reloadManager) {
      hotswap_->reloadManager = std::make_shared<hotswap::ReloadManager>();
      hotswap_->reloadManager->setReloadCallback([this](hotswap::ReloadAction action) {
        performReload(action);
      });
    }

    if (hotswap_->manifestWatcher->poll()) {
      auto manifest = hotswap_->manifestWatcher->readManifest();
      hotswap_->reloadManager->handleManifest(manifest);
    }
  }
#endif

}
