#include <doctest/doctest.h>
#include <engine/InputCollector.hpp>

TEST_CASE("InputCollector tracks key presses") {
  engine::InputCollector collector;
  collector.keyEvent(87, 1);
  CHECK(collector.current().keys.test(87));
  CHECK(collector.current().keysJustPressed.test(87));
}

TEST_CASE("InputCollector reset on R key") {
  engine::InputCollector collector;
  collector.keyEvent(82, 1);
  CHECK(collector.current().resetRequested);
  collector.beginFrame();
  CHECK_FALSE(collector.current().resetRequested);
}

TEST_CASE("beginFrame clears edge bits after consume") {
  engine::InputCollector collector;
  collector.keyEvent(90, 1);
  CHECK(collector.current().keysJustPressed.test(90));
  CHECK(collector.current().keys.test(90));
  collector.beginFrame();
  CHECK_FALSE(collector.current().keysJustPressed.test(90));
  CHECK(collector.current().keys.test(90));
}

TEST_CASE("held key does not re-set keysJustPressed") {
  engine::InputCollector collector;
  collector.keyEvent(72, 1);
  CHECK(collector.current().keysJustPressed.test(72));
  collector.beginFrame();
  collector.keyEvent(72, 1);
  CHECK_FALSE(collector.current().keysJustPressed.test(72));
  CHECK(collector.current().keys.test(72));
}

TEST_CASE("press after beginFrame is visible this tick") {
  engine::InputCollector collector;
  collector.beginFrame();
  collector.keyEvent(88, 1);
  CHECK(collector.current().keysJustPressed.test(88));
  collector.beginFrame();
  CHECK_FALSE(collector.current().keysJustPressed.test(88));
}
