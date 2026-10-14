#include "tests/component/shader_editor_mcp.h"

#include "tests/scenes/shader_editor_scene.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

struct uniform_spec {
  const char *name;
  const char *glsl_type;
  const char *description;
};

constexpr uniform_spec k_uniforms[] = {
    {"u_time", "float", "Seconds since the program started."},
    {"u_random", "float", "A new value in [0,1) every frame."},
    {"u_resolution", "vec2", "Framebuffer size in pixels, (width, height)."},
    {"u_mouse", "vec2",
     "Cursor in [0,1], origin at the bottom-left of the viewport."},
    {"u_texture", "sampler2D",
     "The user image, bound on texture unit 0. Click the thumbnail in the "
     "panel to change it."},
};

const char k_catalog[] =
    "The fragment shader is drawn on a fullscreen quad.\n"
    "Vertex input: layout(location = 0) in vec3 aPos;  // clip space, [-1,1]\n"
    "\n"
    "Built-in uniforms, filled by the engine every frame. Declare only the\n"
    "ones the shader uses, under these exact names:\n"
    "\n"
    "uniform float u_time;        // seconds since start\n"
    "uniform float u_random;      // new value in [0,1) every frame\n"
    "uniform vec2 u_resolution;   // framebuffer size in pixels\n"
    "uniform vec2 u_mouse;        // cursor in [0,1], origin bottom-left\n"
    "uniform sampler2D u_texture; // user image on texture unit 0\n";

std::string value_of(const uniform_spec &spec,
                     const shader_editor_scene::builtin_uniform_sample &sample) {
  const std::string name = spec.name;
  if (name == "u_time") {
    return std::to_string(sample.time);
  }
  if (name == "u_random") {
    return std::to_string(sample.random);
  }
  if (name == "u_resolution") {
    return std::to_string(sample.resolution_x) + ", " +
           std::to_string(sample.resolution_y);
  }
  if (name == "u_mouse") {
    return std::to_string(sample.mouse_x) + ", " + std::to_string(sample.mouse_y);
  }
  return "unit 0, " + std::to_string(sample.texture_w) + "x" +
         std::to_string(sample.texture_h) + ", " + sample.texture_label;
}

} // namespace

shader_editor_mcp::shader_editor_mcp(shader_editor_scene *scene)
    : m_scene(scene) {}

shader_editor_mcp::~shader_editor_mcp() {
  stop_http();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

void shader_editor_mcp::start() {
  if (m_thread.joinable()) {
    return;
  }
  m_thread = std::thread([this] { serve_http(k_shader_editor_mcp_port); });
}

std::string shader_editor_mcp::get_shader() {
  return m_scene->on_render_thread([this] {
    m_scene->refresh_projects();
    return nlohmann::json{{"project", m_scene->active_project()},
                          {"vertex", m_scene->vertex_text()},
                          {"fragment", m_scene->fragment_text()}}
        .dump();
  });
}

std::string shader_editor_mcp::set_shader(std::string vertex,
                                          std::string fragment) {
  return m_scene->on_render_thread([this, vertex = std::move(vertex),
                                    fragment = std::move(fragment)] {
    std::string error;
    const bool ok = m_scene->apply_shader(vertex, fragment, &error);
    nlohmann::json reply{{"ok", ok}, {"project", m_scene->active_project()}};
    if (!ok) {
      reply["error"] = error;
    }
    return reply.dump();
  });
}

std::string shader_editor_mcp::list_projects() {
  return m_scene->on_render_thread([this] {
    m_scene->refresh_projects();
    return nlohmann::json{{"active", m_scene->active_project()},
                          {"projects", m_scene->project_names()}}
        .dump();
  });
}

std::string shader_editor_mcp::open_project(std::string name) {
  return m_scene->on_render_thread([this, name = std::move(name)] {
    std::string error;
    const bool ok = m_scene->open_project(name, &error);
    nlohmann::json reply{{"ok", ok}, {"project", m_scene->active_project()}};
    if (!ok) {
      reply["error"] = error;
    }
    return reply.dump();
  });
}

std::string shader_editor_mcp::builtin_uniforms() {
  return m_scene->on_render_thread([this] {
    const auto sample = m_scene->sample_builtin_uniforms();
    nlohmann::json items = nlohmann::json::array();
    for (const uniform_spec &spec : k_uniforms) {
      items.push_back({{"name", spec.name},
                       {"glsl_type", spec.glsl_type},
                       {"description", spec.description},
                       {"value", value_of(spec, sample)}});
    }
    return nlohmann::json{{"uniforms", std::move(items)}}.dump();
  });
}

mcp::content shader_editor_mcp::capture_preview() {
  const shader_editor_scene::preview shot = m_scene->capture_preview();
  nlohmann::json reply{{"ok", shot.ok},
                       {"width", shot.width},
                       {"height", shot.height},
                       {"path", shot.path},
                       {"mean_rgb", shot.mean_rgb},
                       {"grid", shot.grid}};
  mcp::content out;
  if (!shot.ok) {
    reply["error"] = shot.error;
    out.add(mcp::text{reply.dump()});
    return out;
  }
  std::ifstream file(shot.path, std::ios::binary);
  std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),
                                  std::istreambuf_iterator<char>()};
  out.add(mcp::text{reply.dump()});
  out.add(mcp::image{"image/png", std::move(bytes)});
  return out;
}

std::string shader_editor_mcp::uniform_catalog() { return k_catalog; }
