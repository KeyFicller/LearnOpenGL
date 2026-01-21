#include "basic/material.h"

#include "glm/glm.hpp"
#include "imgui.h"

// -----------------------------------------------------------------------------
void uniform(shader &_shader, const material &_material, std::string _name) {
  _shader.set_uniform((_name + ".ambient").c_str(), _material.Ambient);
  _shader.set_uniform((_name + ".diffuse").c_str(), _material.Diffuse);
  _shader.set_uniform((_name + ".specular").c_str(), _material.Specular);
  _shader.set_uniform((_name + ".shininess").c_str(), _material.Shininess);
}

// -----------------------------------------------------------------------------
void ui(material &_material) {
  ImGui::SliderFloat3("Ambient", &_material.Ambient.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Diffuse", &_material.Diffuse.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat3("Specular", &_material.Specular.x, 0.0f, 1.0f, "%.2f");
  ImGui::SliderFloat("Shininess", &_material.Shininess, 1.0f, 128.0f, "%.0f");
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
  _shader.set_uniform((_name + ".shininess").c_str(), _texture_material.Shininess);
}

// -----------------------------------------------------------------------------
void ui(texture_material &_texture_material) {
  if (_texture_material.DiffuseTexture) {
    ImGui::Text("Diffuse Texture");
    ImGui::Image(_texture_material.DiffuseTexture->ID(), ImVec2(100, 100),
                 ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
  }
  if (_texture_material.SpecularTexture) {
    ImGui::Text("Specular Texture");
    ImGui::Image(_texture_material.SpecularTexture->ID(), ImVec2(100, 100),
                 ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
  }
  ImGui::SliderFloat("Shininess", &_texture_material.Shininess, 1.0f, 128.0f,
                     "%.0f");
}

