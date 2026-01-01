"""Small mysql-test-style runner. Each test gets its own server and data directory."""
import argparse
import difflib
import pathlib
import subprocess
import tempfile
import time


class Server:
    def __init__(self, build, root):
        self.build, self.root = build, root
        self.data = root / "data"
        self.socket = root / "s"
        self.process = None
        self.log = (root / "server.log").open("a")

    def start(self):
        self.process = subprocess.Popen(
            [str(self.build / "mysqld"), "--datadir", str(self.data), "--socket", str(self.socket)],
            stdout=self.log, stderr=self.log)
        for _ in range(100):
            if self.process.poll() is not None:
                raise RuntimeError("Server failed: " + (self.root / "server.log").read_text())
            if self.socket.exists():
                return
            time.sleep(0.02)
        raise RuntimeError("Server startup timeout")

    def stop(self):
        if self.process is not None:
            if self.process.poll() is None:
                self.process.terminate()
                try:
                    self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait()
                    raise RuntimeError("Server did not shut down")
            if self.process.returncode != 0:
                raise RuntimeError(f"Unexpected server exit: {self.process.returncode}")
            self.process = None

    def sql(self, text):
        result = subprocess.run([str(self.build / "mysql"), "--socket", str(self.socket), "-e", text],
                                capture_output=True, text=True, timeout=10)
        expected_code = 1 if any(line.startswith("ERROR ") for line in result.stdout.splitlines()) else 0
        if result.returncode != expected_code or result.stderr:
            raise RuntimeError(f"Client failed: {result.returncode} {result.stderr}")
        return result.stdout

    def backup(self, mode, source, destination, busy=False):
        result = subprocess.run([str(self.build / "mysqlbackup"), mode, str(source), str(destination)],
                                capture_output=True, text=True, timeout=10)
        if result.returncode != (1 if busy else 0):
            raise RuntimeError(f"Backup failed: {result.stdout} {result.stderr}")
        return result.stdout + result.stderr


def run_test(build, path, expected):
    with tempfile.TemporaryDirectory(prefix="mdb-", dir="/tmp") as temp:
        root = pathlib.Path(temp)
        server = Server(build, root)
        output = []
        try:
            server.start()
            for raw in path.read_text().splitlines():
                line = raw.strip()
                if not line or line.startswith("#"):
                    continue
                if line == "--restart":
                    server.stop(); server.start()
                elif line == "--stop":
                    server.stop()
                elif line == "--start-restored":
                    server.data = root / "restored"
                    server.start()
                elif line.startswith("--assert-absent "):
                    name = line.split()[1]
                    if (server.data / name).exists():
                        raise AssertionError("Unexpected file: " + name)
                    output.append("OK FILE ABSENT\n")
                elif line == "--backup":
                    output.append(server.backup("backup", server.data, root / "backup"))
                elif line == "--restore":
                    output.append(server.backup("restore", root / "backup", root / "restored"))
                elif line == "--backup-busy":
                    output.append(server.backup("backup", server.data, root / "backup", busy=True))
                    if (root / "backup").exists():
                        raise AssertionError("Rejected backup created a destination")
                elif line.startswith("--"):
                    raise ValueError("Unknown directive: " + line)
                else:
                    output.append(server.sql(line))
            actual = "".join(output)
            wanted = expected.read_text()
            if actual != wanted:
                raise AssertionError("".join(difflib.unified_diff(
                    wanted.splitlines(True), actual.splitlines(True), fromfile="expected", tofile="actual")))
        finally:
            try:
                server.stop()
            finally:
                server.log.close()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=pathlib.Path, required=True)
    parser.add_argument("--build", type=pathlib.Path, required=True)
    parser.add_argument("--suite", choices=("main", "storage", "errors", "meb"), required=True)
    args = parser.parse_args()
    base = args.source / "mysql-test"
    if args.suite == "meb":
        base = args.source / "internal/mysql-test/suite/meb"
    elif args.suite != "main":
        base /= "suite/" + args.suite
    tests = sorted((base / "t").glob("*.test"))
    if len(tests) != 2:
        raise RuntimeError(f"Expected exactly two tests for {args.suite}, found {len(tests)}")
    for test in tests:
        run_test(args.build.resolve(), test, base / "r" / (test.stem + ".result"))
        print(f"PASS {args.suite}.{test.stem}")


if __name__ == "__main__":
    main()
