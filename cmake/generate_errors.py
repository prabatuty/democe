"""Compile the deliberately small symbol|message catalog format."""
import pathlib
import re
import sys
import json


def generate(source, output):
    symbols, numbers, entries = set(), set(), []
    for filename in ("messages_to_clients.txt", "messages_to_error_log.txt"):
        number = None
        for line in (source / filename).read_text().splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("start-error-number "):
                number = int(line.split()[1])
                continue
            symbol, message = line.split("|", 1)
            if number is None or number in numbers:
                raise ValueError("Missing range or duplicate error number")
            numbers.add(number)
            if symbol != "RESERVED":
                if not re.fullmatch(r"[A-Z][A-Z_0-9]*", symbol) or symbol in symbols:
                    raise ValueError("Invalid or duplicate error symbol: " + symbol)
                symbols.add(symbol)
                entries.append((symbol, number, message))
            number += 1
    lines = ["#pragma once", "namespace mini {", "struct ErrorDef { int code; const char *text; };"]
    lines += [f"inline constexpr ErrorDef {s}{{{n}, {json.dumps(m)}}};" for s, n, m in entries]
    lines.append("}")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n")


if __name__ == "__main__":
    generate(pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2]))
