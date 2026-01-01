#include "handler.h"
#include <regex>
namespace mini {
std::filesystem::path FileEngine::path(const std::string &name) const {
  if (!std::regex_match(name, std::regex("[a-z_][a-z_0-9]*"))) throw Failure(ER_PARSE_ERROR);
  auto p = root_ / (name + ".tbl");
  if (std::filesystem::is_symlink(p)) throw Failure(ER_STORAGE_IO);
  return p;
}
void FileEngine::create_table(const std::string &name, const std::string &column) {
  auto p = path(name);
  if (std::filesystem::exists(p)) throw Failure(ER_TABLE_EXISTS);
  std::ofstream f(p);
  f << "MINIDB1 " << column << '\n';
  f.close();
  if (!f) throw Failure(ER_STORAGE_IO);
}
void FileEngine::open(const std::string &name) {
  close(); current_ = path(name);
  if (!std::filesystem::exists(current_)) throw Failure(ER_NO_SUCH_TABLE);
  std::ifstream f(current_);
  std::string line;
  std::getline(f, line);
  std::smatch m;
  if (!f || !std::regex_match(line, m, std::regex("MINIDB1 ([a-z_][a-z_0-9]*)"))) throw Failure(ER_STORAGE_IO);
  column_ = m[1];
}
void FileEngine::write_row(int32_t value) {
  std::ofstream f(current_, std::ios::app);
  f << value << '\n'; f.close();
  if (!f) throw Failure(ER_STORAGE_IO);
}
void FileEngine::rnd_init() {
  scan_.open(current_);
  std::string header; std::getline(scan_, header);
  if (!scan_) throw Failure(ER_STORAGE_IO);
}
bool FileEngine::rnd_next(int32_t &value) {
  std::string line;
  if (!std::getline(scan_, line)) {
    if (scan_.eof()) return false;
    throw Failure(ER_STORAGE_IO);
  }
  try {
    if (!std::regex_match(line, std::regex("-?[0-9]+"))) throw std::runtime_error("format");
    auto n = std::stoll(line);
    if (n < INT32_MIN || n > INT32_MAX) throw std::out_of_range("INT");
    value = static_cast<int32_t>(n);
  } catch (const std::exception &) { throw Failure(ER_STORAGE_IO); }
  return true;
}
void FileEngine::close() { if (scan_.is_open()) scan_.close(); scan_.clear(); current_.clear(); column_.clear(); }
void FileEngine::delete_table(const std::string &name) {
  auto p = path(name);
  if (!std::filesystem::exists(p)) throw Failure(ER_NO_SUCH_TABLE);
  if (!std::filesystem::remove(p)) throw Failure(ER_STORAGE_IO);
}
}
