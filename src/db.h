#pragma once
#include "error_codes.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
namespace mini {
struct Failure : std::runtime_error {
  int code;
  explicit Failure(ErrorDef error) : std::runtime_error(error.text), code(error.code) {}
};
enum class Op { Create, Insert, Select, Drop };
struct Query { Op op; std::string table; std::string column; std::vector<int32_t> values; };
Query parse(const std::string &sql);
class Handler;
std::string execute(Handler &handler, const std::string &sql);
std::string error_response(const Failure &failure);
void send_all(int fd, const std::string &text);
std::string read_all(int fd, size_t limit);
int connect_socket(const std::string &path);
int listen_socket(const std::string &path);
}
