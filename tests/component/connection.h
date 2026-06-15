#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class server {
public:
  explicit server(std::uint16_t port = 8888);
  virtual ~server();

  std::ptrdiff_t send(const void *data, std::size_t len);
  std::ptrdiff_t recv(void *buf, std::size_t len);

  std::intptr_t client_fd() const;
  void set_non_blocking(bool non_blocking);

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};

class client {
public:
  client();
  ~client();

  bool connect(const std::string &host, std::uint16_t port);
  void close();

  std::ptrdiff_t send(const void *data, std::size_t len);
  std::ptrdiff_t recv(void *buf, std::size_t len);

  bool is_connected() const;
  void set_non_blocking(bool non_blocking);

private:
  struct impl;
  std::unique_ptr<impl> m_impl;
};
