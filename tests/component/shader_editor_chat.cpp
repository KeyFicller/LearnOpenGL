#include "tests/component/shader_editor_chat.h"

#include "basic/texture.h"
#include "imgui.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

shader_editor_chat::~shader_editor_chat() { stop(); }

void shader_editor_chat::start() {
  if (m_pid > 0) {
    return;
  }
  const fs::path python = fs::absolute(".venv/bin/python");
  const fs::path script = fs::absolute("scripts/shader_agent/chat.py");
  if (!fs::exists(python) || !fs::exists(script)) {
    m_log.push_back(
        {"error", "Missing .venv/bin/python or scripts/shader_agent/chat.py"});
    m_scroll = true;
    return;
  }

  int to_child[2] = {-1, -1};
  int from_child[2] = {-1, -1};
  if (pipe(to_child) != 0 || pipe(from_child) != 0) {
    if (to_child[0] >= 0) {
      close(to_child[0]);
      close(to_child[1]);
    }
    m_log.push_back({"error", "Could not open the chat pipes."});
    m_scroll = true;
    return;
  }
  setenv("PYTHONUNBUFFERED", "1", 1);

  const std::string python_s = python.string();
  const std::string script_s = script.string();
  const pid_t pid = fork();
  if (pid < 0) {
    close(to_child[0]);
    close(to_child[1]);
    close(from_child[0]);
    close(from_child[1]);
    m_log.push_back({"error", "Could not start the chat process."});
    m_scroll = true;
    return;
  }
  if (pid == 0) {
    // Only async-signal-safe calls until exec: other threads hold locks.
    dup2(to_child[0], STDIN_FILENO);
    dup2(from_child[1], STDOUT_FILENO);
    // stderr stays out of the transcript. macOS writes a Security-framework
    // diagnostic there during proxy lookup, and chat errors already go to stdout.
    const int devnull = open("/dev/null", O_WRONLY);
    if (devnull >= 0) {
      dup2(devnull, STDERR_FILENO);
      close(devnull);
    }
    close(to_child[0]);
    close(to_child[1]);
    close(from_child[0]);
    close(from_child[1]);
    char *argv[] = {const_cast<char *>(python_s.c_str()),
                    const_cast<char *>(script_s.c_str()),
                    const_cast<char *>("--ui"), nullptr};
    execv(argv[0], argv);
    _exit(127);
  }

  close(to_child[0]);
  close(from_child[1]);
  fcntl(from_child[0], F_SETFL, O_NONBLOCK);
  m_pid = pid;
  m_in = to_child[1];
  m_out = from_child[0];
  m_ready = false;
  m_buf.clear();
  m_log.push_back({"status", "Opening chat…"});
  m_scroll = true;
}

void shader_editor_chat::stop() {
  if (m_in >= 0) {
    close(m_in);
    m_in = -1;
  }
  if (m_pid > 0) {
    kill(m_pid, SIGTERM);
    for (int i = 0; i < 20; ++i) {
      if (waitpid(m_pid, nullptr, WNOHANG) == m_pid) {
        m_pid = -1;
        break;
      }
      usleep(10000);
    }
    if (m_pid > 0) {
      kill(m_pid, SIGKILL);
      waitpid(m_pid, nullptr, 0);
      m_pid = -1;
    }
  }
  if (m_out >= 0) {
    close(m_out);
    m_out = -1;
  }
  m_ready = false;
  m_preview.reset();
  m_preview_path.clear();
}

void shader_editor_chat::handle_line(const std::string &line) {
  if (line.empty()) {
    return;
  }
  // macOS writes this to the chat process stderr during a proxy lookup.
  // It is not a chat message.
  if (line.find("Performance Diagnostics") != std::string::npos ||
      line.find("should not be called on the main thread") != std::string::npos) {
    return;
  }
  std::string kind = "log";
  std::string text = line;
  const nlohmann::json doc = nlohmann::json::parse(line, nullptr, false);
  if (doc.is_object() && doc.contains("kind")) {
    kind = doc.value("kind", "log");
    text = doc.value("text", "");
    if (kind == "preview") {
      text = doc.value("path", "");
    }
  }
  if (kind == "ready") {
    m_ready = true;
    return;
  }
  if (kind == "preview" && !text.empty()) {
    try {
      m_preview = std::make_unique<texture_2d>(text.c_str());
      m_preview_path = text;
    } catch (const std::exception &e) {
      m_log.push_back({"error", e.what()});
    }
  }
  m_log.push_back({kind, text});
  if (m_log.size() > 400) {
    m_log.erase(m_log.begin(), m_log.begin() + (m_log.size() - 400));
  }
  m_scroll = true;
}

void shader_editor_chat::poll() {
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
      m_log.push_back({"status", "Chat closed."});
      m_scroll = true;
      stop();
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      break;
    }
    m_log.push_back({"error", "Lost the chat process."});
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

void shader_editor_chat::send() {
  if (!m_ready || m_in < 0) {
    return;
  }
  const std::string line(m_input);
  if (line.empty()) {
    return;
  }
  const std::string payload = line + "\n";
  std::size_t sent = 0;
  while (sent < payload.size()) {
    const ssize_t n = write(m_in, payload.data() + sent, payload.size() - sent);
    if (n < 0) {
      m_log.push_back({"error", "Could not send the message."});
      m_scroll = true;
      return;
    }
    sent += static_cast<size_t>(n);
  }
  m_log.push_back({"user", line});
  m_scroll = true;
  m_ready = false;
  m_input[0] = '\0';
}

void shader_editor_chat::draw() {
  poll();
  ImGui::SetNextWindowSize(ImVec2(420.0f, 520.0f), ImGuiCond_FirstUseEver);
  ImGui::Begin("Shader Chat");
  if (m_pid <= 0) {
    if (ImGui::Button("Open chat")) {
      start();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("Starts the conversation in this window.");
  } else {
    if (ImGui::Button("Close chat")) {
      stop();
      m_log.push_back({"status", "Chat closed."});
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(m_ready ? "ready" : "thinking…");
  }

  const float footer = ImGui::GetFrameHeightWithSpacing();
  ImGui::BeginChild("shader_chat_log", ImVec2(0.0f, -footer),
                    ImGuiChildFlags_Borders);
  for (const entry &item : m_log) {
    const char *label = "";
    ImVec4 color(0.75f, 0.75f, 0.75f, 1.0f);
    if (item.kind == "user") {
      label = "you: ";
      color = ImVec4(0.45f, 0.75f, 1.0f, 1.0f);
    } else if (item.kind == "agent") {
      label = "agent: ";
      color = ImVec4(0.55f, 0.95f, 0.60f, 1.0f);
    } else if (item.kind == "tool") {
      label = "tool: ";
      color = ImVec4(0.95f, 0.78f, 0.35f, 1.0f);
    } else if (item.kind == "error") {
      color = ImVec4(1.0f, 0.40f, 0.40f, 1.0f);
    } else if (item.kind == "preview") {
      label = "preview: ";
      color = ImVec4(0.78f, 0.62f, 1.0f, 1.0f);
    }
    if (!item.text.empty() || item.kind == "preview") {
      ImGui::PushStyleColor(ImGuiCol_Text, color);
      ImGui::TextWrapped("%s%s", label, item.text.c_str());
      ImGui::PopStyleColor();
    }
    if (item.kind == "preview") {
      if (m_preview && item.text == m_preview_path) {
        const float w = std::min(280.0f, ImGui::GetContentRegionAvail().x);
        const float aspect = m_preview->height() > 0
                                 ? static_cast<float>(m_preview->width()) /
                                       static_cast<float>(m_preview->height())
                                 : 1.0f;
        ImGui::Image(
            reinterpret_cast<void *>(static_cast<intptr_t>(m_preview->ID())),
            ImVec2(w, w / aspect), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
      }
    }
  }
  if (m_scroll) {
    ImGui::SetScrollHereY(1.0f);
    m_scroll = false;
  }
  ImGui::EndChild();

  ImGui::BeginDisabled(m_pid <= 0 || !m_ready);
  const bool entered =
      ImGui::InputText("##shader_chat_input", m_input, sizeof(m_input),
                       ImGuiInputTextFlags_EnterReturnsTrue);
  ImGui::SameLine();
  const bool clicked = ImGui::Button("Send");
  ImGui::EndDisabled();
  if (entered || clicked) {
    send();
  }
  ImGui::End();
}
