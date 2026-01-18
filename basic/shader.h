#pragma once

#include "glm/fwd.hpp"
#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

/**
 * @brief Shader program wrapper
 */
class shader {
public:
  shader(const char *_vertex_path, const char *_fragment_path);
  virtual ~shader();

  shader(const shader &) = delete;
  shader &operator=(const shader &) = delete;
  shader(shader &&) = delete;
  shader &operator=(shader &&) = delete;

public:
  /**
   * @brief Activate the shader program
   */
  void use();

public:
  // Uniform function template specializations
  template <typename tType, unsigned COUNT> struct uniform_function {};

  template <> struct uniform_function<float, 1> {
    static void call(GLuint _location, const float *_values) {
      glUniform1f(_location, _values[0]);
    }
  };

  template <> struct uniform_function<float, 2> {
    static void call(GLuint _location, const float *_values) {
      glUniform2f(_location, _values[0], _values[1]);
    }
  };

  template <> struct uniform_function<float, 3> {
    static void call(GLuint _location, const float *_values) {
      glUniform3f(_location, _values[0], _values[1], _values[2]);
    }
  };

  template <> struct uniform_function<float, 4> {
    static void call(GLuint _location, const float *_values) {
      glUniform4f(_location, _values[0], _values[1], _values[2], _values[3]);
    }
  };

  template <> struct uniform_function<int, 1> {
    static void call(GLuint _location, const int *_values) {
      glUniform1i(_location, _values[0]);
    }
  };

  template <> struct uniform_function<int, 2> {
    static void call(GLuint _location, const int *_values) {
      glUniform2i(_location, _values[0], _values[1]);
    }
  };

  template <> struct uniform_function<int, 3> {
    static void call(GLuint _location, const int *_values) {
      glUniform3i(_location, _values[0], _values[1], _values[2]);
    }
  };

  template <> struct uniform_function<int, 4> {
    static void call(GLuint _location, const int *_values) {
      glUniform4i(_location, _values[0], _values[1], _values[2], _values[3]);
    }
  };

  template <> struct uniform_function<bool, 1> {
    static void call(GLuint _location, const bool *_values) {
      glUniform1i(_location, _values[0]);
    }
  };

  template <> struct uniform_function<bool, 2> {
    static void call(GLuint _location, const bool *_values) {
      glUniform2i(_location, _values[0], _values[1]);
    }
  };

  template <> struct uniform_function<bool, 3> {
    static void call(GLuint _location, const bool *_values) {
      glUniform3i(_location, _values[0], _values[1], _values[2]);
    }
  };

  template <> struct uniform_function<bool, 4> {
    static void call(GLuint _location, const bool *_values) {
      glUniform4i(_location, _values[0], _values[1], _values[2], _values[3]);
    }
  };

  template <> struct uniform_function<glm::mat4, 1> {
    static void call(GLuint _location, const glm::mat4 *_values) {
      glUniformMatrix4fv(_location, 1, GL_FALSE, &(*_values)[0][0]);
    }
  };

  template <> struct uniform_function<glm::vec3, 1> {
    static void call(GLuint _location, const glm::vec3 *_values) {
      glUniform3f(_location, (*_values)[0], (*_values)[1], (*_values)[2]);
    }
  };

  /**
   * @brief Set uniform value(s) from pointer
   * @param _name Uniform name
   * @param _values Pointer to value(s)
   */
  template <typename tType, unsigned COUNT>
  void set_uniform(const char *_name, const tType *_values) {
    uniform_function<tType, COUNT>::call(get_uniform_location(_name), _values);
  }

  /**
   * @brief Set uniform value (accepts temporary values)
   * @param _name Uniform name
   * @param _value Value to set
   */
  template <typename tType>
  void set_uniform(const char *_name, const tType &_value) {
    uniform_function<tType, 1>::call(get_uniform_location(_name), &_value);
  }

  /**
   * @brief Set uniform values from vector
   * @param _name Uniform name
   * @param _values Vector of values
   */
  template <typename tType, unsigned COUNT>
  void set_uniform(const char *_name, const std::vector<tType> &_values) {
    if (_values.size() != COUNT) {
      throw std::runtime_error("Invalid number of values for uniform " +
                               std::string(_name));
    }
    uniform_function<tType, COUNT>::call(get_uniform_location(_name),
                                         _values.data());
  }

  /**
   * @brief Set uniform values from variadic arguments
   * @param _name Uniform name
   * @param _values Variadic arguments of the same type
   */
  template <typename tType, typename... tArgs>
  typename std::enable_if<
      std::conjunction_v<std::is_same<tType, std::decay_t<tArgs>>...>,
      void>::type
  set_uniform(const char *_name, tArgs... _values) {
    constexpr unsigned COUNT = sizeof...(_values);
    tType values_array[COUNT] = {static_cast<tType>(_values)...};
    uniform_function<tType, COUNT>::call(get_uniform_location(_name),
                                         values_array);
  }

  /**
   * @brief Get uniform location by name
   * @param _name Uniform name
   * @return Uniform location, or -1 if not found
   */
  int get_uniform_location(const char *_name) const {
    return glGetUniformLocation(m_ID, _name);
  }

protected:
  GLuint m_ID = -1; // OpenGL shader program ID
};
