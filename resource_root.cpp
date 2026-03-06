#include "resource_root.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(__linux__)
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 4096
#endif
#endif

namespace {

static bool dir_contains(const std::string &dir, const char *sub) {
  std::string path = dir;
  if (!path.empty() && path.back() != '/')
    path += '/';
  path += sub;
  struct stat st{};
  return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool is_resource_root(const std::string &dir) {
  return dir_contains(dir, "shaders") && dir_contains(dir, "assets");
}

static std::string dirname(const std::string &path) {
  std::string::size_type p = path.find_last_of("/\\");
  if (p == std::string::npos)
    return ".";
  if (p == 0)
    return "/";
  return path.substr(0, p);
}

static std::string get_executable_path() {
#if defined(__APPLE__)
  uint32_t size = 0;
  _NSGetExecutablePath(
      nullptr, &size); // get required size (returns -1 when buffer too small)
  if (size == 0)
    return {};
  std::string buf(size + 1, '\0');
  if (_NSGetExecutablePath(&buf[0], &size) != 0)
    return {};
  buf.resize(strlen(buf.c_str()));
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
#else
  (void)0;
  return {};
#endif
}

} // namespace

bool set_resource_root_as_cwd() {
  const int max_ancestors = 8;
  std::string candidates[max_ancestors];
  int n = 0;

  // 1) Current working directory
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)))
    candidates[n++] = cwd;

  // 2) Executable directory and its parent directories (for build/Debug)
  std::string exe = get_executable_path();
  if (!exe.empty()) {
    std::string dir = dirname(exe);
    for (int i = 0; i < max_ancestors && n < max_ancestors; ++i) {
      if (dir.empty() || dir == ".")
        break;
      candidates[n++] = dir;
      std::string parent = dirname(dir);
      if (parent == dir)
        break;
      dir = parent;
    }
  }

  for (int i = 0; i < n; ++i) {
    if (is_resource_root(candidates[i])) {
      if (chdir(candidates[i].c_str()) == 0) {
        std::cout << "Resource root: " << candidates[i] << std::endl;
        return true;
      }
      std::cerr << "Resource root found but chdir failed: " << candidates[i]
                << " (" << strerror(errno) << ")" << std::endl;
      return false;
    }
  }

  std::cerr << "Resource root not found (no directory containing both "
               "'shaders' and 'assets')."
            << std::endl;
  return false;
}
