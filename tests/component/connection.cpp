#include "connection.h"
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

namespace {

const int listen_backlog = 5;

}  // namespace

// -------- server --------

server::server(uint16_t port) : m_client_socket(-1) {
  m_server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_server_socket == -1) {
    throw std::runtime_error("Failed to create server socket");
  }

  int opt = 1;
  if (setsockopt(m_server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(m_server_socket);
    throw std::runtime_error("Failed to set socket options");
  }

  memset(&m_server_address, 0, sizeof(m_server_address));
  m_server_address.sin_family = AF_INET;
  m_server_address.sin_addr.s_addr = INADDR_ANY;
  m_server_address.sin_port = htons(port);

  if (bind(m_server_socket, reinterpret_cast<struct sockaddr*>(&m_server_address),
           sizeof(m_server_address)) < 0) {
    close(m_server_socket);
    throw std::runtime_error("Failed to bind server socket");
  }

  if (listen(m_server_socket, listen_backlog) < 0) {
    close(m_server_socket);
    throw std::runtime_error("Failed to listen on server socket");
  }

  std::cout << "Server listening on port " << port << std::endl;
  std::cout << "Waiting for client connection..." << std::endl;

  m_client_address_len = sizeof(m_client_address);
  m_client_socket =
      accept(m_server_socket, reinterpret_cast<struct sockaddr*>(&m_client_address),
             &m_client_address_len);
  if (m_client_socket == -1) {
    close(m_server_socket);
    throw std::runtime_error("Failed to accept client connection");
  }

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &m_client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
  std::cout << "Client connected: " << client_ip << ":"
            << ntohs(m_client_address.sin_port) << std::endl;
}

server::~server() {
  if (m_client_socket != -1) {
    close(m_client_socket);
    m_client_socket = -1;
  }
  if (m_server_socket != -1) {
    close(m_server_socket);
    m_server_socket = -1;
  }
  std::cout << "Server stopped" << std::endl;
}

ssize_t server::send(const void* data, size_t len) {
  if (m_client_socket == -1) return -1;
  return ::send(m_client_socket, data, len, 0);
}

ssize_t server::recv(void* buf, size_t len) {
  if (m_client_socket == -1) return -1;
  return ::recv(m_client_socket, buf, len, 0);
}

void server::set_non_blocking(bool non_blocking) {
  if (m_client_socket == -1) return;
  int flags = fcntl(m_client_socket, F_GETFL, 0);
  if (flags == -1) return;
  if (non_blocking)
    fcntl(m_client_socket, F_SETFL, flags | O_NONBLOCK);
  else
    fcntl(m_client_socket, F_SETFL, flags & ~O_NONBLOCK);
}

// -------- client --------

client::client() : m_socket(-1) {}

client::~client() {
  close();
}

bool client::connect(const std::string& host, uint16_t port) {
  if (m_socket != -1) {
    ::close(m_socket);
    m_socket = -1;
  }

  m_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (m_socket == -1) return false;

  sockaddr_in server_addr{};
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
    ::close(m_socket);
    m_socket = -1;
    return false;
  }

  if (::connect(m_socket, reinterpret_cast<struct sockaddr*>(&server_addr),
                sizeof(server_addr)) < 0) {
    ::close(m_socket);
    m_socket = -1;
    return false;
  }
  return true;
}

void client::close() {
  if (m_socket != -1) {
    ::close(m_socket);
    m_socket = -1;
  }
}

ssize_t client::send(const void* data, size_t len) {
  if (m_socket == -1) return -1;
  return ::send(m_socket, data, len, 0);
}

ssize_t client::recv(void* buf, size_t len) {
  if (m_socket == -1) return -1;
  return ::recv(m_socket, buf, len, 0);
}

void client::set_non_blocking(bool non_blocking) {
  if (m_socket == -1) return;
  int flags = fcntl(m_socket, F_GETFL, 0);
  if (flags == -1) return;
  if (non_blocking)
    fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
  else
    fcntl(m_socket, F_SETFL, flags & ~O_NONBLOCK);
}
