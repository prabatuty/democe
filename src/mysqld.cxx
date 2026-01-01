#include "handler.h"
#include "config.h"
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>
static volatile sig_atomic_t stopping = 0;
static void stop(int) { stopping = 1; }
int main(int argc, char **argv) {
  std::string data, socket;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--version") { std::cout << "mysqld " MINI_VERSION " " MINI_EDITION "\n"; return 0; }
    if (i + 1 < argc && arg == "--datadir") data = argv[++i];
    else if (i + 1 < argc && arg == "--socket") socket = argv[++i];
    else { std::cerr << "Usage: mysqld --datadir DIR --socket PATH\n"; return 2; }
  }
  if (data.empty() || socket.empty()) { std::cerr << "--datadir and --socket are required\n"; return 2; }
  int lock = -1, listener = -1;
  try {
    std::filesystem::create_directories(data);
    lock = ::open((std::filesystem::path(data) / ".lock").c_str(), O_CREAT | O_RDWR, 0600);
    if (lock < 0 || ::flock(lock, LOCK_EX | LOCK_NB)) throw std::runtime_error("Data directory is busy");
    listener = mini::listen_socket(socket);
    struct sigaction action{}; action.sa_handler = stop; sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, nullptr); sigaction(SIGINT, &action, nullptr);
    std::signal(SIGPIPE, SIG_IGN);
    mini::FileEngine engine(data);
    std::cerr << "LOG " << mini::LOG_READY.code << ": " << mini::LOG_READY.text << '\n';
    while (!stopping) {
      fd_set ready; FD_ZERO(&ready); FD_SET(listener, &ready);
      timeval wait{0, 200000};
      int count = ::select(listener + 1, &ready, nullptr, nullptr, &wait);
      if (count < 0 && errno == EINTR) continue;
      if (count < 0) throw std::runtime_error("Socket select failed");
      if (count == 0) continue;
      int client = ::accept(listener, nullptr, nullptr);
      if (client < 0) continue;
      timeval timeout{2, 0};
      setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
      setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
      try {
        auto request = mini::read_all(client, MINI_MAX_REQUEST);
        auto result = mini::execute(engine, request);
        if (result.find("ERROR " + std::to_string(mini::ER_STORAGE_IO.code)) != std::string::npos)
          std::cerr << "LOG " << mini::LOG_STORAGE_FAILURE.code << ": " << mini::LOG_STORAGE_FAILURE.text << '\n';
        mini::send_all(client, result);
      } catch (const mini::Failure &e) {
        try { mini::send_all(client, mini::error_response(e)); } catch (...) {}
      } catch (const std::exception &e) { std::cerr << e.what() << '\n'; }
      ::close(client);
    }
    std::cerr << "LOG " << mini::LOG_STOPPED.code << ": " << mini::LOG_STOPPED.text << '\n';
    ::close(listener); ::unlink(socket.c_str()); ::close(lock);
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    if (listener >= 0) { ::close(listener); ::unlink(socket.c_str()); }
    if (lock >= 0) ::close(lock);
    return 1;
  }
}
