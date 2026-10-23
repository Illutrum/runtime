#include "Model.hpp"
#include "GameState.hpp"
#include "Input.hpp"

#include <cstring>
#include <cstdlib>
#include <string>

namespace {

  // The reflection codec in rpc-cpp needs the reflection compiler.
  // This sample stores the state bytes so the host builds with the system compiler.
  constexpr const char* kSchema = "GameState";

  std::string registryChecksum() {
    return kSchema;
  }

  size_t copyOut(const std::string& bytes, char** out) {
    *out = static_cast<char*>(std::malloc(bytes.size()));
    if (*out && !bytes.empty()) {
      std::memcpy(*out, bytes.data(), bytes.size());
    }
    return bytes.size();
  }

}

class ExampleModel : public Model {
public:
  void init() override {
    state_ = GameState{};
  }

  void update(const Input& input, float delta) override {
    state_.x += input.moveX * state_.speed * delta;
    state_.y += input.moveY * state_.speed * delta;
    state_.counter++;
  }

  const void* snapshot() override {
    return &state_;
  }

  std::string serialize() override {
    std::string bytes(sizeof(GameState), '\0');
    std::memcpy(bytes.data(), &state_, sizeof(GameState));
    return bytes;
  }

  void deserialize(const std::string& bytes) override {
    if (bytes.size() != sizeof(GameState)) {
      return;
    }
    std::memcpy(&state_, bytes.data(), sizeof(GameState));
  }

  std::string schemaChecksum() override {
    return registryChecksum();
  }

private:
  GameState state_{};
};

extern "C" {

  void* model_create() {
    return new ExampleModel();
  }

  void model_destroy(void* instance) {
    delete static_cast<ExampleModel*>(static_cast<Model*>(instance));
  }

  void model_init(void* instance) {
    static_cast<Model*>(instance)->init();
  }

  size_t model_serialize(void* instance, char** out) {
    auto bytes = static_cast<Model*>(instance)->serialize();
    return copyOut(bytes, out);
  }

  void model_deserialize(void* instance, const char* data, size_t length) {
    static_cast<Model*>(instance)->deserialize(std::string(data, length));
  }

  size_t model_schema_checksum(void* instance, char** out) {
    (void)instance;
    auto bytes = registryChecksum();
    return copyOut(bytes, out);
  }

}
