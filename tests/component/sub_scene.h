#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declaration
class test_scene_base;

/**
 * @brief Template base class for sub-scenes
 * @tparam ParentScene The parent scene type that owns this sub-scene
 *
 * Sub-scenes are used to organize different rendering modes or test cases
 * within a parent scene. They can access parent scene resources through
 * the parent() method.
 */
template <typename ParentScene> class sub_scene {
public:
  sub_scene(ParentScene *parent, const std::string &name)
      : m_parent(parent), m_name(name) {}
  virtual ~sub_scene() = default;

  // Sub-scenes cannot be copied or moved
  sub_scene(const sub_scene &) = delete;
  sub_scene &operator=(const sub_scene &) = delete;
  sub_scene(sub_scene &&) = delete;
  sub_scene &operator=(sub_scene &&) = delete;

  // Get sub-scene name
  const std::string &name() const { return m_name; }

  // Access parent scene
  ParentScene *parent() { return m_parent; }
  const ParentScene *parent() const { return m_parent; }

  // Lifecycle methods (optional overrides)
  virtual void init() {} // Called after parent scene initialization
  virtual void update(float delta_time) {} // Called each frame before render

  // Rendering methods (must be implemented)
  virtual void render() = 0;      // Render the sub-scene
  virtual void render_ui() = 0;   // Render sub-scene UI

protected:
  ParentScene *m_parent;
  std::string m_name;
};

/**
 * @brief Helper class to manage multiple sub-scenes
 * @tparam ParentScene The parent scene type
 */
template <typename ParentScene> class sub_scene_manager {
public:
  using sub_scene_ptr = std::unique_ptr<sub_scene<ParentScene>>;

  // Add a sub-scene (ownership is transferred)
  void add_sub_scene(sub_scene_ptr scene) {
    if (!scene) {
      return;
    }
    m_sub_scenes.push_back(std::move(scene));
    if (m_current_index < 0 && !m_sub_scenes.empty()) {
      m_current_index = 0;
      if (m_sub_scenes[0]) {
        m_sub_scenes[0]->init();
      }
    }
  }

  // Get number of sub-scenes
  size_t count() const { return m_sub_scenes.size(); }

  // Get current sub-scene index
  int current_index() const { return m_current_index; }

  // Set current sub-scene by index
  void set_current(int index) {
    if (index >= 0 && index < static_cast<int>(m_sub_scenes.size())) {
      m_current_index = index;
      if (m_sub_scenes[index]) {
        m_sub_scenes[index]->init();
      }
    }
  }

  // Get current sub-scene
  sub_scene<ParentScene> *current() {
    if (m_current_index >= 0 &&
        m_current_index < static_cast<int>(m_sub_scenes.size())) {
      return m_sub_scenes[m_current_index].get();
    }
    return nullptr;
  }

  // Get current sub-scene name
  std::string current_name() const {
    if (m_current_index >= 0 &&
        m_current_index < static_cast<int>(m_sub_scenes.size()) &&
        m_sub_scenes[m_current_index]) {
      return m_sub_scenes[m_current_index]->name();
    }
    return "";
  }

  // Get all sub-scene names for UI
  std::vector<const char *> get_names() const {
    std::vector<const char *> names;
    for (const auto &scene : m_sub_scenes) {
      if (scene) {
        names.push_back(scene->name().c_str());
      }
    }
    return names;
  }

  // Update current sub-scene
  void update(float delta_time) {
    if (auto *current_scene = current()) {
      current_scene->update(delta_time);
    }
  }

  // Render current sub-scene
  void render() {
    if (auto *current_scene = current()) {
      current_scene->render();
    }
  }

  // Render current sub-scene UI
  void render_ui() {
    if (auto *current_scene = current()) {
      current_scene->render_ui();
    }
  }

private:
  std::vector<sub_scene_ptr> m_sub_scenes;
  int m_current_index = -1;
};

