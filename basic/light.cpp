#include "basic/light.h"
#include "basic/imgui_form.h"

#include "glm/glm.hpp"
#include "imgui.h"

namespace {

void apply_simple_light_terms(light &_light) {
  const glm::vec3 base = _light.SimpleColor * _light.SimpleIntensity;
  _light.Ambient = base * 0.22f;
  _light.Diffuse = base;
  _light.Specular = base * 0.45f;
}

void light_radiance_ui(light &_light) {
  ImGui::PushItemWidth(imgui_form_item_width());
  if (_light.SimpleLighting) {
    ImGui::ColorEdit3("Light color", &_light.SimpleColor.x);
    ImGui::SliderFloat("Intensity", &_light.SimpleIntensity, 0.0f, 3.0f, "%.2f");
    if (ImGui::IsItemHovered()) {
      ImGui::SetTooltip("Scales ambient, diffuse, and specular together.");
    }
    apply_simple_light_terms(_light);
  } else {
    ImGui::SliderFloat3("Ambient", &_light.Ambient.x, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat3("Diffuse", &_light.Diffuse.x, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat3("Specular", &_light.Specular.x, 0.0f, 1.0f, "%.2f");
  }
  ImGui::PopItemWidth();
}

void light_placement_ui(light &_light) {
  const float step = ImGui::GetStyle().IndentSpacing;
  ImGui::PushItemWidth(imgui_form_item_width());

  switch (_light.Type) {
  case light_type::k_directional:
    ImGui::SliderFloat3("Direction", &_light.Direction.x, -1.0f, 1.0f, "%.2f");
    break;

  case light_type::k_point:
    ImGui::Indent(step);
    ImGui::SeparatorText("Transform");
    ImGui::SliderFloat3("Position", &_light.Position.x, -10.0f, 10.0f, "%.2f");
    ImGui::SeparatorText("Distance falloff");
    ImGui::SliderFloat("Constant", &_light.Constant, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Linear", &_light.Linear, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Quadratic", &_light.Quadratic, 0.0f, 1.0f, "%.2f");
    ImGui::Unindent(step);
    break;

  case light_type::k_spot:
    ImGui::Indent(step);
    ImGui::SeparatorText("Transform");
    ImGui::SliderFloat3("Position", &_light.Position.x, -10.0f, 10.0f, "%.2f");
    ImGui::SliderFloat3("Direction", &_light.Direction.x, -1.0f, 1.0f, "%.2f");
    ImGui::SeparatorText("Distance falloff");
    ImGui::SliderFloat("Constant", &_light.Constant, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Linear", &_light.Linear, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Quadratic", &_light.Quadratic, 0.0f, 1.0f, "%.2f");
    ImGui::SeparatorText("Cone");
    ImGui::SliderFloat("Inner cutoff (deg)", &_light.Cutoff, 0.0f, 90.0f, "%.1f");
    ImGui::SliderFloat("Outer cutoff (deg)", &_light.OuterCutoff, 0.0f, 90.0f,
                       "%.1f");
    ImGui::Unindent(step);
    break;
  }

  ImGui::PopItemWidth();
}

} // namespace

// -----------------------------------------------------------------------------
void uniform(shader &_shader, const light &_light, std::string _name) {
  _shader.set_uniform((_name + ".type").c_str(), static_cast<int>(_light.Type));
  _shader.set_uniform((_name + ".ambient").c_str(), _light.Ambient);
  _shader.set_uniform((_name + ".diffuse").c_str(), _light.Diffuse);
  _shader.set_uniform((_name + ".specular").c_str(), _light.Specular);

  _shader.set_uniform((_name + ".position").c_str(), _light.Position);
  _shader.set_uniform((_name + ".direction").c_str(), _light.Direction);
  _shader.set_uniform((_name + ".constant").c_str(), _light.Constant);
  _shader.set_uniform((_name + ".linear").c_str(), _light.Linear);
  _shader.set_uniform((_name + ".quadratic").c_str(), _light.Quadratic);

  float cutoff_cos = glm::cos(glm::radians(_light.Cutoff));
  float outer_cutoff_cos = glm::cos(glm::radians(_light.OuterCutoff));
  _shader.set_uniform((_name + ".cutoff").c_str(), cutoff_cos);
  _shader.set_uniform((_name + ".outer_cutoff").c_str(), outer_cutoff_cos);
}

// -----------------------------------------------------------------------------
void ui(light &_light) {
  constexpr ImGuiTreeNodeFlags root_flags =
      ImGuiTreeNodeFlags_DefaultOpen;
  constexpr ImGuiTreeNodeFlags section_flags =
      ImGuiTreeNodeFlags_DefaultOpen;

  if (!ImGui::CollapsingHeader("Light source", root_flags)) {
    return;
  }

  ImGui::PushItemWidth(imgui_form_item_width(0.78f));
  ImGui::Combo("Type", &(int &)_light.Type, "Directional\0Point\0Spot\0");
  ImGui::PopItemWidth();
  ImGui::Checkbox("Simple lighting", &_light.SimpleLighting);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("On: color and intensity only (plus placement).\n"
                      "Off: edit ambient, diffuse, and specular directly.");
  }

  ImGui::Spacing();

  if (ImGui::CollapsingHeader("Radiance", section_flags)) {
    light_radiance_ui(_light);
  }
  ImGui::Spacing();
  if (ImGui::CollapsingHeader("Placement", section_flags)) {
    light_placement_ui(_light);
  }
}
