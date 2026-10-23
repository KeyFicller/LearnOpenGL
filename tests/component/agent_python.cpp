#include "tests/component/agent_python.h"

#include <cstdlib>
#include <string>

namespace fs = std::filesystem;

std::optional<fs::path> resolve_agent_python() {
  if (const char *env = std::getenv("LEARNOPENGL_PYTHON")) {
    const fs::path override_path(env);
    std::error_code ec;
    if (!override_path.empty() && fs::exists(override_path, ec) && !ec) {
      return fs::absolute(override_path);
    }
  }

  // Prefer the project venv. resource_root has already chdir'd to the repo
  // root, so these relative paths are stable whether the binary lives in
  // build/ or elsewhere. Check Unix and Windows layouts so a shared tree
  // works on either host.
  static const char *const k_relatives[] = {
      ".venv/bin/python",
      ".venv/bin/python3",
      ".venv/Scripts/python.exe",
      ".venv/Scripts/python",
  };
  for (const char *rel : k_relatives) {
    std::error_code ec;
    const fs::path candidate = fs::absolute(rel);
    if (fs::exists(candidate, ec) && !ec) {
      return candidate;
    }
  }
  return std::nullopt;
}

std::string agent_python_missing_hint(const char *script_relpath) {
  std::string msg =
      "No agent Python found (tried LEARNOPENGL_PYTHON and .venv). "
      "Create .venv, pip install the agent requirements, or set "
      "LEARNOPENGL_PYTHON.";
  if (script_relpath && script_relpath[0] != '\0') {
    msg += " Expected script: ";
    msg += script_relpath;
  }
  return msg;
}
