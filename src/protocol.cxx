#include "db.h"
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
namespace mini {
void send_all(int fd, const std::string &text) {
  size_t done = 0;
  while (done < text.size()) {
    auto n = ::send(fd, text.data() + done, text.size() - done, 0);
    if (n < 0 && errno == EINTR) continue;
    if (n <= 0) throw std::runtime_error("Socket write failed");
    done += static_cast<size_t>(n);
  }
}
std::string read_all(int fd, size_t limit) {
  std::string text; char buffer[1024];
  for (;;) {
    auto n = ::recv(fd, buffer, sizeof(buffer), 0);
    if (n < 0 && errno == EINTR) continue;
    if (n < 0) throw std::runtime_error("Socket read failed");
    if (n == 0) return text;
    text.append(buffer, static_cast<size_t>(n));
    if (text.size() > limit) throw Failure(ER_REQUEST_TOO_LARGE);
  }
}
static int socket_at(const std::string &path, bool server) {
  sockaddr_un address{}; address.sun_family = AF_UNIX;
  if (path.size() >= sizeof(address.sun_path)) throw std::runtime_error("Socket path too long");
  std::memcpy(address.sun_path, path.c_str(), path.size() + 1);
  int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) throw std::runtime_error("Cannot create socket");
  timeval timeout{5, 0};
  setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
  setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  int rc = server ? ::bind(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address))
                  : ::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address));
  if (rc < 0 || (server && ::listen(fd, 8) < 0)) {
    auto message = std::string("Socket error: ") + std::strerror(errno);
    ::close(fd); throw std::runtime_error(message);
  }
  return fd;
}
int connect_socket(const std::string &path) { return socket_at(path, false); }
int listen_socket(const std::string &path) { return socket_at(path, true); }
}
