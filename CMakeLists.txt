cmake_minimum_required(VERSION 3.15)

project(LearnOpenGL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Generate compile_commands.json for better IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable debug symbols for all build types (helps with stack traces)
if(MSVC)
  # MSVC compiler flags
  add_compile_options(/Zi)  # Generate debug information
  add_link_options(/DEBUG)  # Generate debug link information
else()
  # GCC/Clang compiler flags
  add_compile_options(-g)   # Generate debug information
endif()

include(FetchContent)

# ------------------------ Vendor:  GLFW ------------------------
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_MakeAvailable(glfw)
# ---------------------------------------------------------------

# ------------------------ Vendor:  GLAD(Generator) ------------------------

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v2.0.0
)

FetchContent_MakeAvailable(glad)

# Find Python for GLAD generation
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})

# Set GLAD directories
set(GLAD_SOURCES_DIR "${glad_SOURCE_DIR}")
set(GLAD_DIR "${CMAKE_BINARY_DIR}/gladsources/glad_gl")
set(GLAD_GENERATED_HEADER "${GLAD_DIR}/include/glad/gl.h")
set(GLAD_GENERATED_SOURCE "${GLAD_DIR}/src/gl.c")
set(GLAD_GENERATED_KHR "${GLAD_DIR}/include/KHR/khrplatform.h")

# Generate GLAD files at configure time
file(MAKE_DIRECTORY "${GLAD_DIR}")
file(COPY "${GLAD_SOURCES_DIR}/glad/files/gl.xml" DESTINATION "${GLAD_SOURCES_DIR}")
file(MAKE_DIRECTORY "${GLAD_SOURCES_DIR}/KHR")
file(COPY "${GLAD_SOURCES_DIR}/glad/files/khrplatform.h" DESTINATION "${GLAD_SOURCES_DIR}/KHR")

# Get Python site-packages path for dependencies
execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import site; print(site.getsitepackages()[0])"
    OUTPUT_VARIABLE Python3_SITE_PACKAGES
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${GLAD_SOURCES_DIR}:${Python3_SITE_PACKAGES}" ${Python3_EXECUTABLE} -m glad --out-path "${GLAD_DIR}" --api gl:core=3.3 c --loader
    WORKING_DIRECTORY "${GLAD_SOURCES_DIR}"
    RESULT_VARIABLE GLAD_GEN_RESULT
    OUTPUT_VARIABLE GLAD_GEN_OUTPUT
    ERROR_VARIABLE GLAD_GEN_ERROR
)

# Check if essential files were generated (even if process returned error due to network issues)
if(NOT EXISTS "${GLAD_GENERATED_SOURCE}" OR NOT EXISTS "${GLAD_GENERATED_HEADER}")
    if(NOT GLAD_GEN_RESULT EQUAL 0)
        message(WARNING "GLAD generation had errors: ${GLAD_GEN_ERROR}")
    endif()
    message(FATAL_ERROR "GLAD files were not generated. Expected: ${GLAD_GENERATED_SOURCE} and ${GLAD_GENERATED_HEADER}")
endif()

# Copy khrplatform.h if it wasn't generated (due to network issues)
if(NOT EXISTS "${GLAD_GENERATED_KHR}")
    file(MAKE_DIRECTORY "${GLAD_DIR}/include/KHR")
    file(COPY "${GLAD_SOURCES_DIR}/glad/files/khrplatform.h" DESTINATION "${GLAD_DIR}/include/KHR")
    message(STATUS "Copied khrplatform.h to ${GLAD_DIR}/include/KHR/")
endif()

# Create GLAD library
add_library(glad_gl STATIC
    "${GLAD_GENERATED_SOURCE}"
)

target_include_directories(glad_gl PUBLIC "${GLAD_DIR}/include")
target_link_libraries(glad_gl PUBLIC ${CMAKE_DL_LIBS})

# ---------------------------------------------------------------

# ------------------------ Vendor:  stb_image ------------------------
# Fetch stb_image
FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        f1c79c0
)
FetchContent_MakeAvailable(stb_image)
# ---------------------------------------------------------------

# ------------------------ Vendor:  imgui ------------------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.92.0-docking
)
FetchContent_MakeAvailable(imgui)
# ---------------------------------------------------------------

# ------------------------ Vendor:  glm ------------------------
# Fetch glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)
# ---------------------------------------------------------------

# ------------------------ Vendor:  assimp ------------------------
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v6.0.0
)
FetchContent_MakeAvailable(assimp)
# ---------------------------------------------------------------

# ------------------------ Vendor: periscope ------------------------
FetchContent_Declare(
  periscope
  GIT_REPOSITORY https://github.com/KeyFicller/periscope.git
  GIT_TAG        4c0e70a
)
FetchContent_MakeAvailable(periscope)
# ---------------------------------------------------------------

# ------------------------ Vendor: ImGuiColorTextEdit ------------------------
# Using git submodule instead of FetchContent because it's modified by current project
set(ImGuiColorTextEdit_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/ImGuiColorTextEdit")
message(STATUS "ImGuiColorTextEdit_SOURCE_DIR: ${ImGuiColorTextEdit_SOURCE_DIR}")
# ---------------------------------------------------------------

# ------------------------ Vendor: Mono -------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(MONO QUIET mono-2)
  if(NOT MONO_FOUND)
    pkg_check_modules(MONO QUIET mono-2.0)
  endif()
endif()
if(NOT MONO_FOUND)
  message(STATUS "Mono not found: C# script editor disabled (install Mono + pkg-config mono-2 to enable).")
endif()
# ---------------------------------------------------------------

# ------------------------ Vendor: FreeType -----------------------
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG        VER-2-14-0
)
FetchContent_MakeAvailable(freetype)
# ---------------------------------------------------------------

# ------------------------ Vendor: nativefiledialog-extended -----
set(NFD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(NFD_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  nativefiledialog-extended
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
  GIT_TAG v1.1.1
)
FetchContent_MakeAvailable(nativefiledialog-extended)
# ---------------------------------------------------------------

# ImGui sources
# Note: We use imgui_opengl3_glad.cpp to ensure GLAD is included before
# imgui_impl_opengl3.cpp (required when using IMGUI_IMPL_OPENGL_LOADER_CUSTOM)
set(IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "vendor/imgui_opengl3_glad.cpp"
)

set(LEARNOPENGL_SOURCES main.cpp callbacks.cpp resource_root.cpp)
if(MONO_FOUND)
  list(APPEND LEARNOPENGL_SOURCES scripts/mono_invoker.cpp)
endif()
add_executable(LearnOpenGL 
    ${LEARNOPENGL_SOURCES}
    basic/shader.cpp 
    basic/texture.cpp
    basic/framebuffer.cpp
    basic/light.cpp 
    basic/imgui_font_setup.cpp
    basic/material.cpp
    basic/vertex_array.cpp
    basic/camera.cpp
    vendor/imgui_opengl3_glad.cpp
    tests/framework/test_suit.cpp
    tests/scenes/scene_base.cpp
    tests/component/mesh_manager.cpp
    tests/component/shader_loader.cpp
    tests/component/prefab_quad.cpp
    tests/component/shader_editor.cpp
    tests/scenes/renderable_scene_base.cpp
    tests/scenes/texture_test_scene.cpp
    tests/scenes/triangle_test_scene.cpp
    tests/scenes/color_test_scene.cpp
    tests/scenes/transform_test_scene.cpp
    tests/scenes/coordinate_test_scene.cpp
    tests/scenes/camera_scene_base.cpp
    tests/scenes/camera_test_scene.cpp
    tests/scenes/light_color_scene.cpp
    tests/scenes/light_material_scene.cpp
    tests/scenes/light_texture_scene.cpp
    tests/scenes/light_type_scene.cpp
    tests/scenes/multiple_light_scene.cpp
    tests/scenes/import_mesh.cpp
    tests/scenes/import_model.cpp
    tests/scenes/import_model_scene.cpp
    tests/scenes/depth_test_scene.cpp
    tests/component/camera_controller.cpp
    tests/component/prefab_cube.cpp
    tests/scenes/stencil_test_scene.cpp
    tests/scenes/stencil_sub_scenes.cpp
    tests/scenes/blender_test_scene.cpp
    tests/scenes/cull_test_scene.cpp
    tests/scenes/texture_cube_scene.cpp
    tests/scenes/advanced_glsl_scene.cpp
    tests/scenes/advanced_glsl_sub_scenes.cpp
    tests/scenes/geometry_shader_scene.cpp
    tests/scenes/geometry_shader_sub_scenes.cpp
    tests/scenes/tessellation_shader_scene.cpp
    tests/scenes/instance_scene.cpp
    tests/scenes/instance_sub_scenes.cpp
    tests/scenes/spline_movement_scene.cpp
    tests/scenes/spline_movement_sub_scenes.cpp
    tests/scenes/spline_movement_driver.cpp
    tests/scenes/reveal_chess_scene.cpp
    tests/scenes/soft_body_frog_scene.cpp
    tests/scenes/occt_demo_scene.cpp
    tests/scenes/shader_editor_scene.cpp
    tests/component/text_renderer.cpp
    tests/component/connection.cpp
    tests/component/interaction_utils.cpp
    tests/component/opengl_shader_definition.cpp
    ${IMGUI_SOURCES}
)

if(MONO_FOUND)
  target_sources(LearnOpenGL PRIVATE
    tests/scenes/script_editor_scene.cpp
    tests/component/script_editor.cpp)
endif()

if(MSVC)
  # UTF-8 source/exec charset for project sources only (u8 literals, comments).
  # Do not use add_compile_options(/utf-8): FetchContent deps may set /source-charset and conflict (D8016).
  target_compile_options(LearnOpenGL PRIVATE /utf-8)
endif()

target_link_libraries(LearnOpenGL PRIVATE glfw glad_gl assimp freetype nfd)
if(WIN32)
  target_link_libraries(LearnOpenGL PRIVATE ws2_32)
endif()

# Optional Open CASCADE Technology (OCCT) demo scene:
# Point LEARNOPENGL_OCCT_BUILD_DIR at your OCCT *build* directory (the one that
# contains OpenCASCADETargets.cmake). Override in CMake or set empty to disable.
set(LEARNOPENGL_OCCT_BUILD_DIR
    "/Users/keyficller/Documents/OCCT/build"
    CACHE PATH
    "OCCT build directory (contains OpenCASCADETargets.cmake); set empty to disable OCCT"
)
if(LEARNOPENGL_OCCT_BUILD_DIR AND EXISTS "${LEARNOPENGL_OCCT_BUILD_DIR}/OpenCASCADETargets.cmake")
  set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG RELWITHDEBINFO)
  set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE RELWITHDEBINFO)
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELWITHDEBINFO)
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL RELWITHDEBINFO)
  include("${LEARNOPENGL_OCCT_BUILD_DIR}/OpenCASCADETargets.cmake")
  target_compile_definitions(LearnOpenGL PRIVATE LEARNOPENGL_USE_OCCT=1)
  # Toy CAD scene + tests/cad (compiled only when OCCT is enabled; no #ifdef in those sources).
  target_sources(LearnOpenGL PRIVATE
      tests/scenes/toy_cad_scene.cpp
      tests/cad/renderer/viewport_axes_gizmo.cpp
      tests/cad/renderer/viewport_axis.cpp
      tests/cad/renderer/viewport_datum.cpp
      tests/cad/renderer/highlight_renderer.cpp
      tests/cad/database/database.cpp
      tests/cad/database/object.cpp
      tests/cad/database/datum.cpp
      tests/cad/database/axis.cpp
      tests/cad/renderer/drawable_object.cpp
      tests/cad/history/history_object.cpp
      tests/cad/history/feature.cpp
      tests/cad/history/primitive_feature.cpp
      tests/cad/history/box_feature.cpp
      tests/cad/history/box_command.cpp
      tests/cad/history/history_tree.cpp
      tests/cad/history/coordinate.cpp
      tests/cad/geometry/subshape_handle.cpp
      tests/cad/geometry/occt_mesh_utils.cpp
      tests/cad/geometry/bvh_tree.cpp
      tests/cad/document/command_panel.cpp
      tests/cad/document/debug_panel.cpp
      tests/cad/document/inspector_panel.cpp
      tests/cad/interaction/command_dispatcher.cpp
      tests/cad/interaction/doc_input_handler.cpp
      tests/cad/interaction/point_input_handler.cpp
      tests/cad/interaction/ray_pick.cpp
      tests/cad/interaction/selection_manager.cpp
      tests/cad/instance.cpp)
  target_include_directories(LearnOpenGL PRIVATE
      "${LEARNOPENGL_OCCT_BUILD_DIR}/include/opencascade")
  target_link_libraries(LearnOpenGL PRIVATE TKPrim TKMesh TKBO)
  # Locate OCCT shared libraries for runtime (macOS build tree layout)
  set(LEARNOPENGL_OCCT_LIBDIR "")
  file(GLOB LEARNOPENGL_OCCT_LIBDIRS "${LEARNOPENGL_OCCT_BUILD_DIR}/mac64/*/libi")
  if(LEARNOPENGL_OCCT_LIBDIRS)
    list(GET LEARNOPENGL_OCCT_LIBDIRS 0 LEARNOPENGL_OCCT_LIBDIR)
  endif()
  if(NOT LEARNOPENGL_OCCT_LIBDIR AND EXISTS "${LEARNOPENGL_OCCT_BUILD_DIR}/mac64/clang/libi")
    set(LEARNOPENGL_OCCT_LIBDIR "${LEARNOPENGL_OCCT_BUILD_DIR}/mac64/clang/libi")
  endif()
  if(LEARNOPENGL_OCCT_LIBDIR)
    set_property(TARGET LearnOpenGL APPEND PROPERTY BUILD_RPATH
        "${LEARNOPENGL_OCCT_LIBDIR}")
    set_property(TARGET LearnOpenGL APPEND PROPERTY INSTALL_RPATH
        "${LEARNOPENGL_OCCT_LIBDIR}")
  endif()
  message(STATUS "Open CASCADE enabled: ${LEARNOPENGL_OCCT_BUILD_DIR}")
else()
  set(LEARNOPENGL_OCCT_LIBDIR "")
  if(LEARNOPENGL_OCCT_BUILD_DIR)
    message(WARNING "LEARNOPENGL_OCCT_BUILD_DIR set but OpenCASCADETargets.cmake not found; OCCT demo disabled.")
  endif()
endif()
if(MONO_FOUND)
  target_link_directories(LearnOpenGL PRIVATE ${MONO_LIBRARY_DIRS})
  target_link_libraries(LearnOpenGL PRIVATE ${MONO_LIBRARIES})
  target_include_directories(LearnOpenGL PRIVATE ${MONO_INCLUDE_DIRS})
  target_compile_definitions(LearnOpenGL PRIVATE LEARNOPENGL_USE_MONO=1)
  # Help Mono find System.Native and other runtime libs at load time;
  # OCCT dylibs use @rpath too — merge paths (plain INSTALL_RPATH overwrites
  # earlier APPEND values and breaks OCCT when BUILD_WITH_INSTALL_RPATH is on).
  if(APPLE)
    set(_learn_rpath "/opt/homebrew/lib;/usr/local/lib")
    if(LEARNOPENGL_OCCT_LIBDIR)
      set(_learn_rpath "${LEARNOPENGL_OCCT_LIBDIR};${_learn_rpath}")
    endif()
    set_target_properties(LearnOpenGL PROPERTIES
      INSTALL_RPATH "${_learn_rpath}"
      BUILD_WITH_INSTALL_RPATH TRUE)
  endif()
endif()
target_include_directories(LearnOpenGL PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${stb_image_SOURCE_DIR}" 
    "${glm_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${assimp_SOURCE_DIR}/include"
    "${periscope_SOURCE_DIR}/include"
    "${ImGuiColorTextEdit_SOURCE_DIR}"
    "${freetype_SOURCE_DIR}/include"
)

# Check if ImGuiColorTextEdit submodule exists
if(NOT EXISTS "${ImGuiColorTextEdit_SOURCE_DIR}/TextEditor.h")
    message(WARNING "ImGuiColorTextEdit submodule not found at ${ImGuiColorTextEdit_SOURCE_DIR}")
    message(WARNING "Please run: git submodule update --init --recursive")
endif()

# Use GLAD as OpenGL loader for ImGui (instead of imgui's built-in loader)
target_compile_definitions(LearnOpenGL PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)