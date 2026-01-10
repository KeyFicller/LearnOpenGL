cmake_minimum_required(VERSION 3.15)

project(LearnOpenGL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://bgithub.xyz/glfw/glfw.git
  GIT_TAG        3.3.8
)

FetchContent_MakeAvailable(glfw)

# Fetch GLAD (generator)
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://bgithub.xyz/Dav1dde/glad.git
  GIT_TAG        v2.0.0
)

FetchContent_MakeAvailable(glad)

# Fetch stb_image
FetchContent_Declare(
  stb_image
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        f1c79c0
)

FetchContent_MakeAvailable(stb_image)

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

add_executable(LearnOpenGL main.cpp shader.cpp misc_functions.cpp texture.cpp)

target_link_libraries(LearnOpenGL PRIVATE glfw glad_gl)
target_include_directories(LearnOpenGL PUBLIC "${stb_image_SOURCE_DIR}")