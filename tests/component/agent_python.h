#pragma once

#include <filesystem>
#include <optional>
#include <string>

// Interpreter used by in-app Python agents (Shader Chat, Reveal Chess).
// Order: LEARNOPENGL_PYTHON (if set and exists), then project-local .venv
// (Unix bin/ or Windows Scripts/), else empty.
std::optional<std::filesystem::path> resolve_agent_python();

// Short UI / log hint when resolve_agent_python() returns empty.
std::string agent_python_missing_hint(const char *script_relpath);
