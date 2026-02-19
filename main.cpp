#include <exception>
#include <fstream>
#include <iostream>
#include <string>

// GLAD
#include <glad/gl.h>

// GLFW (include after glad)
#include <GLFW/glfw3.h>

// ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "basic/framebuffer.h"
#include "callbacks.h"
#ifdef LEARNOPENGL_USE_MONO
#include "scripts/mono_invoker.h"
#endif
#include "tests/framework/test_suit.h"

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
        ImGuiConfigFlags_NavEnableKeyboard | // Enable Keyboard Controls
        ImGuiConfigFlags_DockingEnable;      // Enable Docking

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

#ifdef LEARNOPENGL_USE_MONO
    // Initialize C# Mono scripting (optional - continues if scripts not found)
    mono_invoker::invoker invoker;
    const char *dll_candidates[] = {"runtime/Scripts.dll",
                                  "scripts/csharp/bin/Scripts.dll"};
    std::string dll_path;
    for (const char *p : dll_candidates) {
      if (std::ifstream(p).good()) {
        dll_path = p;
        break;
      }
    }
    if (!dll_path.empty()) {
      invoker.load(dll_path);
    }
    if (!invoker.is_ready()) {
      std::cout << "[Mono] Failed to load assembly (tried runtime/Scripts.dll "
                   "and scripts/csharp/bin/Scripts.dll)" << std::endl;
    }
    std::string msg;
    if (!invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "SayHello"_md),
                          msg)) {
      std::cout
          << "[Mono] Failed to invoke method: Scripts.ExampleScript.SayHello"
          << std::endl;
    }
    std::cout << "[Mono] " << msg << std::endl;
    int int_result;
    if (!invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "SayInt"_md),
                          int_result)) {
      std::cout
          << "[Mono] Failed to invoke method: Scripts.ExampleScript.SayInt"
          << std::endl;
    }
    std::cout << "[Mono] " << int_result << std::endl;
    float float_result;
    if (!invoker.invoke_r(mono_invoker::script_ncm(
                              "Scripts"_ns, "ExampleScript"_cls, "SayFloat"_md),
                          float_result)) {
      std::cout
          << "[Mono] Failed to invoke method: Scripts.ExampleScript.SayFloat"
          << std::endl;
    }
    std::cout << "[Mono] " << float_result << std::endl;
#endif

    // Set user pointer to access test_suit in callbacks
    glfwSetWindowUserPointer(window, &test_suit);

    // Create framebuffer for scene rendering
    int framebuffer_width = WIDTH;
    int framebuffer_height = HEIGHT;
    framebuffer scene_framebuffer(framebuffer_width, framebuffer_height);

    // ImGui state
    bool show_demo_window = false;
    ImGui::StyleColorsClassic();

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

      // Create dockspace
      ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->Pos);
      ImGui::SetNextWindowSize(viewport->Size);
      ImGui::SetNextWindowViewport(viewport->ID);
      ImGuiWindowFlags window_flags =
          ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
          ImGuiWindowFlags_NoNavFocus;

      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
      ImGui::Begin("DockSpace", nullptr, window_flags);
      ImGui::PopStyleVar(3);

      ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f),
                       ImGuiDockNodeFlags_PassthruCentralNode);
      ImGui::End();

      // Show the big demo window
      if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

      // Render test suit UI in dock window
      test_suit.render_ui();

      // Process input
      processInput(window);

      // Get viewport size first to adjust framebuffer
      ImVec2 viewport_size =
          ImVec2(scene_framebuffer.get_width(), scene_framebuffer.get_height());
      ImGui::Begin("Scene Viewport");
      ImVec2 available_size = ImGui::GetContentRegionAvail();
      if (available_size.x > 0 && available_size.y > 0) {
        viewport_size = available_size;
      }

      // Get window position and content region to account for ToolBar and
      // padding
      ImVec2 window_pos = ImGui::GetWindowPos();
      ImVec2 content_min = ImGui::GetWindowContentRegionMin();
      ImVec2 content_max = ImGui::GetWindowContentRegionMax();

      // Calculate actual content region position (window pos + content region
      // offset)
      test_suit.m_viewport_x = window_pos.x + content_min.x;
      test_suit.m_viewport_y = window_pos.y + content_min.y;
      test_suit.m_viewport_width = content_max.x - content_min.x;
      test_suit.m_viewport_height = content_max.y - content_min.y;
      ImGui::End();

      // Resize framebuffer if needed (with some minimum size)
      if (viewport_size.x > 0 && viewport_size.y > 0) {
        int new_width = static_cast<int>(viewport_size.x);
        int new_height = static_cast<int>(viewport_size.y);
        if (new_width != scene_framebuffer.get_width() ||
            new_height != scene_framebuffer.get_height()) {
          scene_framebuffer.resize(new_width, new_height);
        }
      }

      // Render scene to framebuffer
      scene_framebuffer.bind();
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

      // Unbind framebuffer (back to default)
      scene_framebuffer.unbind();

      // Display scene in ImGui window
      ImGui::Begin("Scene Viewport");
      // Display framebuffer texture in ImGui window
      ImVec2 display_size = ImGui::GetContentRegionAvail();
      ImGui::Image((void *)(intptr_t)scene_framebuffer.get_color_texture(),
                   display_size, ImVec2(0, 1), ImVec2(1, 0));
      ImGui::End();

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