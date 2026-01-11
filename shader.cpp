#include "shader.h"

#include <fstream>
#include <iostream>

int create_shader_from_file(const char *_path, GLenum _shader_type) {
  GLuint shader = glCreateShader(_shader_type);
  std::ifstream shader_file(_path);
  if (!shader_file.is_open()) {
    std::cout << "ERROR: Failed to open shader file" << std::endl;
    throw std::runtime_error("Failed to open shader file");
  }
  std::string shader_source =
      std::string(std::istreambuf_iterator<char>(shader_file),
                  std::istreambuf_iterator<char>());
  shader_file.close();
  const char *shader_source_ptr = shader_source.c_str();
  glShaderSource(shader, 1, &shader_source_ptr, NULL);
  glCompileShader(shader);
  int success;
  char info_log[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, info_log);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << info_log << std::endl;
  }
  return shader;
}

int create_shader_program_from_shaders(GLuint _vertex_shader,
                                       GLuint _fragment_shader) {
  GLuint shader_program = glCreateProgram();
  glAttachShader(shader_program, _vertex_shader);
  glAttachShader(shader_program, _fragment_shader);
  glLinkProgram(shader_program);
  int success;
  char info_log[512];
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 512, NULL, info_log);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
              << info_log << std::endl;
  }
  glDeleteShader(_vertex_shader);
  glDeleteShader(_fragment_shader);
  return shader_program;
}

shader::shader(const char *_vertex_path, const char *_fragment_path) {
  // Vertex Shader
  GLuint vertex_shader =
      create_shader_from_file(_vertex_path, GL_VERTEX_SHADER);
  // Fragment Shader
  GLuint fragment_shader =
      create_shader_from_file(_fragment_path, GL_FRAGMENT_SHADER);
  // Shader Program
  m_ID = create_shader_program_from_shaders(vertex_shader, fragment_shader);
}

shader::~shader() { glDeleteProgram(m_ID); }

void shader::use() { glUseProgram(m_ID); }
