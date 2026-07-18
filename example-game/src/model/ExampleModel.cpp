#include "Model.hpp"
#include "GameState.hpp"
#include "Input.hpp"

#include <serialization/deserialize.hpp>
#include <serialization/serialize.hpp>
#include <serialization/stream_reader.hpp>
#include <serialization/stream_writer.hpp>
#include <serialization/type_registry.hpp>

#include <cstring>
#include <memory>
#include <sstream>
#include <string>

namespace {

  serialization::type_registry& gameRegistry() {
    static serialization::type_registry registry(false);
    static bool initialized = false;
    if (!initialized) {
      registry.register_type(serialization::type_description::create(GameState{}, registry));
      registry.register_type(serialization::type_description::create(Input{}, registry));
      initialized = true;
    }
    return registry;
  }

  std::string registryChecksum() {
    std::ostringstream output;
    serialization::stream_writer writer(output);
    writer.register_serialize_typed_function<serialization::type_description>();
    serialization::serialize(writer, gameRegistry());
    writer.write_rest();
    return output.str();
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
    std::ostringstream output;
    serialization::stream_writer writer(output);
    writer.register_serialize_typed_function<GameState>();
    serialization::serialize(writer, state_);
    writer.write_rest();
    return output.str();
  }

  void deserialize(const std::string& bytes) override {
    std::istringstream input(bytes);
    serialization::stream_reader reader(input);
    reader.register_deserialize_typed_function<GameState>();
    serialization::deserialize(reader, state_);
    reader.read_rest();
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
