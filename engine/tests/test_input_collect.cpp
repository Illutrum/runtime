#include <doctest/doctest.h>
#include <engine/InputCollector.hpp>

TEST_CASE("InputCollector tracks key presses") {
  engine::InputCollector collector;
  collector.beginFrame();
  collector.keyEvent(87, 1);
  CHECK(collector.current().keys.test(87));
}

TEST_CASE("InputCollector reset on R key") {
  engine::InputCollector collector;
  collector.beginFrame();
  collector.keyEvent(82, 1);
  CHECK(collector.current().resetRequested);
}
