#include "shader_helper.h"

#include <iostream>

shader *load_shader(const std::string &vertex_path,
                    const std::string &fragment_path) {
  try {
    return new shader(vertex_path.c_str(), fragment_path.c_str());
  } catch (const std::exception &e) {
    std::cerr << "Failed to load shader: " << e.what() << std::endl;
    return nullptr;
  }
}

void load_shader_pair(const std::string &vertex_path,
                      const std::string &fragment_path,
                      const std::string &light_fragment_path, shader *&main_shader,
                      shader *&light_shader) {
  main_shader = load_shader(vertex_path, fragment_path);
  light_shader = load_shader(vertex_path, light_fragment_path);
}

