#!/usr/bin/env python3
"""真实Git索引、真实消费者及真实编译器的库存边界；不得在产品工作树中变异。"""
import importlib.util
import pathlib
import subprocess
import sys
import tempfile


def main():
    if len(sys.argv) != 3:
        raise SystemExit("用法: python3 tests/tools-compile-boundaries.py HARNESS_ROOT PRODUCT")
    harness = pathlib.Path(sys.argv[1]).resolve()
    product = pathlib.Path(sys.argv[2]).resolve()
    sys.dont_write_bytecode = True
    spec = importlib.util.spec_from_file_location("registration", harness / "tests/update-tools-compile.py")
    registration = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(registration)
    with tempfile.TemporaryDirectory(prefix="tools-inventory-boundaries-") as tmp:
        root = pathlib.Path(tmp)
        consumer = root / "inventory-consumer"
        built = subprocess.run(["sh", str(product), "-b", "lnx/x86_64", str(harness / "tests/tools-compile.cx"), "-o", str(consumer)],
                               cwd=harness, capture_output=True, timeout=40)
        assert built.returncode == 0 and consumer.is_file(), (built.stdout, built.stderr)
        command = [str(consumer)]
        (root / "tools").mkdir()
        (root / "tests").mkdir()
        (root / "unisacc.com").symlink_to(product)
        # 仅映射productpkg定义的分包成员目录，不引入外部Git配置/忽略文件。
        dev = product.parent / "unisacc.com.dev"
        if dev.is_dir():
            (root / "unisacc.com.dev").symlink_to(dev, target_is_directory=True)
        def git(*args):
            return subprocess.check_output(["git", *args], cwd=root)
        git("init", "-q")
        expected = root / "tests/tools-compile-expected.tsv"
        manifest = root / "tests/tools-compile-sources.tsv"
        batches = root / "tests/batches.tsv"
        expected.write_text("")
        for i in range(17):
            (root / f"tools/source-{i:03d}.cx").write_text(f"int inventory_{i}(void) {{ return {i}; }}\n")
        (root / "tools/alias.cx").symlink_to("source-000.cx")
        (root / "tools/ignored.txt").write_text("不是源\n")
        git("add", "tools")
        sequence = 0
        def call(*args):
            return subprocess.run(command + list(map(str, args)), cwd=harness, capture_output=True, timeout=55)
        def approve():
            manifest.unlink(missing_ok=True)
            r = call("--register", root, manifest)
            assert r.returncode == 0, r.stderr
            size = len(manifest.read_bytes().splitlines())
            batches.write_text("".join(registration.batch(i) for i in range((size + 7) // 8)))
        def shard(index=0):
            nonlocal sequence
            sequence += 1
            out = root / f"result-{sequence}"
            r = call(root, out, index)
            return r, out
        def refuse(fragment):
            r, out = shard()
            assert r.returncode != 0 and fragment in r.stderr, (r.returncode, r.stderr)
            assert not out.exists(), "必须在任何编译和输出目录创建前拒绝"
        approve()
        before = manifest.read_bytes()
        listing = call("--shards", root)
        total = int(listing.stdout)
        assert listing.returncode == 0 and total == (len(before.splitlines()) + 7) // 8
        found = []
        for index in range(total):
            r, out = shard(index)
            assert r.returncode == 0, (r.stdout, r.stderr)
            rows = (out / "results.tsv").read_text().splitlines()
            assert 1 <= len(rows) <= 8
            assert all(row.startswith("PASS\t") for row in rows)
            found.extend(row.split("\t")[1] for row in rows)
            if index == 0:
                assert any("tools/alias.cx\tmode=symlink-follow\t" in row for row in rows)
        assert found == [row.split("\t")[0] for row in before.decode().splitlines()]
        assert len(found) == len(set(found))
        r, out = shard(total)
        assert r.returncode != 0 and b"shard outside inventory" in r.stderr and not out.exists()
        new = root / "tools/new.cx"
        new.write_text("int added(void) { return 0; }\n")
        git("add", "tools/new.cx")
        refuse(b"REFUSE added\ttools/new.cx\tregular")
        approve()
        new_index = [x.split(b"\t")[0] for x in manifest.read_bytes().splitlines()].index(b"tools/new.cx") // 8
        r, out = shard(new_index)
        assert r.returncode == 0 and "PASS\ttools/new.cx\t" in (out / "results.tsv").read_text()
        git("rm", "-q", "-f", "tools/new.cx")
        refuse(b"REFUSE removed\ttools/new.cx\tregular")
        approve()
        git("update-index", "--chmod=+x", "tools/source-000.cx")
        r = call("--shards", root)
        assert r.returncode == 0 and int(r.stdout) == total
        alias = root / "tools/alias.cx"
        alias.unlink()
        alias.write_text("int alias_regular(void) { return 0; }\n")
        git("add", "tools/alias.cx")
        r, out = shard()
        assert r.returncode != 0 and not out.exists()
        assert b"REFUSE added\ttools/alias.cx\tregular" in r.stderr
        assert b"REFUSE removed\ttools/alias.cx\tsymlink" in r.stderr
        approve()
        good = manifest.read_bytes()
        for bad in [good + good.splitlines(keepends=True)[0], b"tools/alias.cx\tinvalid\n", good.rstrip(b"\n")]:
            manifest.write_bytes(bad)
            refuse(b"REFUSE")
        manifest.write_bytes(good)
        good_batches = batches.read_text()
        for bad in ["".join(good_batches.splitlines(keepends=True)[1:]), good_batches + registration.batch(0), good_batches + registration.batch(total)]:
            batches.write_text(bad)
            refuse(b"REFUSE")
        batches.write_text(good_batches)
        expected.write_text("tools/alias.cx\tknown-debt\tinventory-negative\t边界负例\n")
        r, out = shard()
        assert r.returncode != 0 and "XPASS\ttools/alias.cx\t" in (out / "results.tsv").read_text()
        alias.write_text("#error inventory-negative\n")
        r, out = shard()
        assert r.returncode == 0 and "XFAIL\ttools/alias.cx\t" in (out / "results.tsv").read_text()
        expected.write_text("tools/alias.cx\tknown-debt\tdiagnostic-does-not-match\t诊断漂移\n")
        r, out = shard()
        assert r.returncode != 0 and "FAIL\ttools/alias.cx\t" in (out / "results.tsv").read_text()
        expected.write_text("")
        r, out = shard()
        assert r.returncode != 0 and "FAIL\ttools/alias.cx\t" in (out / "results.tsv").read_text()
        expected.write_text("tools/alias.cx\tunknown-category\tx\t类别漂移\n")
        r, out = shard()
        assert r.returncode != 0 and not out.exists()
        expected.write_text("")
        alias.write_text("int restored(void) { return 0; }\n")
        # 超越旧容量而不固定当前产品库存；只查询分片，不编译大量夹具。
        for i in range(8 * 73 + 1):
            (root / f"tools/growth-{i:04d}.cx").write_text("int growth(void) { return 0; }\n")
        git("add", "tools")
        approve()
        grown_listing = call("--shards", root)
        assert grown_listing.returncode == 0, grown_listing.stderr
        grown = int(grown_listing.stdout)
        indexed_sources = [entry.split(b"\t", 1)[1]
                           for entry in git("ls-files", "-s", "-z", "--", "tools").split(b"\0")
                           if entry and entry.split(b"\t", 1)[1].endswith(b".cx")]
        assert grown == (len(indexed_sources) + 7) // 8
        batches.write_text("".join(batches.read_text().splitlines(keepends=True)[:-1]))
        refuse(b"REFUSE missing registered shard")
        blob = git("rev-parse", ":tools/source-000.cx").strip().decode()
        git("update-index", "--add", "--cacheinfo", f"160000,{blob},tools/unsupported.cx")
        refuse(b"unsupported source index mode")
        git("update-index", "--force-remove", "tools/unsupported.cx")
        subprocess.run(["git", "update-index", "--index-info"], cwd=root,
                       input=f"100644 {blob} 1\ttools/unmerged.cx\n".encode(), check=True)
        refuse(b"unmerged source index")
        git("update-index", "--force-remove", "tools/unmerged.cx")
        for name in ["tools/tab\t.cx", "tools/line\n.cx"]:
            (root / name).write_text("int invalid_name(void) { return 0; }\n")
            git("add", name)
            refuse(b"source name cannot be represented in TSV")
            git("rm", "-q", "-f", name)
        print("TOOLS_COMPILE_BOUNDARIES\tPASS")


if __name__ == "__main__":
    main()
