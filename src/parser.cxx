#include "db.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <regex>
namespace mini {
Query parse(const std::string &sql) {
  const auto flags = std::regex::icase;
  const std::string name = "([a-zA-Z_][a-zA-Z_0-9]*)";
  std::smatch m;
  Query q{};
  if (std::regex_match(sql, m, std::regex("\\s*CREATE\\s+TABLE\\s+" + name + "\\s*\\(\\s*" + name + "\\s+INT\\s*\\)\\s*", flags))) {
    q.op = Op::Create; q.table = m[1]; q.column = m[2];
  } else if (std::regex_match(sql, m, std::regex("\\s*SELECT\\s+\\*\\s+FROM\\s+" + name + "\\s*", flags))) {
    q.op = Op::Select; q.table = m[1];
  } else if (std::regex_match(sql, m, std::regex("\\s*DROP\\s+TABLE\\s+" + name + "\\s*", flags))) {
    q.op = Op::Drop; q.table = m[1];
  } else if (std::regex_match(sql, m, std::regex("\\s*INSERT\\s+INTO\\s+" + name + "\\s+VALUES\\s+(.+?)\\s*", flags))) {
    q.op = Op::Insert; q.table = m[1];
    std::string rest = m[2];
    const std::regex item("^\\s*\\(\\s*([+-]?[0-9]+)\\s*\\)\\s*(,|$)");
    while (!rest.empty()) {
      std::smatch v;
      if (!std::regex_search(rest, v, item)) throw Failure(ER_INVALID_INT);
      try {
        auto n = std::stoll(v[1].str());
        if (n < INT32_MIN || n > INT32_MAX) throw std::out_of_range("INT");
        q.values.push_back(static_cast<int32_t>(n));
      } catch (const std::exception &) { throw Failure(ER_INVALID_INT); }
      const bool comma = v[2] == ",";
      rest = v.suffix();
      if (comma && rest.find_first_not_of(" \t\r\n") == std::string::npos) throw Failure(ER_INVALID_INT);
    }
  } else throw Failure(ER_PARSE_ERROR);
  if (q.table.size() > 64 || q.column.size() > 64) throw Failure(ER_PARSE_ERROR);
  for (auto *s : {&q.table, &q.column})
    std::transform(s->begin(), s->end(), s->begin(), [](unsigned char c) { return std::tolower(c); });
  return q;
}
}
