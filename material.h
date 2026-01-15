#pragma once

#include "glm/glm.hpp"
#include "imgui.h"
#include "shader.h"
#include "texture.h"

struct material {
public:
  glm::vec3 m_ambient = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 m_specular = glm::vec3(1.0f, 1.0f, 1.0f);
  float m_shininess = 32.0f;
};

struct texture_material {
public:
  texture_2d *m_diffuse_texture = nullptr;
  texture_2d *m_specular_texture = nullptr;
  float m_shininess = 32.0f;

  ~texture_material() {
    delete m_diffuse_texture;
    delete m_specular_texture;
  }
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

inline void ui(texture_material &_texture_material) {

  if (_texture_material.m_diffuse_texture) {
    ImGui::Text("Diffuse Texture");
    ImGui::Image(_texture_material.m_diffuse_texture->ID(), ImVec2(100, 100),
                 ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
  }
  if (_texture_material.m_specular_texture) {
    ImGui::Text("Specular Texture");
    ImGui::Image(_texture_material.m_specular_texture->ID(), ImVec2(100, 100),
                 ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
  }
  ImGui::SliderFloat("Shininess", &_texture_material.m_shininess, 1.0f, 128.0f,
                     "%.0f");
}

inline void uniform(shader &_shader, const texture_material &_texture_material,
                    std::string _name) {
  if (_texture_material.m_diffuse_texture) {
    _texture_material.m_diffuse_texture->bind(0);
    int texture_unit = 0;
    _shader.set_uniform<int, 1>((_name + ".diffuse").c_str(), &texture_unit);
  }
  if (_texture_material.m_specular_texture) {
    _texture_material.m_specular_texture->bind(1);
    int texture_unit = 1;
    _shader.set_uniform<int, 1>((_name + ".specular").c_str(), &texture_unit);
  }
  _shader.set_uniform<float, 1>((_name + ".shininess").c_str(),
                                &_texture_material.m_shininess);
}