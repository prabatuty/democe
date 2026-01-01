#include "db.h"
#include "config.h"
#include <csignal>
#include <iostream>
#include <iterator>
#include <sys/socket.h>
#include <unistd.h>
int main(int argc, char **argv) {
  std::signal(SIGPIPE, SIG_IGN);
  std::string socket, sql;
  bool expression = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--version") { std::cout << "mysql " MINI_VERSION " " MINI_EDITION "\n"; return 0; }
    if (i + 1 < argc && arg == "--socket") socket = argv[++i];
    else if (i + 1 < argc && arg == "-e") { sql = argv[++i]; expression = true; }
    else { std::cerr << "Usage: mysql --socket PATH [-e SQL]\n"; return 2; }
  }
  if (socket.empty()) { std::cerr << "--socket is required\n"; return 2; }
  if (!expression) sql.assign(std::istreambuf_iterator<char>(std::cin), {});
  int fd = -1;
  try {
    if (sql.size() > MINI_MAX_REQUEST) throw mini::Failure(mini::ER_REQUEST_TOO_LARGE);
    fd = mini::connect_socket(socket);
    mini::send_all(fd, sql); ::shutdown(fd, SHUT_WR);
    auto response = mini::read_all(fd, 16 * 1024 * 1024);
    ::close(fd); fd = -1;
    std::cout << response;
    return response.rfind("ERROR ", 0) == 0 || response.find("\nERROR ") != std::string::npos ? 1 : 0;
  } catch (const std::exception &e) {
    if (fd >= 0) ::close(fd);
    std::cerr << e.what() << '\n'; return 1;
  }
}
