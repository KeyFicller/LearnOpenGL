#pragma once

#include "glm/glm.hpp"
#include "imgui.h"
#include "shader.h"

struct light {
public:
  glm::vec3 m_position = glm::vec3(1.0f, 1.0f, 0.0f);
  glm::vec3 m_ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_specular = glm::vec3(1.0f, 1.0f, 1.0f);
};

inline void uniform(shader &_shader, const light &_light, std::string _name) {
  _shader.set_uniform<glm::vec3, 1>((_name + ".position").c_str(),
                                    &_light.m_position);
  _shader.set_uniform<glm::vec3, 1>((_name + ".ambient").c_str(),
                                    &_light.m_ambient);
  _shader.set_uniform<glm::vec3, 1>((_name + ".diffuse").c_str(),
                                    &_light.m_diffuse);
  _shader.set_uniform<glm::vec3, 1>((_name + ".specular").c_str(),
                                    &_light.m_specular);
}

inline void ui(light &_light) {
  ImGui::SliderFloat3("Ambient", &_light.m_ambient.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Diffuse", &_light.m_diffuse.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Specular", &_light.m_specular.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Position", &_light.m_position.x, -10.0f, 10.0f, "%.2f");
}