#ifndef TEMPLATE_MODEL_HPP
#define TEMPLATE_MODEL_HPP

#include "Input.hpp"
#include <string>

class Model {
public:
  virtual ~Model() {}
  virtual void init() = 0;
  virtual void update(const Input& input, float delta) = 0;
  virtual const void* snapshot() = 0;
  virtual std::string serialize() = 0;
  virtual void deserialize(const std::string& bytes) = 0;
  virtual std::string schemaChecksum() = 0;
};

#endif
