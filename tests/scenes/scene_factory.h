#pragma once

#include "tests/framework/test_suit.h"
#include <functional>
#include <map>

// Helper macro to simplify scene registration
#define REGISTER_SCENE(scene_enum, scene_class)                                \
  m_scenes[scene_enum] = new scene_class();                                    \
  m_scenes[scene_enum]->init(_window);

// Factory function to create and initialize a scene
template <typename SceneType>
test_scene_base *create_scene(GLFWwindow *_window) {
  test_scene_base *scene = new SceneType();
  scene->init(_window);
  return scene;
}

