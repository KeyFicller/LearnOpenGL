#include "tests/component/reveal_chess_chat.h"

#include "imgui.h"

#include <nlohmann/json.hpp>

#include <cerrno>
#include <ctime>
#include <fcntl.h>
#include <filesystem>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

reveal_chess_chat::~reveal_chess_chat() { stop(); }

void reveal_chess_chat::start(bool thinking, int move_seconds,
                              const std::string &stamp) {
  if (m_pid > 0) {
    return;
  }
  const fs::path python = fs::absolute(".venv/bin/python");
  const fs::path script = fs::absolute("scripts/chess_agent/play.py");
  if (!fs::exists(python) || !fs::exists(script)) {
    m_log.push_back(
        {"error", "Missing .venv/bin/python or scripts/chess_agent/play.py"});
    m_scroll = true;
    return;
  }

  int from_child[2] = {-1, -1};
  if (pipe(from_child) != 0) {
    m_log.push_back({"error", "Could not open the AI pipe."});
    m_scroll = true;
    return;
  }
  setenv("PYTHONUNBUFFERED", "1", 1);
  setenv("CHESS_THINKING", thinking ? "1" : "0", 1);
  setenv("CHESS_MOVE_SECONDS", std::to_string(move_seconds).c_str(), 1);
  setenv("CHESS_LOG_STAMP", stamp.c_str(), 1);

  const std::string python_s = python.string();
  const std::string script_s = script.string();
  const pid_t pid = fork();
  if (pid < 0) {
    close(from_child[0]);
    close(from_child[1]);
    m_log.push_back({"error", "Could not start the AI process."});
    m_scroll = true;
    return;
  }
  if (pid == 0) {
    setpgid(0, 0);
    dup2(from_child[1], STDOUT_FILENO);
    const int devnull_in = open("/dev/null", O_RDONLY);
    const int devnull_out = open("/dev/null", O_WRONLY);
    if (devnull_in >= 0) {
      dup2(devnull_in, STDIN_FILENO);
      close(devnull_in);
    }
    if (devnull_out >= 0) {
      dup2(devnull_out, STDERR_FILENO);
      close(devnull_out);
    }
    close(from_child[0]);
    close(from_child[1]);
    char *argv[] = {const_cast<char *>(python_s.c_str()),
                    const_cast<char *>(script_s.c_str()),
                    const_cast<char *>("--ui"), nullptr};
    execv(argv[0], argv);
    _exit(127);
  }

  setpgid(pid, pid);
  close(from_child[1]);
  fcntl(from_child[0], F_SETFL, O_NONBLOCK);
  m_pid = pid;
  m_out = from_child[0];
  m_buf.clear();
  m_thinking = false;
  m_thinking_side.clear();
  // The models themselves come from .env, so the UI names none of them.
  m_log.push_back({"status", "Red vs Black (models from .env)"});
  m_scroll = true;
}

void reveal_chess_chat::stop() {
  m_thinking = false;
  m_thinking_side.clear();
  if (m_pid > 0) {
    // The child is its own process group, so this also reaps anything it
    // spawned.
    if (kill(-m_pid, SIGTERM) != 0)
      kill(m_pid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
      if (waitpid(m_pid, nullptr, WNOHANG) == m_pid) {
        m_pid = -1;
        break;
      }
      usleep(10000);
    }
    if (m_pid > 0) {
      if (kill(-m_pid, SIGKILL) != 0)
        kill(m_pid, SIGKILL);
      waitpid(m_pid, nullptr, 0);
      m_pid = -1;
    }
  }
  if (m_out >= 0) {
    close(m_out);
    m_out = -1;
  }
}

void reveal_chess_chat::handle_line(const std::string &line) {
  if (line.empty()) {
    return;
  }
  if (line.find("Performance Diagnostics") != std::string::npos ||
      line.find("should not be called on the main thread") !=
          std::string::npos) {
    return;
  }
  std::string kind = "log";
  std::string text = line;
  const nlohmann::json doc = nlohmann::json::parse(line, nullptr, false);
  std::string side;
  if (doc.is_object() && doc.contains("kind")) {
    kind = doc.value("kind", "log");
    text = doc.value("text", "");
    side = doc.value("side", "");
  }
  if (text.empty()) {
    return;
  }
  if (kind == "status" && text == "thinking…") {
    m_thinking = true;
    m_thinking_side = side;
  } else if (kind == "move" || kind == "error" || kind == "status") {
    m_thinking = false;
    m_thinking_side.clear();
  }
  m_log.push_back({kind, text, side});
  if (m_log.size() > 400) {
    m_log.erase(m_log.begin(), m_log.begin() + (m_log.size() - 400));
  }
  m_scroll = true;
}

void reveal_chess_chat::note(const std::string &kind, const std::string &text,
                             const std::string &side) {
  handle_line(
      nlohmann::json{{"kind", kind}, {"text", text}, {"side", side}}.dump());
}

void reveal_chess_chat::poll() {
  if (m_out < 0) {
    return;
  }
  char buf[4096];
  while (true) {
    const ssize_t n = read(m_out, buf, sizeof(buf));
    if (n > 0) {
      m_buf.append(buf, static_cast<size_t>(n));
      continue;
    }
    if (n == 0) {
      m_log.push_back({"status", "AI process exited."});
      m_scroll = true;
      stop();
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
    m_log.push_back({"error", "Lost the AI process."});
    m_scroll = true;
    stop();
    return;
  }
  for (std::size_t nl = m_buf.find('\n'); nl != std::string::npos;
       nl = m_buf.find('\n')) {
    std::string line = m_buf.substr(0, nl);
    m_buf.erase(0, nl + 1);
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    handle_line(line);
  }
}

void reveal_chess_chat::draw() {
  poll();
  ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
  ImGui::Begin("Chess AI");
  ImGui::TextUnformatted(m_pid > 0 ? "Match in progress."
                                   : "Choose models, then Start.");

  ImGui::BeginChild("chess_ai_log", ImVec2(0.0f, 0.0f),
                    ImGuiChildFlags_Borders);
  for (const entry &item : m_log) {
    const bool red = item.side == "red";
    const char *who = red ? "红 " : item.side == "black" ? "黑 " : "";
    const char *label = "";
    ImVec4 color = red ? ImVec4(0.95f, 0.45f, 0.38f, 1.0f)
                       : ImVec4(0.55f, 0.75f, 0.95f, 1.0f);
    if (item.side.empty())
      color = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
    if (item.kind == "think") {
      label = "think: ";
    } else if (item.kind == "agent") {
      label = "";
    } else if (item.kind == "aside") {
      label = "内心: ";
    } else if (item.kind == "taunt") {
      label = "狠话: ";
    } else if (item.kind == "summary") {
      label = "总结: ";
    } else if (item.kind == "memory") {
      label = "记忆: ";
      color = ImVec4(0.65f, 0.85f, 0.65f, 1.0f);
    } else if (item.kind == "move") {
      label = "move: ";
    } else if (item.kind == "tool") {
      label = "tool: ";
      color = ImVec4(0.85f, 0.70f, 0.40f, 1.0f);
    } else if (item.kind == "error") {
      color = ImVec4(1.0f, 0.40f, 0.40f, 1.0f);
    } else if (item.kind == "timeout") {
      label = "超时: ";
      color = ImVec4(1.0f, 0.55f, 0.25f, 1.0f);
    } else if (item.kind == "status") {
      color = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    if (item.text.find('\n') != std::string::npos)
      ImGui::TextUnformatted((std::string(who) + label + item.text).c_str());
    else
      ImGui::TextWrapped("%s%s%s", who, label, item.text.c_str());
    ImGui::PopStyleColor();
  }
  if (m_scroll) {
    ImGui::SetScrollHereY(1.0f);
    m_scroll = false;
  }
  ImGui::EndChild();
  ImGui::End();
}
