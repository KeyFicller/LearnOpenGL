#include "basic/material.h"
#include "basic/imgui_form.h"

#include "glm/glm.hpp"
#include "imgui.h"

#include <algorithm>

namespace {

void apply_simple_material_terms(material &_material) {
  _material.Diffuse = _material.SimpleAlbedo;
  _material.Ambient = _material.SimpleAlbedo * 0.25f;
  _material.Specular =
      glm::vec3(std::clamp(_material.SimpleSpecular, 0.0f, 1.0f));
}

void material_simple_ui(material &_material) {
  ImGui::PushItemWidth(imgui_form_item_width());
  ImGui::ColorEdit3("Surface color", &_material.SimpleAlbedo.x);
  ImGui::SliderFloat("Specular", &_material.SimpleSpecular, 0.0f, 1.0f, "%.2f");
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("Specular RGB weight (single channel, dielectric-style).");
  }
  ImGui::SliderFloat("Shininess", &_material.Shininess, 1.0f, 256.0f, "%.0f");
  ImGui::PopItemWidth();
  apply_simple_material_terms(_material);
}

void material_advanced_ui(material &_material) {
  const float step = ImGui::GetStyle().IndentSpacing;
  constexpr ImGuiTreeNodeFlags section_flags = ImGuiTreeNodeFlags_DefaultOpen;

  ImGui::PushItemWidth(imgui_form_item_width());

  if (ImGui::CollapsingHeader("Diffuse & ambient", section_flags)) {
    ImGui::Indent(step);
    ImGui::SliderFloat3("Ambient", &_material.Ambient.x, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat3("Diffuse", &_material.Diffuse.x, 0.0f, 1.0f, "%.2f");
    ImGui::Unindent(step);
  }

  ImGui::Spacing();
  if (ImGui::CollapsingHeader("Specular & highlight", section_flags)) {
    ImGui::Indent(step);
    ImGui::SliderFloat3("Specular", &_material.Specular.x, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Shininess", &_material.Shininess, 1.0f, 256.0f, "%.0f");
    ImGui::Unindent(step);
  }

  ImGui::PopItemWidth();
}

} // namespace

// -----------------------------------------------------------------------------
void uniform(shader &_shader, const material &_material, std::string _name) {
  _shader.set_uniform((_name + ".ambient").c_str(), _material.Ambient);
  _shader.set_uniform((_name + ".diffuse").c_str(), _material.Diffuse);
  _shader.set_uniform((_name + ".specular").c_str(), _material.Specular);
  _shader.set_uniform((_name + ".shininess").c_str(), _material.Shininess);
}

// -----------------------------------------------------------------------------
void ui(material &_material) {
  constexpr ImGuiTreeNodeFlags root_flags = ImGuiTreeNodeFlags_DefaultOpen;

  if (!ImGui::CollapsingHeader("Material", root_flags)) {
    return;
  }

  ImGui::Checkbox("Simple material", &_material.SimpleMaterial);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip("On: surface color, specular strength, and shininess.\n"
                      "Off: edit ambient, diffuse, and specular RGB directly.");
  }

  ImGui::Spacing();

  if (_material.SimpleMaterial) {
    if (ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen)) {
      material_simple_ui(_material);
    }
  } else {
    material_advanced_ui(_material);
  }
}

// -----------------------------------------------------------------------------
void uniform(shader &_shader, const texture_material &_texture_material,
             std::string _name) {
  if (_texture_material.DiffuseTexture) {
    _texture_material.DiffuseTexture->bind(0);
    int texture_unit = 0;
    _shader.set_uniform((_name + ".diffuse").c_str(), texture_unit);
  }
  if (_texture_material.SpecularTexture) {
    _texture_material.SpecularTexture->bind(1);
    int texture_unit = 1;
    _shader.set_uniform((_name + ".specular").c_str(), texture_unit);
  }
  _shader.set_uniform((_name + ".shininess").c_str(),
                      _texture_material.Shininess);
}

// -----------------------------------------------------------------------------
void ui(texture_material &_texture_material) {
  if (!ImGui::CollapsingHeader("Texture material", ImGuiTreeNodeFlags_DefaultOpen)) {
    return;
  }

  if (_texture_material.DiffuseTexture) {
    ImGui::Text("Diffuse Texture");
    ImGui::Image(_texture_material.DiffuseTexture->ID(), ImVec2(100, 100),
                 ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
  }
  if (_texture_material.SpecularTexture) {
    ImGui::Text("Specular Texture");
    ImGui::Image(_texture_material.SpecularTexture->ID(), ImVec2(100, 100),
                 ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
  }
  ImGui::PushItemWidth(imgui_form_item_width());
  ImGui::SliderFloat("Shininess", &_texture_material.Shininess, 1.0f, 128.0f,
                     "%.0f");
  ImGui::PopItemWidth();
}
