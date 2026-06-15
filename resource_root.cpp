#include "resource_root.h"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <unistd.h>
#elif defined(__linux__)
#include <limits.h>
#include <unistd.h>
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#ifndef PATH_MAX
#if defined(_WIN32) && defined(MAX_PATH)
#define PATH_MAX MAX_PATH
#elif defined(MAXPATHLEN)
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 4096
#endif
#endif

namespace fs = std::filesystem;

namespace {

static bool is_resource_root(const fs::path &dir) {
  std::error_code ec;
  return fs::is_directory(dir / "shaders", ec) &&
         fs::is_directory(dir / "assets", ec);
}

static std::string get_executable_path() {
#if defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  if (size == 0)
    return {};
  std::string buf(size + 1, '\0');
  if (_NSGetExecutablePath(&buf[0], &size) != 0)
    return {};
  buf.resize(std::strlen(buf.c_str()));
  char resolved[PATH_MAX];
  if (realpath(buf.c_str(), resolved))
    return resolved;
  return buf;
#elif defined(__linux__)
  char buf[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0)
    return {};
  buf[n] = '\0';
  return buf;
#elif defined(_WIN32)
  std::string buf(static_cast<size_t>(32768), '\0');
  DWORD n = GetModuleFileNameA(nullptr, buf.data(),
                               static_cast<DWORD>(buf.size()));
  if (n == 0)
    return {};
  buf.resize(n);
  return buf;
#else
  return {};
#endif
}

} // namespace

bool set_resource_root_as_cwd() {
  const int max_ancestors = 8;
  fs::path candidates[max_ancestors];
  int n = 0;

  std::error_code ec;
  fs::path cwd = fs::current_path(ec);
  if (!ec)
    candidates[n++] = cwd;

  std::string exe = get_executable_path();
  if (!exe.empty()) {
    fs::path dir = fs::path(exe).lexically_normal().parent_path();
    for (int i = 0; i < max_ancestors && n < max_ancestors; ++i) {
      if (dir.empty() || dir == fs::path("."))
        break;
      candidates[n++] = dir;
      fs::path parent = dir.parent_path();
      if (parent == dir)
        break;
      dir = parent;
    }
  }

  for (int i = 0; i < n; ++i) {
    if (!is_resource_root(candidates[i]))
      continue;
    std::error_code ch_ec;
    fs::current_path(candidates[i], ch_ec);
    if (!ch_ec) {
      std::cout << "Resource root: " << candidates[i].string() << std::endl;
      return true;
    }
    std::cerr << "Resource root found but chdir failed: "
              << candidates[i].string() << " (" << ch_ec.message() << ")"
              << std::endl;
    return false;
  }

  std::cerr << "Resource root not found (no directory containing both "
               "'shaders' and 'assets')."
            << std::endl;
  return false;
}
