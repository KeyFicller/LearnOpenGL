#pragma once

#include "glm/glm.hpp"
#include "imgui.h"
#include "shader.h"

struct material {
public:
  glm::vec3 m_ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_specular = glm::vec3(1.0f, 1.0f, 1.0f);
  float m_shininess = 32.0f;
};

inline void uniform(shader &_shader, const material &_material,
                    std::string _name) {
  _shader.set_uniform<glm::vec3, 1>((_name + ".ambient").c_str(),
                                    &_material.m_ambient);
  _shader.set_uniform<glm::vec3, 1>((_name + ".diffuse").c_str(),
                                    &_material.m_diffuse);
  _shader.set_uniform<glm::vec3, 1>((_name + ".specular").c_str(),
                                    &_material.m_specular);
  _shader.set_uniform<float, 1>((_name + ".shininess").c_str(),
                                &_material.m_shininess);
}

inline void ui(material &_material) {
  ImGui::SliderFloat3("Ambient", &_material.m_ambient.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Diffuse", &_material.m_diffuse.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Specular", &_material.m_specular.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat("Shininess", &_material.m_shininess, 1.0f, 128.0f, "%.0f");
}