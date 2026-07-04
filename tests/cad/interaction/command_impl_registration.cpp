#include "tests/cad/interaction/command_registry.h"

#include "tests/cad/history/box_command.h"
#include "tests/cad/history/extrude_command.h"
#include "tests/cad/history/sketch_circle_command.h"
#include "tests/cad/history/sketch_command.h"
#include "tests/cad/history/sketch_line_command.h"

namespace toy_cad::interaction {

void register_command_impl_factories() {
  auto &registry = command_registry::instance();
  registry.register_impl_factory("box",
                                 []() { return std::make_unique<box_command>(); });
  registry.register_impl_factory(
      "extrude", []() { return std::make_unique<extrude_command>(); });
  registry.register_impl_factory(
      "sketch", []() { return std::make_unique<sketch_command>(); });
  registry.register_impl_factory(
      "sketch_line",
      []() { return std::make_unique<sketch_line_command>(); });
  registry.register_impl_factory(
      "sketch_circle",
      []() { return std::make_unique<sketch_circle_command>(); });
}

} // namespace toy_cad::interaction
