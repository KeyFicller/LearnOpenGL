#include "basic/shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

// Helper function to check OpenGL errors
static void check_gl_error(const char *operation) {
  GLenum error = glGetError();
  if (error != GL_NO_ERROR) {
    std::stringstream ss;
    ss << "OpenGL error " << error << " after " << operation;
    throw std::runtime_error(ss.str());
  }
}

int create_shader_from_source(const std::string &_source, GLenum _shader_type) {
  GLuint shader = glCreateShader(_shader_type);
  check_gl_error("glCreateShader");

  const char *shader_source_ptr = _source.c_str();
  glShaderSource(shader, 1, &shader_source_ptr, NULL);
  check_gl_error("glShaderSource");

  glCompileShader(shader);
  check_gl_error("glCompileShader");

  int success;
  char info_log[1024];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(shader, 1024, NULL, info_log);
    std::stringstream ss;
    ss << "Shader compilation failed for " << ":\n" << info_log;
    glDeleteShader(shader);
    throw std::runtime_error(ss.str());
  }
  return shader;
}

int create_shader_from_file(const char *_path, GLenum _shader_type) {

  std::ifstream shader_file(_path);
  if (!shader_file.is_open()) {
    std::stringstream ss;
    ss << "Failed to open shader file: " << _path;
    throw std::runtime_error(ss.str());
  }

  std::string shader_source =
      std::string(std::istreambuf_iterator<char>(shader_file),
                  std::istreambuf_iterator<char>());
  shader_file.close();

  return create_shader_from_source(shader_source, _shader_type);
}

int create_shader_program_from_shaders(GLuint _vertex_shader,
                                       GLuint _fragment_shader,
                                       GLuint _geometry_shader) {
  GLuint shader_program = glCreateProgram();
  check_gl_error("glCreateProgram");

  glAttachShader(shader_program, _vertex_shader);
  check_gl_error("glAttachShader (vertex)");

  glAttachShader(shader_program, _fragment_shader);
  check_gl_error("glAttachShader (fragment)");

  if (_geometry_shader != 0) {
    glAttachShader(shader_program, _geometry_shader);
    check_gl_error("glAttachShader (geometry)");
  }

  glLinkProgram(shader_program);
  check_gl_error("glLinkProgram");

  int success;
  char info_log[1024];
  glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(shader_program, 1024, NULL, info_log);
    std::stringstream ss;
    ss << "Shader program linking failed:\n" << info_log;
    glDeleteProgram(shader_program);
    glDeleteShader(_vertex_shader);
    glDeleteShader(_fragment_shader);
    if (_geometry_shader != 0) {
      glDeleteShader(_geometry_shader);
    }
    throw std::runtime_error(ss.str());
  }

  glDeleteShader(_vertex_shader);
  glDeleteShader(_fragment_shader);
  if (_geometry_shader != 0) {
    glDeleteShader(_geometry_shader);
  }
  return shader_program;
}

shader::shader(const char *_vertex_path, const char *_fragment_path,
               const char *_geometry_path) {
  try {

    GLuint vertex_shader =
        create_shader_from_file(_vertex_path, GL_VERTEX_SHADER);
    GLuint fragment_shader =
        create_shader_from_file(_fragment_path, GL_FRAGMENT_SHADER);
    GLuint geometry_shader =
        _geometry_path
            ? create_shader_from_file(_geometry_path, GL_GEOMETRY_SHADER)
            : 0;
    m_ID = create_shader_program_from_shaders(vertex_shader, fragment_shader,
                                              geometry_shader);
  } catch (std::exception &e) {
    std::stringstream ss;
    ss << "Failed to create shader program from file: " << _vertex_path << ", "
       << _fragment_path << ", " << _geometry_path << ": " << e.what();
    throw std::runtime_error(ss.str());
  }
}

shader::~shader() { glDeleteProgram(m_ID); }

void shader::use() { glUseProgram(m_ID); }

shader *shader::shader_from_source(const std::string &_vertex_source,
                                   const std::string &_fragment_source,
                                   const std::string &_geometry_source) {

  GLuint vertex_shader =
      create_shader_from_source(_vertex_source, GL_VERTEX_SHADER);
  GLuint fragment_shader =
      create_shader_from_source(_fragment_source, GL_FRAGMENT_SHADER);
  GLuint geometry_shader =
      !_geometry_source.empty()
          ? create_shader_from_source(_geometry_source, GL_GEOMETRY_SHADER)
          : 0;
  int shader_program = create_shader_program_from_shaders(
      vertex_shader, fragment_shader, geometry_shader);
  return new shader(shader_program);
}