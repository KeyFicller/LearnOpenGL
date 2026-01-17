#include <exception>
#include <iostream>
#include <stdexcept>

// GLAD
#include <glad/gl.h>

// GLFW (include after glad)
#include <GLFW/glfw3.h>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "tests/framework/test_suit.h"

// Function prototypes
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mode);
void error_callback(int error, const char *description);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// The MAIN function, from here we start the application and run the game loop
int main() {
  try {
    // Init GLFW
    if (!glfwInit()) {
      std::cerr << "Failed to initialize GLFW" << std::endl;
      return -1;
    }
    // Set all the required options for GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    // Create a GLFWwindow object that we can use for GLFW's functions
    GLFWwindow *window =
        glfwCreateWindow(WIDTH, HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
      const char *description;
      int code = glfwGetError(&description);
      std::cout << "Failed to create GLFW window (error " << code
                << "): " << (description ? description : "unknown error")
                << std::endl;
      glfwTerminate();
      return -1;
    }
    glfwMakeContextCurrent(window);

    // Set the required callback functions
    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
      std::cout << "Failed to initialize OpenGL context" << std::endl;
      return -1;
    }

    // Successfully loaded OpenGL
    std::cout << "Loaded OpenGL " << GLAD_VERSION_MAJOR(version) << "."
              << GLAD_VERSION_MINOR(version) << std::endl;

    // Define the viewport dimensions
    glViewport(0, 0, WIDTH, HEIGHT);

    // ------------------ Setup Dear ImGui ------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |=
        ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    // Note: install_callbacks = false to prevent ImGui from intercepting our
    // callbacks We handle callbacks manually and forward to ImGui if needed
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // ----------------------------------------------------------

    // Initialize test suit
    test_suit test_suit;
    try {
      test_suit.init(window);
    } catch (const std::exception &e) {
      std::cerr << "Failed to initialize test suit: " << e.what() << std::endl;
      glfwTerminate();
      return -1;
    }

    // Set user pointer to access test_suit in callbacks
    glfwSetWindowUserPointer(window, &test_suit);

    // ImGui state
    bool show_demo_window = false;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);

    float last_time = glfwGetTime();

    // Game loop
    while (!glfwWindowShouldClose(window)) {
      // Poll and handle events
      glfwPollEvents();

      // Update test suit
      float current_time = glfwGetTime();
      float delta_time = current_time - last_time;
      last_time = current_time;
      test_suit.update(delta_time);

      // Start the Dear ImGui frame
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplGlfw_NewFrame();
      ImGui::NewFrame();

      // Show the big demo window
      if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

      // Render test suit UI
      test_suit.render_ui();

      // Process input
      processInput(window);

      // Render
      // Clear the colorbuffer
      glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
              GL_STENCIL_BUFFER_BIT);

      // Render current test scene
      try {
        test_suit.render_scene();

        // Check for OpenGL errors after rendering
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
          std::cerr << "OpenGL error after render: " << error << std::endl;
        }
      } catch (const std::exception &e) {
        std::cerr << "Error rendering scene: " << e.what() << std::endl;
        // Check OpenGL error state
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
          std::cerr << "OpenGL error code: " << error << std::endl;
        }
        // Continue to next frame instead of crashing
      }

      // Render ImGui
      ImGui::Render();
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

      // Swap the screen buffers
      glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Terminates GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "\n========================================\n";
    std::cerr << "EXCEPTION CAUGHT: " << e.what() << std::endl;
    std::cerr << "Exception type: " << typeid(e).name() << std::endl;
    std::cerr << "========================================\n";

    // Try to cleanup
    try {
      glfwTerminate();
    } catch (...) {
      // Ignore cleanup errors
    }

    return -1;
  } catch (...) {
    std::cerr << "\n========================================\n";
    std::cerr << "UNKNOWN EXCEPTION CAUGHT!" << std::endl;
    std::cerr << "========================================\n";

    // Try to cleanup
    try {
      glfwTerminate();
    } catch (...) {
      // Ignore cleanup errors
    }

    return -1;
  }
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mode) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}

// GLFW error callback
void error_callback(int error, const char *description) {
  std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// GLFW framebuffer size callback
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);

  // Notify test suit about framebuffer resize
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_framebuffer_resized(width, height);
  }
}

// GLFW mouse cursor callback
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
  // Forward mouse event to ImGui
  // ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);

  // Process our camera movement
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_mouse_moved(xpos, ypos);
  }
}

// GLFW mouse button callback
void mouse_button_callback(GLFWwindow *window, int button, int action,
                           int mods) {
  // Forward mouse button event to ImGui
  // ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

  // Process mouse button events here if needed
  // test_suit *test_suit_ptr =
  //     static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  // if (test_suit_ptr) {
  //   test_suit_ptr->on_mouse_button(button, action, mods);
  // }
}

// GLFW mouse scroll callback
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
  // Forward scroll event to ImGui (it needs this for UI scrolling)
  // ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

  // Process our camera scroll (we'll check WantCaptureMouse in the scene logic)
  test_suit *test_suit_ptr =
      static_cast<test_suit *>(glfwGetWindowUserPointer(window));
  if (test_suit_ptr) {
    test_suit_ptr->on_mouse_scroll(xoffset, yoffset);
  }
}

// Process all input: query GLFW whether relevant keys are pressed/released,
// if they are we call corresponding functions
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);
}