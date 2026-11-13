#!/usr/bin/env bun
// PreToolUse(Bash) 钩子:拒绝用 sed -i / perl -i / awk -i inplace / 重定向 / tee / python open(w)
// 直接改仓内文件的 Bash 命令,提示改用 Edit/Write 或 .cx 工具(GUARD-EDIT-V1)。
// 理由(为何是 bun 脚本而不是 .cx):钩子每个 Bash 调用都跑,需要毫秒级启动;.cx 冷启动 1 秒以上。
// 这是工具链之外的配置脚本,不承载项目算法。fail-open:任何解析错误或不确定一律放行(exit 0)。
// 已知不能杜绝:eval、sh -c "..."(引号内容被剥离)、python/node 脚本文件写入、cd 后的相对路径、变量路径。
import { resolve } from "node:path";

const HOME = process.env.HOME ?? "/home/devali";
const REPO = process.env.GUARD_REPO ?? `${HOME}/repos/unisacc`;
const JOB = process.env.CLAUDE_JOB_DIR ?? "";
const SAFE = ["/tmp/", "/dev/", `${HOME}/cc-work/`, `${REPO}/out/`, ...(JOB ? [JOB + "/"] : [])];

function stripBodies(cmd: string): string {
  // 去 heredoc 正文(保留首行),再去引号内容,避免文本里的 > 与 sed -i 误报
  const lines = cmd.split("\n");
  const out: string[] = [];
  let end: string | null = null;
  for (const l of lines) {
    if (end !== null) { if (l.trim() === end) end = null; continue; }
    out.push(l);
    const m = /<<-?\s*(['"]?)([A-Za-z_][A-Za-z0-9_]*)\1/.exec(l);
    if (m) end = m[2];
  }
  let s = out.join("\n"), r = "";
  for (let i = 0; i < s.length; i++) {
    const c = s[i];
    if (c === "\\") { r += c + (s[i + 1] ?? ""); i++; continue; }
    if (c === "'") { const j = s.indexOf("'", i + 1); if (j < 0) return r; r += "''"; i = j; continue; }
    if (c === '"') {
      let j = i + 1;
      while (j < s.length && s[j] !== '"') j += s[j] === "\\" ? 2 : 1;
      r += '""'; i = j; continue;
    }
    r += c;
  }
  return r;
}

// 返回 true=该路径是仓内受控路径(应拦);false=放行或不确定
function inRepo(p: string, cwd: string, hasCd: boolean): boolean {
  p = p.replace(/^["']|["']$/g, "");
  p = p.replace(/^~(?=\/|$)/, HOME).replace(/^\$\{?HOME\}?/, HOME);
  if (JOB) p = p.replace(/^\$\{?CLAUDE_JOB_DIR\}?/, JOB);
  if (p.includes("$") || p.includes("`") || p === "") return false;
  if (!p.startsWith("/")) {
    if (hasCd || !cwd) return false;
    p = resolve(cwd, p);
  } else p = resolve(p);
  if (SAFE.some((s) => p.startsWith(s))) return false;
  return p === REPO || p.startsWith(REPO + "/");
}

function check(cmd: string, cwd: string): string | null {
  const s = stripBodies(cmd);
  const hasCd = /(^|[;&|(\s])(cd|pushd)\s/.test(s);
  for (const seg of s.split(/;|&&|\|\||\||\n/)) {
    const t = seg.trim();
    const w = t.split(/\s+/);
    // 跳过前置环境变量赋值取首词
    let k = 0;
    while (k < w.length && /^[A-Za-z_][A-Za-z0-9_]*=/.test(w[k])) k++;
    const first = (w[k] ?? "").replace(/^.*\//, "");
    // 项目所有者 2026-11-13:只在 ~/repos/unisacc 里工作;禁止克隆/worktree/外部工作目录/out/ 下建子树
    if (first === "git" && (w[k + 1] === "clone" || (w[k + 1] === "worktree" && w[k + 2] === "add")))
      return "git clone/worktree add:不得另建克隆或工作树,直接在 ~/repos/unisacc 里工作";
    if (first === "mkdir" && w.slice(k + 1).some((a) => /(^|\/)out\/[^\s]+|\/work(\/|$)|^~\/work|cc-work/.test(a)))
      return "mkdir 在 out/ 或 ~/work 下建工作目录:不得另建工作目录";
    if (["git", "gh", "mux"].includes(first)) continue;
    const args = w.slice(k + 1);
    const fileArgs = () => args.filter((a) => !a.startsWith("-") && a !== "");
    if (first === "sed" && args.some((a) => /^-[A-Za-z]*i|^--in-place/.test(a)) ) {
      const f = fileArgs();
      if (f.some((a) => inRepo(a, cwd, hasCd))) return "sed -i 改仓内文件";
    }
    if (first === "perl" && args.some((a) => /^-[A-Za-z]*i/.test(a))) {
      if (fileArgs().some((a) => inRepo(a, cwd, hasCd))) return "perl -i 改仓内文件";
    }
    if ((first === "awk" || first === "gawk") && /-i\s+inplace/.test(args.join(" "))) {
      if (fileArgs().some((a) => inRepo(a, cwd, hasCd))) return "awk -i inplace 改仓内文件";
    }
    if (first === "tee") {
      if (fileArgs().some((a) => inRepo(a, cwd, hasCd))) return "tee 写仓内文件";
    }
    for (const m of t.matchAll(/(?:^|[^<>&0-9])(?:\d?>>?|&>>?)\s*([^\s;&|<>()]+)/g)) {
      if (m[1].startsWith("&")) continue;
      if (inRepo(m[1], cwd, hasCd)) return `重定向写仓内文件 ${m[1]}`;
    }
  }
  // python -c / node -e 内联写文件:看原始命令;仅当出现 open(...,'w'|'a') 且未提及 out/ 或 /tmp 时拦
  if (/\bpython3?\s+(-I\s+)?-c\b/.test(cmd) && /open\([^)]*['"][wa]b?\+?['"]/.test(cmd) && !/\/tmp\/|\bout\/|cc-work|CLAUDE_JOB_DIR/.test(cmd))
    return "python -c open(...,'w') 写文件";
  return null;
}

try {
  const input = JSON.parse(await Bun.stdin.text());
  if (input?.tool_name && input.tool_name !== "Bash") process.exit(0);
  const cmd: string = input?.tool_input?.command ?? "";
  if (!cmd) process.exit(0);
  const why = check(cmd, input?.cwd ?? "");
  if (why) {
    console.error(`GUARD-EDIT-V1 拒绝:${why}。仓内文件只用 Edit/Write 工具或 .cx 工具(tools/edit-span.cx、tools/refactor.cx)修改;临时产物写 $CLAUDE_JOB_DIR/tmp 或 /tmp,不要建克隆或 out/ 子目录。追加账本请用 sh ./unisacc.com tools/ledger-add.cx -- --add ...`);
    process.exit(2);
  }
} catch {
  // fail-open
}
process.exit(0);
