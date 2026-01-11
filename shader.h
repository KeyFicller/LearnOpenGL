#pragma once

#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

class Shader {
public:
  Shader(const char *vertexPath, const char *fragmentPath);
  virtual ~Shader();

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;
  Shader(Shader &&) = delete;
  Shader &operator=(Shader &&) = delete;

public:
  void use();

public:
  template <typename T, unsigned COUNT> struct UniformFunction {};

  template <> struct UniformFunction<float, 1> {
    static void call(GLuint location, const float *values) {
      glUniform1f(location, values[0]);
    }
  };

  template <> struct UniformFunction<float, 2> {
    static void call(GLuint location, const float *values) {
      glUniform2f(location, values[0], values[1]);
    }
  };

  template <> struct UniformFunction<float, 3> {
    static void call(GLuint location, const float *values) {
      glUniform3f(location, values[0], values[1], values[2]);
    }
  };

  template <> struct UniformFunction<float, 4> {
    static void call(GLuint location, const float *values) {
      glUniform4f(location, values[0], values[1], values[2], values[3]);
    }
  };

  template <> struct UniformFunction<int, 1> {
    static void call(GLuint location, const int *values) {
      glUniform1i(location, values[0]);
    }
  };

  template <> struct UniformFunction<int, 2> {
    static void call(GLuint location, const int *values) {
      glUniform2i(location, values[0], values[1]);
    }
  };

  template <> struct UniformFunction<int, 3> {
    static void call(GLuint location, const int *values) {
      glUniform3i(location, values[0], values[1], values[2]);
    }
  };

  template <> struct UniformFunction<int, 4> {
    static void call(GLuint location, const int *values) {
      glUniform4i(location, values[0], values[1], values[2], values[3]);
    }
  };

  template <> struct UniformFunction<bool, 1> {
    static void call(GLuint location, const bool *values) {
      glUniform1i(location, values[0]);
    }
  };

  template <> struct UniformFunction<bool, 2> {
    static void call(GLuint location, const bool *values) {
      glUniform2i(location, values[0], values[1]);
    }
  };

  template <> struct UniformFunction<bool, 3> {
    static void call(GLuint location, const bool *values) {
      glUniform3i(location, values[0], values[1], values[2]);
    }
  };

  template <> struct UniformFunction<bool, 4> {
    static void call(GLuint location, const bool *values) {
      glUniform4i(location, values[0], values[1], values[2], values[3]);
    }
  };

  template <> struct UniformFunction<glm::mat4, 1> {
    static void call(GLuint location, const glm::mat4 *values) {
      glUniformMatrix4fv(location, 1, GL_FALSE, &(*values)[0][0]);
    }
  };

  template <typename T, unsigned COUNT>
  void set_uniform(const char *name, const T *values) {
    UniformFunction<T, COUNT>::call(get_uniform_location(name), values);
  }

  template <typename T, unsigned COUNT>
  void set_uniform(const char *name, const std::vector<T> &values) {
    if (values.size() != COUNT) {
      throw std::runtime_error("Invalid number of values for uniform " +
                               std::string(name));
    }
    UniformFunction<T, COUNT>::call(get_uniform_location(name), values.data());
  }

  template <typename T, typename... Args>
  typename std::enable_if<
      std::conjunction_v<std::is_same<T, std::decay_t<Args>>...>, void>::type
  set_uniform(const char *name, Args... values) {
    constexpr unsigned COUNT = sizeof...(values);
    T valuesArray[COUNT] = {static_cast<T>(values)...};
    UniformFunction<T, COUNT>::call(get_uniform_location(name), valuesArray);
  }

  int get_uniform_location(const char *name) const {
    return glGetUniformLocation(m_ID, name);
  }

protected:
  GLuint m_ID = -1;
};