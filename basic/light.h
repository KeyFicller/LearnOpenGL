#pragma once

#include "glm/glm.hpp"
#include "imgui.h"
#include "shader.h"

enum class light_type : int {
  k_directional,
  k_point,
  k_spot,
};

struct light {
public:
  glm::vec3 m_ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_specular = glm::vec3(1.0f, 1.0f, 1.0f);

  light_type m_type = light_type::k_point;

  glm::vec3 m_direction = glm::vec3(0.0f, 0.0f, -1.0f);

  glm::vec3 m_position = glm::vec3(1.0f, 1.0f, 0.0f);
  float m_constant = 1.0f;
  float m_linear = 0.09f;
  float m_quadratic = 0.032f;

  float m_cutoff = glm::cos(glm::radians(12.5f));
  float m_outer_cutoff = glm::cos(glm::radians(17.5f));
};

inline void uniform(shader &_shader, const light &_light, std::string _name) {

  _shader.set_uniform<int, 1>((_name + ".type").c_str(),
                              &((int &)_light.m_type));
  _shader.set_uniform<glm::vec3, 1>((_name + ".ambient").c_str(),
                                    &_light.m_ambient);
  _shader.set_uniform<glm::vec3, 1>((_name + ".diffuse").c_str(),
                                    &_light.m_diffuse);
  _shader.set_uniform<glm::vec3, 1>((_name + ".specular").c_str(),
                                    &_light.m_specular);

  // Always set all fields to ensure shader receives valid values
  _shader.set_uniform<glm::vec3, 1>((_name + ".position").c_str(),
                                    &_light.m_position);
  _shader.set_uniform<glm::vec3, 1>((_name + ".direction").c_str(),
                                    &_light.m_direction);
  _shader.set_uniform<float, 1>((_name + ".constant").c_str(),
                                &_light.m_constant);
  _shader.set_uniform<float, 1>((_name + ".linear").c_str(), &_light.m_linear);
  _shader.set_uniform<float, 1>((_name + ".quadratic").c_str(),
                                &_light.m_quadratic);
  _shader.set_uniform<float, 1>((_name + ".cutoff").c_str(), &_light.m_cutoff);
  _shader.set_uniform<float, 1>((_name + ".outer_cutoff").c_str(),
                                &_light.m_outer_cutoff);
}

inline void ui(light &_light) {

  ImGui::Combo("Type", &(int &)_light.m_type, "Directional\0Point\0Spot\0");

  ImGui::SliderFloat3("Ambient", &_light.m_ambient.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Diffuse", &_light.m_diffuse.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Specular", &_light.m_specular.x, 0.0f, 1.0f, "%.2f");

  switch (_light.m_type) {
  case light_type::k_directional:
    ImGui::SliderFloat3("Direction", &_light.m_direction.x, -1.0f, 1.0f,
                        "%.2f");
    break;
  case light_type::k_point:
    ImGui::SliderFloat3("Position", &_light.m_position.x, -10.0f, 10.0f,
                        "%.2f");
    ImGui::SliderFloat("Constant", &_light.m_constant, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Linear", &_light.m_linear, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Quadratic", &_light.m_quadratic, 0.0f, 1.0f, "%.2f");
    break;
  case light_type::k_spot:
    ImGui::SliderFloat3("Position", &_light.m_position.x, -10.0f, 10.0f,
                        "%.2f");
    ImGui::SliderFloat3("Direction", &_light.m_direction.x, -1.0f, 1.0f,
                        "%.2f");
    ImGui::SliderFloat("Cutoff", &_light.m_cutoff, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Outer Cutoff", &_light.m_outer_cutoff, 0.0f, 1.0f,
                       "%.2f");
    break;
  }
}