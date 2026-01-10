#include "shader.h"

#include <fstream>
#include <iostream>

int create_shader_from_file(const char *path, GLenum shaderType) {
  GLuint shader = glCreateShader(shaderType);
  std::ifstream shaderFile(path);
  if (!shaderFile.is_open()) {
    std::cout << "ERROR: Failed to open shader file" << std::endl;
    throw std::runtime_error("Failed to open shader file");
  }
  std::string shaderSource =
      std::string(std::istreambuf_iterator<char>(shaderFile),
                  std::istreambuf_iterator<char>());
  shaderFile.close();
  const char *shaderSourcePtr = shaderSource.c_str();
  glShaderSource(shader, 1, &shaderSourcePtr, NULL);
  glCompileShader(shader);
  int success;
  char infoLog[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
              << infoLog << std::endl;
  }
  return shader;
}

int create_shader_program_from_shaders(GLuint vertexShader,
                                       GLuint fragmentShader) {
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  int success;
  char infoLog[512];
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
              << infoLog << std::endl;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return shaderProgram;
}

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
  // Vertex Shader
  GLuint vertexShader = create_shader_from_file(vertexPath, GL_VERTEX_SHADER);
  // Fragment Shader
  GLuint fragmentShader =
      create_shader_from_file(fragmentPath, GL_FRAGMENT_SHADER);
  // Shader Program
  m_ID = create_shader_program_from_shaders(vertexShader, fragmentShader);
}

Shader::~Shader() { glDeleteProgram(m_ID); }

void Shader::use() { glUseProgram(m_ID); }