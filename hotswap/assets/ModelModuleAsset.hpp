#ifndef HOTSWAP_MODEL_MODULE_ASSET_HPP
#define HOTSWAP_MODEL_MODULE_ASSET_HPP

#include <Asset.hpp>
#include <cstddef>
#include <dlfcn.h>
#include <string>

namespace hotswap {

  class ModelModuleAsset : public asset_manager::TypedAsset<ModelModuleAsset> {
  public:
    void* handle = nullptr;
    void* instance = nullptr;

    using CreateFn = void* (*)();
    using DestroyFn = void (*)(void*);
    using InitFn = void (*)(void*);
    using SerializeFn = size_t (*)(void*, char**);
    using DeserializeFn = void (*)(void*, const char*, size_t);
    using SchemaFn = size_t (*)(void*, char**);

    CreateFn createFn = nullptr;
    DestroyFn destroyFn = nullptr;
    InitFn initFn = nullptr;
    SerializeFn serializeFn = nullptr;
    DeserializeFn deserializeFn = nullptr;
    SchemaFn schemaFn = nullptr;

    std::string libraryPath;

    ModelModuleAsset(std::string type, std::string name);

    void startLoadingSelf() override;
    void startReload() override;
    void unload();

    static void registerAssetType();

    CreateFn createFnPtr() const { return createFn; }
    DestroyFn destroyFnPtr() const { return destroyFn; }
    InitFn initFnPtr() const { return initFn; }
    SerializeFn serializeFnPtr() const { return serializeFn; }
    DeserializeFn deserializeFnPtr() const { return deserializeFn; }
    SchemaFn schemaFnPtr() const { return schemaFn; }
  };

}

#endif
