#include "ModelModuleAsset.hpp"

#include <AssetsManager.hpp>
#include <iostream>
#include <stdexcept>

namespace hotswap {

  namespace {
    struct ModelModuleAssetRegistrar {
      ModelModuleAssetRegistrar() { ModelModuleAsset::registerAssetType(); }
    };
    ModelModuleAssetRegistrar modelModuleAssetRegistrar;
  }

  void ModelModuleAsset::registerAssetType() {
    asset_manager::Asset::registerType<ModelModuleAsset>("modelModule");
  }

  ModelModuleAsset::ModelModuleAsset(std::string type, std::string name)
    : TypedAsset<ModelModuleAsset>(type, name), libraryPath(name) {}

  void ModelModuleAsset::unload() {
    if (instance) {
      if (destroyFn) destroyFn(instance);
      instance = nullptr;
    }
    asset_manager::manager.dropLiveRequests();
    if (handle) {
      dlclose(handle);
      handle = nullptr;
    }
    createFn = nullptr;
    destroyFn = nullptr;
    initFn = nullptr;
    serializeFn = nullptr;
    deserializeFn = nullptr;
    schemaFn = nullptr;
  }

  void ModelModuleAsset::startLoadingSelf() {
    unload();
    asset_manager::logLoad("module", libraryPath);
    handle = dlopen(libraryPath.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
      const char* err = dlerror();
      error = std::make_exception_ptr(std::runtime_error(err ? err : "dlopen failed"));
      handleLoadingFailed();
      return;
    }

    createFn = reinterpret_cast<CreateFn>(dlsym(handle, "model_create"));
    destroyFn = reinterpret_cast<DestroyFn>(dlsym(handle, "model_destroy"));
    initFn = reinterpret_cast<InitFn>(dlsym(handle, "model_init"));
    serializeFn = reinterpret_cast<SerializeFn>(dlsym(handle, "model_serialize"));
    deserializeFn = reinterpret_cast<DeserializeFn>(dlsym(handle, "model_deserialize"));
    schemaFn = reinterpret_cast<SchemaFn>(dlsym(handle, "model_schema_checksum"));

    if (!createFn || !destroyFn) {
      error = std::make_exception_ptr(std::runtime_error("model_create/model_destroy not found"));
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

  void ModelModuleAsset::startReload() {
    handleReloadStarted();
    startLoadingSelf();
  }

}
