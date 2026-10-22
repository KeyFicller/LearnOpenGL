# LearnOpenGL Test Suite

OpenGL / ImGui experiments (spline motion, soft body, ShaderToy-like editor, chess, toy CAD).

## Build

### Prerequisites

- **CMake** 3.25+ (presets), Ninja or Unix Makefiles
- **clang-p2996** (P2996 static reflection), with reflective libc++:
  - `<CPPREF_LLVM_ROOT>/bin/clang++`
  - `<CPPREF_LLVM_ROOT>/include/c++/v1/meta`
  - `<CPPREF_LLVM_ROOT>/lib` (runtime / rpath)
- Network access to GitHub for FetchContent deps (GLFW, GLAD, …), or git `url.insteadOf` mirrors via your local preset

Set the toolchain prefix with either:

```bash
export CPPREF_LLVM_ROOT=/path/to/llvm-p2996
# or: cmake -DCPPREF_LLVM_ROOT=/path/to/llvm-p2996 ...
```

### Submodules

This repo vendors a modified [ImGuiColorTextEdit](https://github.com/KeyFicller/ImGuiColorTextEdit) under `vendor/ImGuiColorTextEdit`. Initialize it after clone:

```bash
git submodule update --init --recursive
```

Or clone with submodules in one step:

```bash
git clone --recurse-submodules https://github.com/KeyFicller/LearnOpenGL.git
```

Without the submodule, CMake configures with a warning and the shader editor path that needs `TextEditor.h` will not build cleanly.

### Configure and build

```bash
# one-time: machine-local preset (gitignored)
cp CMakeUserPresets.json.example CMakeUserPresets.json
# edit CPPREF_LLVM_ROOT inside CMakeUserPresets.json

cmake --preset local
cmake --build --preset local
```

`CMakePresets.json` `default` also works if `CPPREF_LLVM_ROOT` is already in the environment.

## Spline movement program animation
![Spline Movement](doc/spline_movement_demo.png)
## Soft body animation
![Soft Body](doc/soft_body.gif)
## Shader editor (like ShaderToy)
![Shader inTime Editor](doc/shader_editor_demo.png)
## Reveal chess
![Reveal Chess](doc/reveal_chess.png)
## Toy CAD
![Toy CAD](doc/toy_cad_demo.png)