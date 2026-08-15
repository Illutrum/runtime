#ifndef ENGINE_BACKEND_HPP
#define ENGINE_BACKEND_HPP

namespace engine {

  class ModelBackend {
  public:
    ModelBackend() : instance_(nullptr) {}

    void attach(void* instance) {
      instance_ = instance;
    }

    void clear() {
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
  };

  class ViewBackend {
  public:
    ViewBackend() : instance_(nullptr) {}

    void attach(void* instance) {
      instance_ = instance;
    }

    void clear() {
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
  };

}

#endif
