#ifndef ENGINE_BACKEND_HPP
#define ENGINE_BACKEND_HPP

#include <functional>
#include <string>

namespace engine {

  using ModelCreateFn = void* (*)();
  using ModelDestroyFn = void (*)(void*);
  using ViewCreateFn = void* (*)();
  using ViewDestroyFn = void (*)(void*);

  class ModelBackend {
  public:
    ModelBackend() : instance_(nullptr), createFn_(nullptr), destroyFn_(nullptr) {}

    void set(void* instance, ModelCreateFn createFn, ModelDestroyFn destroyFn) {
      destroyCurrent();
      instance_ = instance;
      createFn_ = createFn;
      destroyFn_ = destroyFn;
    }

    void destroyCurrent() {
      if (instance_ && destroyFn_) {
        destroyFn_(instance_);
      }
      instance_ = nullptr;
    }

    void* instance() const { return instance_; }

    template<typename T>
    T* as() const {
      return static_cast<T*>(instance_);
    }

    bool loaded() const { return instance_ != nullptr; }

  private:
    void* instance_;
    ModelCreateFn createFn_;
    ModelDestroyFn destroyFn_;
  };

  class ViewBackend {
  public:
    ViewBackend() : instance_(nullptr), createFn_(nullptr), destroyFn_(nullptr) {}

    void set(void* instance, ViewCreateFn createFn, ViewDestroyFn destroyFn) {
      destroyCurrent();
      instance_ = instance;
      createFn_ = createFn;
      destroyFn_ = destroyFn;
    }

    void destroyCurrent() {
      if (instance_ && destroyFn_) {
        destroyFn_(instance_);
      }
      instance_ = nullptr;
    }

    void* instance() const { return instance_; }

    template<typename T>
    T* as() const {
      return static_cast<T*>(instance_);
    }

    bool loaded() const { return instance_ != nullptr; }

  private:
    void* instance_;
    ViewCreateFn createFn_;
    ViewDestroyFn destroyFn_;
  };

}

#endif
