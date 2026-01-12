#include "scene_base.h"

test_scene_base::test_scene_base(const std::string &_name) : m_name(_name) {}

test_scene_base::~test_scene_base() {}

void test_scene_base::init(GLFWwindow *_window) { m_window = _window; }
