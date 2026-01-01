#pragma once
#include "db.h"
#include <filesystem>
#include <fstream>
namespace mini {
class Handler {
 public:
  virtual ~Handler() = default;
  virtual void create_table(const std::string &, const std::string &) = 0;
  virtual void open(const std::string &) = 0;
  virtual void write_row(int32_t) = 0;
  virtual void rnd_init() = 0;
  virtual bool rnd_next(int32_t &) = 0;
  virtual std::string column() const = 0;
  virtual void close() = 0;
  virtual void delete_table(const std::string &) = 0;
};
class FileEngine final : public Handler {
  std::filesystem::path root_, current_;
  std::string column_;
  std::ifstream scan_;
  std::filesystem::path path(const std::string &) const;
 public:
  explicit FileEngine(std::filesystem::path root) : root_(std::move(root)) {}
  void create_table(const std::string &, const std::string &) override;
  void open(const std::string &) override;
  void write_row(int32_t) override;
  void rnd_init() override;
  bool rnd_next(int32_t &) override;
  std::string column() const override { return column_; }
  void close() override;
  void delete_table(const std::string &) override;
};
}
