const fs = require("node:fs");
const path = require("node:path");
const {spawnSync} = require("node:child_process");

function glob(dir) {
  let count = 0;
  for (const entry of fs.readdirSync(dir, {withFileTypes: true})) {
    if (entry.isDirectory()) count += glob(path.join(dir, entry.name));
    else if (entry.isFile() && path.matchesGlob(entry.name, "*.cx")) count++;
  }
  return count;
}
function work(kind, root) {
  if (kind === "glob") return glob(path.join(root, "tree"));
  if (kind === "spawn") {
    for (let i = 0; i < 100; i++) {
      const child = spawnSync("/bin/true", []);
      if (child.error || child.status !== 0) throw Error("child failed");
    }
    return 100;
  }
  const data = fs.readFileSync(path.join(root, kind === "json" ? "data.json" : "data.tsv"));
  if (kind === "lines") {
    let n = 0;
    for (const byte of data) if (byte === 10) n++;
    return n;
  }
  const text = data.toString("utf8");
  if (kind === "split") return text.split(/[\t\n]/).filter(Boolean).length;
  if (kind === "json") {
    const encoded = Buffer.from(JSON.stringify(JSON.parse(text)));
    if (!data.equals(encoded)) throw Error("roundtrip");
    return encoded.length;
  }
  if (kind !== "tsv") throw Error("unknown case");
  const sums = Array(10).fill(0);
  for (const line of text.split("\n")) {
    if (!line) continue;
    const [key, value] = line.split("\t");
    sums[Number(key.slice(1))] += Number(value);
  }
  return sums.reduce((sum, n, i) => sum + n * (i + 1), 0);
}
const start = process.hrtime.bigint();
const result = work(process.argv[2], process.argv[3]);
const end = process.hrtime.bigint();
console.log(["RESULT", result, start, end].join("\t"));
