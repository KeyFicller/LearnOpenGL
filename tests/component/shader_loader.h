#pragma once

#include "basic/shader.h"
#include <string>

// Helper function to load shader with error handling
shader *load_shader(const std::string &vertex_path,
                    const std::string &fragment_path);

// Helper function to load shader pair (main shader + light shader)
void load_shader_pair(const std::string &vertex_path,
                      const std::string &fragment_path,
                      const std::string &light_fragment_path, shader *&main_shader,
                      shader *&light_shader);

