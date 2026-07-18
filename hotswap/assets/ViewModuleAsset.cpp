#include "ViewModuleAsset.hpp"

#include <iostream>
#include <stdexcept>

namespace hotswap {

  namespace {
    struct ViewModuleAssetRegistrar {
      ViewModuleAssetRegistrar() { ViewModuleAsset::registerAssetType(); }
    };
    ViewModuleAssetRegistrar viewModuleAssetRegistrar;
  }

  void ViewModuleAsset::registerAssetType() {
    asset_manager::Asset::registerType<ViewModuleAsset>("viewModule");
  }

  ViewModuleAsset::ViewModuleAsset(std::string type, std::string name)
    : TypedAsset<ViewModuleAsset>(type, name), libraryPath(name) {}

  void ViewModuleAsset::unload() {
    if (instance && destroyFn) {
      destroyFn(instance);
      instance = nullptr;
    }
    if (handle) {
      dlclose(handle);
      handle = nullptr;
    }
    createFn = nullptr;
    destroyFn = nullptr;
    initFn = nullptr;
  }

  void ViewModuleAsset::startLoadingSelf() {
    unload();
    asset_manager::logLoad("module", libraryPath);
    handle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
      const char* err = dlerror();
      error = std::make_exception_ptr(std::runtime_error(err ? err : "dlopen failed"));
      handleLoadingFailed();
      return;
    }

    createFn = reinterpret_cast<CreateFn>(dlsym(handle, "view_create"));
    destroyFn = reinterpret_cast<DestroyFn>(dlsym(handle, "view_destroy"));
    initFn = reinterpret_cast<InitFn>(dlsym(handle, "view_init"));

    if (!createFn || !destroyFn) {
      error = std::make_exception_ptr(std::runtime_error("view_create/view_destroy not found"));
      handleLoadingFailed();
      return;
    }

    try {
      instance = createFn();
      if (initFn && instance) {
        initFn(instance);
      }
      handleLoadingFinished();
    } catch (...) {
      error = std::current_exception();
      handleLoadingFailed();
    }
  }

  void ViewModuleAsset::startReload() {
    handleReloadStarted();
    startLoadingSelf();
  }

}
