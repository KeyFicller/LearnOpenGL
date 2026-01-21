#include "basic/light.h"

#include "glm/glm.hpp"
#include "imgui.h"

// -----------------------------------------------------------------------------
void uniform(shader &_shader, const light &_light, std::string _name) {
  _shader.set_uniform((_name + ".type").c_str(), static_cast<int>(_light.Type));
  _shader.set_uniform((_name + ".ambient").c_str(), _light.Ambient);
  _shader.set_uniform((_name + ".diffuse").c_str(), _light.Diffuse);
  _shader.set_uniform((_name + ".specular").c_str(), _light.Specular);

  // Always set all fields to ensure shader receives valid values
  _shader.set_uniform((_name + ".position").c_str(), _light.Position);
  _shader.set_uniform((_name + ".direction").c_str(), _light.Direction);
  _shader.set_uniform((_name + ".constant").c_str(), _light.Constant);
  _shader.set_uniform((_name + ".linear").c_str(), _light.Linear);
  _shader.set_uniform((_name + ".quadratic").c_str(), _light.Quadratic);

  // Convert cutoff angles (degrees) to cos(radians) for shader
  float cutoff_cos = glm::cos(glm::radians(_light.Cutoff));
  float outer_cutoff_cos = glm::cos(glm::radians(_light.OuterCutoff));
  _shader.set_uniform((_name + ".cutoff").c_str(), cutoff_cos);
  _shader.set_uniform((_name + ".outer_cutoff").c_str(), outer_cutoff_cos);
}

// -----------------------------------------------------------------------------
void ui(light &_light) {
  ImGui::Combo("Type", &(int &)_light.Type, "Directional\0Point\0Spot\0");

  ImGui::SliderFloat3("Ambient", &_light.Ambient.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Diffuse", &_light.Diffuse.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Specular", &_light.Specular.x, 0.0f, 1.0f, "%.2f");

  switch (_light.Type) {
  case light_type::k_directional:
    ImGui::SliderFloat3("Direction", &_light.Direction.x, -1.0f, 1.0f, "%.2f");
    break;
  case light_type::k_point:
    ImGui::SliderFloat3("Position", &_light.Position.x, -10.0f, 10.0f, "%.2f");
    ImGui::SliderFloat("Constant", &_light.Constant, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Linear", &_light.Linear, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Quadratic", &_light.Quadratic, 0.0f, 1.0f, "%.2f");
    break;
  case light_type::k_spot:
    ImGui::SliderFloat3("Position", &_light.Position.x, -10.0f, 10.0f, "%.2f");
    ImGui::SliderFloat3("Direction", &_light.Direction.x, -1.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Cutoff (degrees)", &_light.Cutoff, 0.0f, 90.0f, "%.1f");
    ImGui::SliderFloat("Outer Cutoff (degrees)", &_light.OuterCutoff, 0.0f,
                       90.0f, "%.1f");
    break;
  }
}
