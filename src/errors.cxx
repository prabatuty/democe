#include "db.h"
namespace mini {
std::string error_response(const Failure &f) {
  return "ERROR " + std::to_string(f.code) + ": " + f.what() + "\n";
}
}
