# Miniature MySQL repository demo

This is newly written educational code, not a build of MySQL Server. The source
models a client/server, handler interface, file engine, error catalogs, and
mysql-test-style suites. C++17, CMake 3.20+, Python 3.9+, and macOS or Linux are required.

## Build and test

```
cmake -S . -B build
cmake --build build -j 4
ctest --test-dir build --output-on-failure
```

## Run

Start the server in one terminal:

```
build/mysqld --datadir /tmp/mini-data --socket /tmp/mini.sock
```

In another terminal:

```
build/mysql --socket /tmp/mini.sock -e 'CREATE TABLE numbers (value INT); INSERT INTO numbers VALUES (10), (20); SELECT * FROM numbers;'
build/mysql --socket /tmp/mini.sock -e 'DROP TABLE numbers;'
```

Use Ctrl-C to stop the server. Both executables accept `--version`.
Input can also be piped into mysql. Errors go to standard output with a nonzero
client exit status; startup/shutdown logs go to server standard error.

## Deliberate boundaries

One signed 32-bit INT column per table; CREATE, INSERT, SELECT *, DROP only.
Identifiers and SQL keywords are case-insensitive. Statements are separated by
semicolons. There are no transactions, crash recovery, indexes, joins, networking
beyond local Unix sockets, or MySQL protocol compatibility. A batch continues after
SQL errors. Multi-row INSERT validates every value before writing, but I/O failures
can leave partial writes. File flush/close is used, not durable transactional fsync.
The server handles one connection at a time. A response is limited to 16 MiB by the
client, and request limits vary by branch through MYSQL_VERSION.
Stop cleanly before reusing a socket path; after a forced crash, remove only the
confirmed stale socket. Use a private directory for database and socket files.

## Source and catalogs

Seven `.cxx` files in src implement client, server, parser, executor, file engine,
protocol, and error formatting. `Handler` isolates storage from SQL execution.
Each `.tbl` file contains `MINIDB1 column` followed by integer rows.

The two catalogs under share use `start-error-number N` followed by `SYMBOL|text`.
They contain 8.0, 8.4, 9.7, and innovation sections on every demo branch so filtering
can be compared consistently. These are demo numbers, not production assignments.
`RESERVED|` occupies a numeric slot without generating a C++ symbol. Catalog
generation rejects duplicate symbols/numbers. Some entries illustrate versioned
ranges and are intentionally not runtime errors.

CTest runs three public suites, each containing two `.test`/`.result` pairs.
The runner is intentionally small and is not compatible with full mysqltest syntax.
Optional source components and their tests are discovered at configure time.
