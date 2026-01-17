cmake_minimum_required(VERSION 3.15)

project(LearnOpenGL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
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
  GIT_REPOSITORY https://bgithub.xyz/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_MakeAvailable(glfw)
# ---------------------------------------------------------------

# ------------------------ Vendor:  GLAD(Generator) ------------------------

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://bgithub.xyz/Dav1dde/glad.git
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

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH="${GLAD_SOURCES_DIR}" ${Python3_EXECUTABLE} -m glad --out-path "${GLAD_DIR}" --api gl:core=3.3 c --loader
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
  GIT_REPOSITORY https://bgithub.xyz/nothings/stb.git
  GIT_TAG        f1c79c0
)
FetchContent_MakeAvailable(stb_image)
# ---------------------------------------------------------------

# ------------------------ Vendor:  imgui ------------------------
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://bgithub.xyz/ocornut/imgui.git
  GIT_TAG        v1.92.0-docking
)
FetchContent_MakeAvailable(imgui)
# ---------------------------------------------------------------

# ------------------------ Vendor:  glm ------------------------
# Fetch glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://bgithub.xyz/g-truc/glm.git
  GIT_TAG        1.0.3
)
FetchContent_MakeAvailable(glm)
# ---------------------------------------------------------------

# ------------------------ Vendor:  assimp ------------------------
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://bgithub.xyz/assimp/assimp.git
  GIT_TAG        v6.0.0
)
FetchContent_MakeAvailable(assimp)
# ---------------------------------------------------------------

# ImGui sources
# Note: We use a wrapper for imgui_impl_opengl3.cpp to include GLAD before it
set(IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "basic/imgui_impl_opengl3_wrapper.cpp"
)

add_executable(LearnOpenGL 
    main.cpp 
    basic/shader.cpp 
    basic/texture.cpp
    basic/framebuffer.cpp
    basic/imgui_impl_opengl3_wrapper.cpp
    tests/framework/test_suit.cpp
    tests/scenes/scene_base.cpp
    tests/component/mesh_manager.cpp
    tests/component/shader_loader.cpp
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
    ${IMGUI_SOURCES}
)

target_link_libraries(LearnOpenGL PRIVATE glfw glad_gl assimp)
target_include_directories(LearnOpenGL PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${stb_image_SOURCE_DIR}" 
    "${glm_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${assimp_SOURCE_DIR}/include"
)

# Use GLAD as OpenGL loader for ImGui (instead of imgui's built-in loader)
target_compile_definitions(LearnOpenGL PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM)