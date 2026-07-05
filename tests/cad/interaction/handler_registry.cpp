#include "handler_registry.h"

#include "extrude_depth_input_handler.h"
#include "face_input_handler.h"
#include "profile_input_handler.h"
#include "flow_command.h"
#include "point_input_handler.h"
#include "sketch_plane_input_handler.h"
#include "sketch_point_input_handler.h"

namespace toy_cad::interaction {

handler_registry &handler_registry::instance() {
  static handler_registry s;
  return s;
}

bool handler_registry::is_known_type(const std::string &type) const {
  return type == "point" || type == "face" || type == "profile" ||
         type == "extrude_depth" || type == "sketch_plane" ||
         type == "sketch_point";
}

std::unique_ptr<input_handler>
handler_registry::create(const std::string &type, flow_command *owner,
                         const std::string &flag) {
  std::unique_ptr<input_handler> handler;
  if (type == "point") {
    handler = std::make_unique<point_input_handler>();
  } else if (type == "face") {
    handler = std::make_unique<face_input_handler>();
  } else if (type == "profile") {
    handler = std::make_unique<profile_input_handler>();
  } else if (type == "extrude_depth") {
    handler = std::make_unique<extrude_depth_input_handler>();
  } else if (type == "sketch_plane") {
    handler = std::make_unique<sketch_plane_input_handler>();
  } else if (type == "sketch_point") {
    handler = std::make_unique<sketch_point_input_handler>();
  } else {
    return nullptr;
  }

  if (owner && handler) {
    handler->set_owner(owner, flag.c_str());
  }
  return handler;
}

} // namespace toy_cad::interaction
