import fnmatch
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

def glob(directory):
    count = 0
    with os.scandir(directory) as entries:
        for entry in entries:
            if entry.is_dir(follow_symlinks=False):
                count += glob(entry.path)
            elif entry.is_file(follow_symlinks=False) and fnmatch.fnmatchcase(entry.name, "*.cx"):
                count += 1
    return count

def work(kind, root):
    if kind == "glob":
        return glob(root / "tree")
    if kind == "spawn":
        for _ in range(100):
            subprocess.run(["/bin/true"], check=True)
        return 100
    data = (root / ("data.json" if kind == "json" else "data.tsv")).read_bytes()
    if kind == "lines":
        return data.count(b"\n")
    if kind == "split":
        return sum(1 for field in re.split(rb"[\t\n]", data) if field)
    if kind == "json":
        encoded = json.dumps(json.loads(data), separators=(",", ":")).encode()
        if encoded != data:
            raise ValueError("roundtrip")
        return len(encoded)
    if kind != "tsv":
        raise ValueError(kind)
    sums = [0] * 10
    for line in data.split(b"\n"):
        if line:
            key, value = line.split(b"\t")
            sums[int(key[1:])] += int(value)
    return sum((i + 1) * total for i, total in enumerate(sums))

start = time.monotonic_ns()
result = work(sys.argv[1], Path(sys.argv[2]))
end = time.monotonic_ns()
print("RESULT", result, start, end, sep="\t")
