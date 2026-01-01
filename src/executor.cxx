#include "handler.h"
#include <sstream>
namespace mini {
std::string execute(Handler &h, const std::string &sql) {
  std::ostringstream result;
  std::istringstream input(sql);
  std::string statement;
  while (std::getline(input, statement, ';')) {
    if (statement.find_first_not_of(" \n\r\t") == std::string::npos) continue;
    try {
      auto q = parse(statement);
      switch (q.op) {
        case Op::Create: h.create_table(q.table, q.column); result << "OK CREATE TABLE\n"; break;
        case Op::Drop: h.delete_table(q.table); result << "OK DROP TABLE\n"; break;
        case Op::Insert:
          h.open(q.table);
          for (auto value : q.values) h.write_row(value);
          result << "OK INSERT " << q.values.size() << '\n'; break;
        case Op::Select: {
          h.open(q.table); h.rnd_init();
          std::ostringstream rows; rows << h.column() << '\n';
          int32_t value; size_t count = 0;
          while (h.rnd_next(value)) { rows << value << '\n'; ++count; }
          result << rows.str() << "ROWS " << count << '\n'; break;
        }
      }
    } catch (const Failure &f) { result << error_response(f); }
      catch (const std::exception &) { result << error_response(Failure(ER_STORAGE_IO)); }
    h.close();
  }
  return result.str();
}
}
