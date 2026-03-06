#pragma once

#include <arpa/inet.h>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

class server {
public:
  explicit server(uint16_t port = 8888);
  virtual ~server();

  ssize_t send(const void *data, size_t len);
  ssize_t recv(void *buf, size_t len);

  int client_fd() const { return m_client_socket; }
  void set_non_blocking(bool non_blocking);

protected:
  int m_server_socket;
  int m_client_socket;
  sockaddr_in m_server_address;
  sockaddr_in m_client_address;
  socklen_t m_client_address_len;
};

class client {
public:
  client();
  ~client();

  bool connect(const std::string &host, uint16_t port);
  void close();

  ssize_t send(const void *data, size_t len);
  ssize_t recv(void *buf, size_t len);

  bool is_connected() const { return m_socket != -1; }
  void set_non_blocking(bool non_blocking);

private:
  int m_socket;
};
