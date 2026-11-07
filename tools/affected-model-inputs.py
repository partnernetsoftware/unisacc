#!/usr/bin/env python3
"""Project source-package gen read records without running any generator.

Usage: python3 tools/affected-model-inputs.py CONTEXT_OR_TAR [--check]
A relocated context may override its recorded root with --root-prefix PATH.
Only metadata and small read-list members are read; archives are not extracted.
"""
import argparse
from pathlib import Path, PurePosixPath
import re
import sys
import tarfile


LIMIT = 128 * 1024 * 1024
HEADERS = (
    "# Observed source-package gen reads; MODEL\\trepository-relative exec input.\n"
    "# Regenerate: python3 tools/affected-model-inputs.py CONTEXT_OR_TAR [--check]\n"
    "# Optional --root-prefix overrides the root recorded in context.tsv.\n"
    "# Names/count follow cxlib.sourcepackage recipes; inputs are sorted and deduplicated.\n"
)


def fail(message):
    raise ValueError(message)


def recipes(repo):
    source = (repo / "tools/cxlib/sourcepackage.cx").read_text(encoding="utf-8")
    blocks = re.findall(r"static const SpModel recipes\[\]\s*=\s*\{(.*?)\n\};", source, re.S)
    if len(blocks) != 1:
        fail("expected one sourcepackage recipes definition")
    # Match whole entries, not the nested flags initializer. Reject any drift
    # outside this deliberately narrow representation instead of dropping rows.
    entry = re.compile(r'\s*\{\s*"([a-zA-Z0-9_-]+)"\s*,\s*"[a-zA-Z0-9_/-]+"\s*,\s*\{[^{}]*\}\s*\}\s*,?')
    block = blocks[0]
    models = []
    at = 0
    while block[at:].strip():
        match = entry.match(block, at)
        if not match:
            fail("unsupported sourcepackage recipe shape")
        models.append(match.group(1))
        at = match.end()
    if not models or len(set(models)) != len(models):
        fail("empty or duplicate sourcepackage recipes")
    return sorted(models)


def wire(data, label):
    if not data or not data.endswith(b"\n") or b"\0" in data or b"\r" in data:
        fail(f"{label}: empty or malformed LF record")
    return data.decode("utf-8").splitlines()


def absolute(path):
    if not path.startswith("/") or path == "/":
        fail("expected a non-root absolute path")
    if any(part in ("", ".", "..") for part in path[1:].split("/")):
        fail("noncanonical absolute path")
    if any(ord(c) < 32 or ord(c) == 127 for c in path):
        fail("control character in path")
    return path


class Context:
    def __init__(self, source):
        self.source = source
        self.archive = None
        if source.is_dir():
            self.base = source
            self.names = {p.name for p in (source / "jobs").iterdir() if p.name.startswith("gen-")}
        else:
            self.archive = tarfile.open(source, "r:*")
            self.members = {}
            for member in self.archive.getmembers():
                name = member.name
                if name in self.members:
                    fail("duplicate archive member")
                if name.startswith("/") or ".." in PurePosixPath(name).parts:
                    fail("unsafe archive member")
                self.members[name] = member
            candidates = [n for n in self.members if n == "context.tsv" or n.endswith("/context.tsv")]
            if len(candidates) != 1:
                fail("archive must contain exactly one context.tsv")
            self.base = candidates[0][:-len("context.tsv")]
            jobs = self.base + "jobs/"
            self.names = {n[len(jobs):].split("/", 1)[0] for n in self.members if n.startswith(jobs + "gen-")}

    def close(self):
        if self.archive:
            self.archive.close()

    def read(self, relative):
        if self.archive:
            member = self.members.get(self.base + relative)
            if member is None:
                raise FileNotFoundError(relative)
            if not member.isfile() or member.size > LIMIT:
                fail(f"{relative}: not a bounded regular record")
            with self.archive.extractfile(member) as stream:
                return stream.read()
        path = self.base / relative
        if path.is_symlink() or not path.is_file() or path.stat().st_size > LIMIT:
            if not path.exists():
                raise FileNotFoundError(relative)
            fail(f"{relative}: not a bounded regular record")
        return path.read_bytes()


def project(context, models, override):
    metadata = wire(context.read("context.tsv"), "context.tsv")
    roots = [row.split("\t", 1)[1] for row in metadata if row.startswith("root\t")]
    if len(roots) != 1:
        fail("context.tsv must declare exactly one root")
    root = absolute(override if override is not None else roots[0])
    expected = {"gen-" + model for model in models}
    if context.names != expected:
        fail("gen model coverage mismatch: missing=" + ",".join(sorted(expected - context.names))
             + "; unexpected=" + ",".join(sorted(context.names - expected)))
    rows = set()
    for model in models:
        stem = "jobs/gen-" + model + "/"
        try:
            data = context.read(stem + "job-reads.before.tsv")
        except FileNotFoundError:
            data = context.read(stem + "command.reads.tsv")
        inputs = set()
        for path in wire(data, "gen-" + model):
            absolute(path)
            if path != root and not path.startswith(root + "/"):
                fail(f"gen-{model}: read outside root prefix")
            relative = path[len(root):].lstrip("/")
            if relative.startswith("exec/") and len(relative) > 5:
                inputs.add(relative)
        if not inputs:
            fail(f"gen-{model}: no observed exec inputs")
        rows.update((model, path) for path in inputs)
    return (HEADERS + "".join(model + "\t" + path + "\n" for model, path in sorted(rows))).encode("utf-8"), rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("context", type=Path)
    parser.add_argument("--root-prefix")
    parser.add_argument("--check", action="store_true", help="compare without writing")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    context = None
    try:
        models = recipes(repo)
        context = Context(args.context)
        data, rows = project(context, models, args.root_prefix)
        destination = repo / "tools/affected-model-inputs.tsv"
        if args.check:
            if destination.read_bytes() != data:
                print("affected-model-inputs: projection differs", file=sys.stderr)
                return 1
        else:
            destination.write_bytes(data)
        print(f"affected-model-inputs: {len(models)} models, {len(rows)} inputs"
              + (", unchanged" if args.check else ", written"))
        return 0
    except (OSError, ValueError, UnicodeError, tarfile.TarError) as error:
        print(f"affected-model-inputs: {error}", file=sys.stderr)
        return 2
    finally:
        if context is not None:
            context.close()


if __name__ == "__main__":
    sys.exit(main())
