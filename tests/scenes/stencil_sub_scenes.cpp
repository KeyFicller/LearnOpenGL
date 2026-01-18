#include "stencil_sub_scenes.h"
#include "stencil_test_scene.h"

#include "glad/gl.h"
#include "imgui.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// stencil_object_outline_sub_scene
// ============================================================================

stencil_object_outline_sub_scene::stencil_object_outline_sub_scene(
    stencil_test_scene *parent)
    : sub_scene(parent, "Object Outline") {}

void stencil_object_outline_sub_scene::render() {
  auto *p = parent();
  if (!p->m_shader) {
    return;
  }

  // Configure stencil test for writing
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilMask(0xFF);

  // Draw normal object and write to stencil buffer
  p->m_shader->use();
  p->set_matrices(p->m_shader,
                  glm::translate(glm::mat4(1.0f), m_object_position));
  p->m_shader->set_uniform<float, 3>("uLightColor", &p->m_light_color.x);
  p->m_shader->set_uniform<float, 3>("uObjectColor", &p->m_object_color.x);
  p->m_shader->set_uniform<float, 1>("uAmbientStrength",
                                     &p->m_ambient_strength);
  p->m_shader->set_uniform<float, 3>("uLightPosition", &p->m_light_position.x);
  p->m_shader->set_uniform<float, 1>("uSpecularStrength",
                                     &p->m_specular_strength);
  p->m_shader->set_uniform<float, 3>("uEyePosition", &p->m_camera.m_position.x);
  p->m_shader->set_uniform<float, 1>("uShininess", &p->m_shininess);
  p->m_shader->set_uniform("uDrawBoundaries", 0); // false = 0
  p->m_mesh.draw();

  // Draw boundary outline using stencil test
  glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
  glStencilMask(0x00); // Disable writing to stencil buffer
  glDisable(GL_DEPTH_TEST);

  p->set_matrices(p->m_shader,
                  glm::translate(glm::mat4(1.0f), m_object_position) *
                      glm::scale(glm::mat4(1.0f), glm::vec3(m_boundary_scale)));
  p->m_shader->set_uniform("uDrawBoundaries", 1); // true = 1
  p->m_shader->set_uniform("uBoundaryColor", m_boundary_color);
  p->m_mesh.draw();

  glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF); // Re-enable stencil writing for next frame
}

void stencil_object_outline_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Object Outline Parameters");
  ImGui::SliderFloat3("Object Position", &m_object_position.x, -10.0f, 10.0f,
                      "%.2f");
  ImGui::SliderFloat("Boundary Scale", &m_boundary_scale, 1.0f, 1.2f, "%.3f");
  ImGui::ColorEdit3("Boundary Color", &m_boundary_color.x);
}

// ============================================================================
// stencil_mirror_sub_scene
// ============================================================================

stencil_mirror_sub_scene::stencil_mirror_sub_scene(stencil_test_scene *parent)
    : sub_scene(parent, "Mirror") {}

void stencil_mirror_sub_scene::render() {
  auto *p = parent();
  if (!p->m_shader) {
    return;
  }

  // Step 1: Draw scene normally (objects that will be reflected)
  glStencilMask(0x00); // Disable writing to stencil buffer
  p->m_shader->use();
  p->set_matrices(p->m_shader,
                  glm::translate(glm::mat4(1.0f), m_object_position));
  p->m_shader->set_uniform<float, 3>("uLightColor", &p->m_light_color.x);
  p->m_shader->set_uniform<float, 3>("uObjectColor", &p->m_object_color.x);
  p->m_shader->set_uniform<float, 1>("uAmbientStrength",
                                     &p->m_ambient_strength);
  p->m_shader->set_uniform<float, 3>("uLightPosition", &p->m_light_position.x);
  p->m_shader->set_uniform<float, 1>("uSpecularStrength",
                                     &p->m_specular_strength);
  p->m_shader->set_uniform<float, 3>("uEyePosition", &p->m_camera.m_position.x);
  p->m_shader->set_uniform<float, 1>("uShininess", &p->m_shininess);
  p->m_shader->set_uniform("uDrawBoundaries", 0);
  p->m_mesh.draw();

  // Step 2: Draw mirror plane to stencil buffer (write stencil only, no
  // color/depth)
  glStencilFunc(GL_ALWAYS, 1, 0xFF);
  glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
  glStencilMask(0xFF); // Enable writing to stencil buffer
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE,
              GL_FALSE); // Disable color writing
  glDepthMask(GL_FALSE); // Disable depth writing

  // Draw mirror plane (using cube as placeholder - should be quad in future)
  glm::mat4 mirror_model = glm::translate(glm::mat4(1.0f), m_mirror_position) *
                           glm::scale(glm::mat4(1.0f), m_mirror_scale);
  p->set_matrices(p->m_shader, mirror_model);
  p->m_shader->set_uniform("uObjectColor", glm::vec3(0.5f, 0.5f, 0.5f));
  p->m_shader->set_uniform("uDrawBoundaries", 0);
  p->m_mesh.draw(); // Actually draw the mirror plane to stencil buffer

  // Restore color and depth masks, disable stencil writing
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // Re-enable color writing
  glDepthMask(GL_TRUE);                            // Re-enable depth writing
  glStencilMask(0x00); // Disable writing to stencil buffer

  // Step 3: Draw reflected objects (clipped by stencil, with reflection
  // transform)
  glStencilFunc(GL_EQUAL, 1, 0xFF); // Only draw where stencil == 1

  // Calculate reflection matrix (reflect across X axis for YZ plane)
  glm::mat4 reflection_matrix = glm::mat4(1.0f);
  reflection_matrix[0][0] = -1.0f; // Reflect across X axis

  // Draw reflected object
  glm::mat4 reflected_model =
      glm::translate(glm::mat4(1.0f), m_mirror_position) * reflection_matrix *
      glm::translate(glm::mat4(1.0f), -m_mirror_position) *
      glm::translate(glm::mat4(1.0f), m_object_position);

  p->set_matrices(p->m_shader, reflected_model);
  p->m_shader->set_uniform<float, 3>("uLightColor", &p->m_light_color.x);
  p->m_shader->set_uniform<float, 3>("uObjectColor", &p->m_object_color.x);
  p->m_shader->set_uniform<float, 1>("uAmbientStrength",
                                     &p->m_ambient_strength);
  p->m_shader->set_uniform<float, 3>("uLightPosition", &p->m_light_position.x);
  p->m_shader->set_uniform<float, 1>("uSpecularStrength",
                                     &p->m_specular_strength);
  p->m_shader->set_uniform<float, 3>("uEyePosition", &p->m_camera.m_position.x);
  p->m_shader->set_uniform<float, 1>("uShininess", &p->m_shininess);
  p->m_shader->set_uniform("uDrawBoundaries", 0);
  // Scale down color to simulate reflection
  glm::vec3 reflected_color = p->m_object_color * 0.6f;
  p->m_shader->set_uniform<float, 3>("uObjectColor", &reflected_color.x);
  p->m_mesh.draw();

  // Step 4: Draw mirror plane surface (with transparency, clipped by stencil)
  glStencilFunc(GL_EQUAL, 1, 0xFF);       // Only draw where stencil == 1
  glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); // Don't modify stencil
  glDisable(GL_DEPTH_TEST); // Disable depth test for mirror surface
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  p->set_matrices(p->m_shader, mirror_model);
  p->m_shader->set_uniform("uObjectColor",
                           glm::vec3(0.8f, 0.8f, 0.8f)); // Mirror surface color
  p->m_shader->set_uniform("uDrawBoundaries", 0);
  p->m_mesh.draw(); // Draw mirror surface

  // Restore state
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  glStencilMask(0xFF); // Re-enable stencil writing for next frame
}

void stencil_mirror_sub_scene::render_ui() {
  ImGui::Separator();
  ImGui::Text("Mirror Parameters");
  ImGui::SliderFloat3("Object Position", &m_object_position.x, -10.0f, 10.0f,
                      "%.2f");
  ImGui::SliderFloat3("Mirror Position", &m_mirror_position.x, -10.0f, 10.0f,
                      "%.2f");
  ImGui::SliderFloat3("Mirror Scale", &m_mirror_scale.x, 0.1f, 10.0f, "%.2f");
  ImGui::SliderFloat("Mirror Alpha", &m_mirror_alpha, 0.0f, 1.0f, "%.2f");
}
