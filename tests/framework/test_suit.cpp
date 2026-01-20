#include "tests/framework/test_suit.h"

#include "imgui.h"
#include "tests/scenes/advanced_glsl_scene.h"
#include "tests/scenes/blender_test_scene.h"
#include "tests/scenes/camera_test_scene.h"
#include "tests/scenes/color_test_scene.h"
#include "tests/scenes/coordinate_test_scene.h"
#include "tests/scenes/cull_test_scene.h"
#include "tests/scenes/depth_test_scene.h"
#include "tests/scenes/import_model_scene.h"
#include "tests/scenes/light_color_scene.h"
#include "tests/scenes/light_material_scene.h"
#include "tests/scenes/light_texture_scene.h"
#include "tests/scenes/light_type_scene.h"
#include "tests/scenes/multiple_light_scene.h"
#include "tests/scenes/scene_factory.h"
#include "tests/scenes/stencil_test_scene.h"
#include "tests/scenes/texture_cube_scene.h"
#include "tests/scenes/texture_test_scene.h"
#include "tests/scenes/transform_test_scene.h"
#include "tests/scenes/triangle_test_scene.h"
#include <exception>
#include <iostream>

test_suit::test_suit() : m_current_scene(test_scene::k_texture_test) {}

test_suit::~test_suit() {
  for (auto &[scene, test_scene] : m_scenes) {
    delete test_scene;
  }
}

void test_suit::init(GLFWwindow *_window) {
  try {
    // Create and initialize all test scenes
    REGISTER_SCENE(test_scene::k_texture_test, texture_test_scene);
    REGISTER_SCENE(test_scene::k_triangle_test, triangle_test_scene);
    REGISTER_SCENE(test_scene::k_color_test, color_test_scene);
    REGISTER_SCENE(test_scene::k_transform_test, transform_test_scene);
    REGISTER_SCENE(test_scene::k_coordinate_test, coordinate_test_scene);
    REGISTER_SCENE(test_scene::k_camera_test, camera_test_scene);
    REGISTER_SCENE(test_scene::k_light_color_test, light_color_scene);
    REGISTER_SCENE(test_scene::k_light_material_test, light_material_scene);
    REGISTER_SCENE(test_scene::k_light_texture_test, light_texture_scene);
    REGISTER_SCENE(test_scene::k_light_type_test, light_type_scene);
    REGISTER_SCENE(test_scene::k_multiple_light_test, multiple_light_scene);
    REGISTER_SCENE(test_scene::k_import_model_test, import_model_scene);
    REGISTER_SCENE(test_scene::k_depth_test_test, depth_test_scene);
    REGISTER_SCENE(test_scene::k_stencil_test_test, stencil_test_scene);
    REGISTER_SCENE(test_scene::k_blender_test_test, blender_test_scene);
    REGISTER_SCENE(test_scene::k_cull_test_test, cull_test_scene);
    REGISTER_SCENE(test_scene::k_texture_cube_test_test, texture_cube_scene);
    REGISTER_SCENE(test_scene::k_advanced_glsl_test_test, advanced_glsl_scene);
  } catch (const std::exception &e) {
    std::cerr << "Error initializing test scenes: " << e.what() << std::endl;
    throw; // Re-throw to be caught by main
  } catch (...) {
    std::cerr << "Unknown error initializing test scenes" << std::endl;
    throw;
  }
}

void test_suit::render_ui() {
  // Scene selection window
  ImGui::Begin("Scene Selection");
  ImGui::Text("Select Test Scene:");
  ImGui::Separator();

  for (int i = 0; i < static_cast<int>(test_scene::k_count); i++) {
    test_scene scene = static_cast<test_scene>(i);
    bool is_selected = (m_current_scene == scene);
    if (ImGui::RadioButton(get_scene_name(scene), is_selected)) {
      m_current_scene = scene;
    }
  }
  ImGui::End();

  // Scene properties window
  ImGui::Begin("Scene Properties");
  test_scene_base *current = get_scene(m_current_scene);
  if (current) {
    ImGui::Text("Current Scene: %s", current->get_name());
    ImGui::Separator();

    // Render scene-specific UI controls
    current->render_ui();
  }
  ImGui::Separator();
  ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
              1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
  ImGui::End();
}

void test_suit::render_scene() {
  test_scene_base *scene = get_scene(m_current_scene);
  if (scene) {
    try {
      scene->render();
    } catch (const std::exception &e) {
      std::cerr << "Error in scene render: " << e.what() << std::endl;
      std::cerr << "Scene: " << (scene ? scene->get_name() : "unknown")
                << std::endl;
      throw; // Re-throw to be caught by main
    }
  }
}

const char *test_suit::get_scene_name(test_scene _scene) {
  return get_scene(_scene)->get_name();
}

test_scene_base *test_suit::get_scene(test_scene _scene) {
  return m_scenes[_scene];
}

void test_suit::update(float _delta_time) {
  test_scene_base *scene = get_scene(m_current_scene);
  if (scene) {
    scene->update(_delta_time);
  }
}

bool test_suit::on_mouse_moved(double _xpos, double _ypos) {
  test_scene_base *scene = get_scene(m_current_scene);
  if (scene) {
    return scene->on_mouse_moved(_xpos, _ypos);
  }
  return false;
}

bool test_suit::on_mouse_scroll(double _xoffset, double _yoffset) {
  test_scene_base *scene = get_scene(m_current_scene);
  if (scene) {
    return scene->on_mouse_scroll(_xoffset, _yoffset);
  }
  return false;
}

void test_suit::on_framebuffer_resized(int _width, int _height) {
  test_scene_base *scene = get_scene(m_current_scene);
  if (scene) {
    scene->on_framebuffer_resized(_width, _height);
  }
}
