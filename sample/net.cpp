//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/string.h"
#include "core/utils.h"

#include <string.h>

#include <ws2tcpip.h>
#include <winsock2.h>

int main() {
  core_init(M_os_txt("net.log"));
  WORD wVersionRequested;
  WSADATA wsaData;
  wVersionRequested = MAKEWORD(2, 2);
  M_check(WSAStartup(wVersionRequested, &wsaData) == 0);
  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  M_check(socket_fd != INVALID_SOCKET);

  struct sockaddr_in server = {};

  struct addrinfo hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* server_info = {};
  M_check(getaddrinfo("netflix.com", "https", &hints, &server_info) == 0);
  for (struct addrinfo* p = server_info; p != NULL; p = p->ai_next) {
    server = *(struct sockaddr_in*)p->ai_addr;
  }
  freeaddrinfo(server_info);

  M_check(connect(socket_fd, (struct sockaddr*)&server, sizeof(server)) == 0);
  const char* message = "GET / HTTP/1.1\r\n\r\n";
  M_check(send(socket_fd, message, strlen(message), 0) != -1);

  Linear_allocator_t<> allocator("res_allocator");
  allocator.init();
  M_scope_exit(allocator.destroy());
  char res_buf[4096];
  Dynamic_array_t<char> res;
  res.init(&allocator);
  while (int rv = recv(socket_fd, res_buf, 4096, 0)) {
    M_check(rv != -1);
    res.append_array(res_buf, rv);
    Cstring_t res_str(res_buf, rv);
    if (res_str.ends_with("\r\n")) {
      break;
    }
  }
  res.append(0);
  M_logi("%s", res.m_p);
  return 0;
}
