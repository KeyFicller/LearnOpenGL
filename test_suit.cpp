#include "test_suit.h"

#include "tests/scenes/camera_test_scene.h"
#include "tests/scenes/color_test_scene.h"
#include "tests/scenes/coordinate_test_scene.h"
#include "tests/scenes/light_color_scene.h"
#include "tests/scenes/texture_test_scene.h"
#include "tests/scenes/transform_test_scene.h"
#include "tests/scenes/triangle_test_scene.h"

#include "imgui.h"

test_suit::test_suit() : m_current_scene(test_scene::k_texture_test) {}

test_suit::~test_suit() {
  for (auto &[scene, test_scene] : m_scenes) {
    delete test_scene;
  }
}

void test_suit::init(GLFWwindow *_window) {
  // Create and initialize all test scenes
  m_scenes[test_scene::k_texture_test] = new texture_test_scene();
  m_scenes[test_scene::k_texture_test]->init(_window);

  m_scenes[test_scene::k_triangle_test] = new triangle_test_scene();
  m_scenes[test_scene::k_triangle_test]->init(_window);

  m_scenes[test_scene::k_color_test] = new color_test_scene();
  m_scenes[test_scene::k_color_test]->init(_window);

  m_scenes[test_scene::k_transform_test] = new transform_test_scene();
  m_scenes[test_scene::k_transform_test]->init(_window);

  m_scenes[test_scene::k_coordinate_test] = new coordinate_test_scene();
  m_scenes[test_scene::k_coordinate_test]->init(_window);

  m_scenes[test_scene::k_camera_test] = new camera_test_scene();
  m_scenes[test_scene::k_camera_test]->init(_window);

  m_scenes[test_scene::k_light_color_test] = new light_color_scene();
  m_scenes[test_scene::k_light_color_test]->init(_window);
}

void test_suit::render_ui() {
  ImGui::Begin("Test Scene Control");

  ImGui::Text("Select Test Scene:");
  ImGui::Separator();

  for (int i = 0; i < static_cast<int>(test_scene::k_count); i++) {
    test_scene scene = static_cast<test_scene>(i);
    bool is_selected = (m_current_scene == scene);
    if (ImGui::RadioButton(get_scene_name(scene), is_selected)) {
      m_current_scene = scene;
    }
  }

  ImGui::Separator();
  test_scene_base *current = get_scene(m_current_scene);
  if (current) {
    ImGui::Text("Current Scene: %s", current->get_name());

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
    scene->render();
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