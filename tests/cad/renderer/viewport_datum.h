#pragma once

#include <glm/glm.hpp>

class shader;
class mesh_manager;

/** Global singleton: unit datum quad; placement via `mvp`. */
class viewport_datum {
public:
  static viewport_datum &instance();

  viewport_datum(const viewport_datum &) = delete;
  viewport_datum &operator=(const viewport_datum &) = delete;

  void draw(shader &shader_prog, const glm::mat4 &mvp, const glm::vec4 &color);

  ~viewport_datum();

private:
  viewport_datum();

  mesh_manager *m_quad = nullptr;
};
