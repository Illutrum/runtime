#ifndef HOTSWAP_VIEW_MODULE_ASSET_HPP
#define HOTSWAP_VIEW_MODULE_ASSET_HPP

#include <Asset.hpp>
#include <dlfcn.h>
#include <string>

namespace hotswap {

  class ViewModuleAsset : public asset_manager::TypedAsset<ViewModuleAsset> {
  public:
    void* handle = nullptr;
    void* instance = nullptr;

    using CreateFn = void* (*)();
    using DestroyFn = void (*)(void*);
    using InitFn = void (*)(void*);

    CreateFn createFn = nullptr;
    DestroyFn destroyFn = nullptr;
    InitFn initFn = nullptr;

    std::string libraryPath;

    ViewModuleAsset(std::string type, std::string name);

    void startLoadingSelf() override;
    void startReload() override;
    void unload();

    static void registerAssetType();

    CreateFn createFnPtr() const { return createFn; }
    DestroyFn destroyFnPtr() const { return destroyFn; }
    InitFn initFnPtr() const { return initFn; }
  };

}

#endif
