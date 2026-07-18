#include "Model.hpp"
#include "GameState.hpp"
#include "Input.hpp"

#include <cstring>
#include <cstdlib>

class StubModel : public Model {
public:
  void init() override { state_ = {}; }
  void update(const Input& input, float delta) override {
    state_.x += input.moveX * delta;
    state_.y += input.moveY * delta;
  }
  const void* snapshot() override { return &state_; }
  std::string serialize() override { return {}; }
  void deserialize(const std::string&) override {}
  std::string schemaChecksum() override { return "stub-schema"; }

private:
  GameState state_{};
};

extern "C" {
  void* model_create() { return new StubModel(); }
  void model_destroy(void* instance) { delete static_cast<StubModel*>(static_cast<Model*>(instance)); }
  void model_init(void* instance) { static_cast<Model*>(instance)->init(); }
  size_t model_serialize(void*, char**) { return 0; }
  void model_deserialize(void*, const char*, size_t) {}
  size_t model_schema_checksum(void*, char** out) {
    const char* value = "stub-schema";
    *out = static_cast<char*>(std::malloc(11));
    std::memcpy(*out, value, 11);
    return 11;
  }
}
