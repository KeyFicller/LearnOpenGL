#include "connection.h"
#include <iostream>
#include <mutex>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

const int listen_backlog = 5;

#ifdef _WIN32
using sock_fd = SOCKET;
inline constexpr sock_fd sock_fd_invalid = INVALID_SOCKET;
using sock_len_t = int;
#else
using sock_fd = int;
inline constexpr sock_fd sock_fd_invalid = -1;
using sock_len_t = socklen_t;
#endif

#ifdef _WIN32
void ensure_winsock() {
  static std::once_flag once;
  std::call_once(once, [] {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
      throw std::runtime_error("WSAStartup failed");
  });
}
#endif

void sock_close(sock_fd fd) {
  if (fd == sock_fd_invalid)
    return;
#ifdef _WIN32
  closesocket(fd);
#else
  ::close(fd);
#endif
}

std::ptrdiff_t sock_send(sock_fd fd, const void *data, std::size_t len) {
  if (fd == sock_fd_invalid)
    return -1;
#ifdef _WIN32
  int n = ::send(fd, static_cast<const char *>(data), static_cast<int>(len), 0);
#else
  ssize_t n = ::send(fd, data, len, 0);
#endif
  return static_cast<std::ptrdiff_t>(n);
}

std::ptrdiff_t sock_recv(sock_fd fd, void *buf, std::size_t len) {
  if (fd == sock_fd_invalid)
    return -1;
#ifdef _WIN32
  int n = ::recv(fd, static_cast<char *>(buf), static_cast<int>(len), 0);
#else
  ssize_t n = ::recv(fd, buf, len, 0);
#endif
  return static_cast<std::ptrdiff_t>(n);
}

void sock_set_non_blocking(sock_fd fd, bool non_blocking) {
  if (fd == sock_fd_invalid)
    return;
#ifdef _WIN32
  u_long mode = non_blocking ? 1UL : 0UL;
  ioctlsocket(fd, FIONBIO, &mode);
#else
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return;
  if (non_blocking)
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  else
    fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

std::intptr_t sock_to_intptr(sock_fd fd) {
  if (fd == sock_fd_invalid)
    return -1;
#ifdef _WIN32
  return static_cast<std::intptr_t>(static_cast<std::uintptr_t>(fd));
#else
  return static_cast<std::intptr_t>(fd);
#endif
}

}  // namespace

struct server::impl {
  sock_fd server_socket = sock_fd_invalid;
  sock_fd client_socket = sock_fd_invalid;

  explicit impl(std::uint16_t port) {
#ifdef _WIN32
    ensure_winsock();
#endif

    server_socket = static_cast<sock_fd>(socket(AF_INET, SOCK_STREAM, 0));
    if (server_socket == sock_fd_invalid) {
      throw std::runtime_error("Failed to create server socket");
    }

    int opt = 1;
#ifdef _WIN32
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char *>(&opt),
                   static_cast<int>(sizeof(opt))) != 0) {
#else
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt,
                   sizeof(opt)) < 0) {
#endif
      cleanup_listen();
      throw std::runtime_error("Failed to set socket options");
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&server_address),
             sizeof(server_address)) < 0) {
      cleanup_listen();
      throw std::runtime_error("Failed to bind server socket");
    }

    if (listen(server_socket, listen_backlog) < 0) {
      cleanup_listen();
      throw std::runtime_error("Failed to listen on server socket");
    }

    std::cout << "Server listening on port " << port << std::endl;
    std::cout << "Waiting for client connection..." << std::endl;

    sockaddr_in client_address{};
    sock_len_t client_address_len = static_cast<sock_len_t>(sizeof(client_address));
    client_socket = static_cast<sock_fd>(
        accept(server_socket, reinterpret_cast<struct sockaddr *>(&client_address),
               &client_address_len));
    if (client_socket == sock_fd_invalid) {
      cleanup_listen();
      throw std::runtime_error("Failed to accept client connection");
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "Client connected: " << client_ip << ":"
              << ntohs(client_address.sin_port) << std::endl;
  }

  ~impl() {
    if (client_socket != sock_fd_invalid) {
      sock_close(client_socket);
      client_socket = sock_fd_invalid;
    }
    if (server_socket != sock_fd_invalid) {
      sock_close(server_socket);
      server_socket = sock_fd_invalid;
    }
    std::cout << "Server stopped" << std::endl;
  }

  void cleanup_listen() {
    sock_close(server_socket);
    server_socket = sock_fd_invalid;
  }
};

struct client::impl {
  sock_fd m_socket = sock_fd_invalid;

  bool connect_sock(const std::string &host, std::uint16_t port) {
    if (m_socket != sock_fd_invalid) {
      sock_close(m_socket);
      m_socket = sock_fd_invalid;
    }

#ifdef _WIN32
    ensure_winsock();
#endif

    m_socket = static_cast<sock_fd>(socket(AF_INET, SOCK_STREAM, 0));
    if (m_socket == sock_fd_invalid)
      return false;

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr) <= 0) {
      sock_close(m_socket);
      m_socket = sock_fd_invalid;
      return false;
    }

    if (::connect(m_socket, reinterpret_cast<struct sockaddr *>(&server_addr),
                  sizeof(server_addr)) < 0) {
      sock_close(m_socket);
      m_socket = sock_fd_invalid;
      return false;
    }
    return true;
  }

  void close_sock() {
    if (m_socket != sock_fd_invalid) {
      sock_close(m_socket);
      m_socket = sock_fd_invalid;
    }
  }

  bool is_connected() const { return m_socket != sock_fd_invalid; }
};

server::server(std::uint16_t port) : m_impl(std::make_unique<impl>(port)) {}

server::~server() = default;

std::ptrdiff_t server::send(const void *data, std::size_t len) {
  return sock_send(m_impl->client_socket, data, len);
}

std::ptrdiff_t server::recv(void *buf, std::size_t len) {
  return sock_recv(m_impl->client_socket, buf, len);
}

std::intptr_t server::client_fd() const {
  return sock_to_intptr(m_impl->client_socket);
}

void server::set_non_blocking(bool non_blocking) {
  sock_set_non_blocking(m_impl->client_socket, non_blocking);
}

client::client() : m_impl(std::make_unique<impl>()) {}

client::~client() { close(); }

bool client::connect(const std::string &host, std::uint16_t port) {
  return m_impl->connect_sock(host, port);
}

void client::close() { m_impl->close_sock(); }

std::ptrdiff_t client::send(const void *data, std::size_t len) {
  return sock_send(m_impl->m_socket, data, len);
}

std::ptrdiff_t client::recv(void *buf, std::size_t len) {
  return sock_recv(m_impl->m_socket, buf, len);
}

bool client::is_connected() const { return m_impl->is_connected(); }

void client::set_non_blocking(bool non_blocking) {
  sock_set_non_blocking(m_impl->m_socket, non_blocking);
}
