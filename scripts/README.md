# C# Scripting (Mono)

C# 脚本通过 Mono 运行时嵌入到 LearnOpenGL 项目中。

## 环境

- **macOS**: `brew install mono` 已安装
- **Linux**: `sudo apt install mono-complete` 或 `sudo dnf install mono-devel`
- **Windows**: 安装 [Mono for Windows](https://www.mono-project.com/download/stable/)

## 构建 C# 脚本

```bash
./scripts/build_csharp.sh
```

或在构建 LearnOpenGL 时自动执行（CMake 会调用该脚本并复制 DLL 到输出目录）。

## 运行

1. 构建项目：`cmake -B build && cmake --build build`
2. 从项目根目录运行：`./build/LearnOpenGL`（或 `./build/Debug/LearnOpenGL`）
3. 启动时会尝试加载 `scripts/csharp/bin/Scripts.dll`，并调用 `Scripts.ExampleScript::SayHello()`

若控制台输出 `Hello from C# script!`，说明 Mono 集成成功。

## 添加新脚本

1. 在 `scripts/csharp/` 下添加 `.cs` 文件
2. 修改 `build_csharp.sh` 中的编译命令（如使用 mcs，需包含新文件）
3. 在 C++ 中调用：`mono_host::invoke_static("Scripts.YourClass::YourMethod")`

## API

- `mono_host::init(path)` - 初始化 Mono 并加载程序集
- `mono_host::shutdown()` - 关闭 Mono
- `mono_host::invoke_static("Namespace.Class::Method")` - 调用静态无参方法
- `mono_host::invoke_static("Namespace.Class::Method", float)` - 调用带一个 float 参数的静态方法
