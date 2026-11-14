#!/usr/bin/env bun
// PreToolUse(Bash) 钩子:拒绝用 sed -i / perl -i / awk -i inplace / 重定向 / tee / python open(w)
// 直接改仓内文件的 Bash 命令,提示改用 Edit/Write 或 .cx 工具(GUARD-EDIT-V1)。
// 理由(为何是 bun 脚本而不是 .cx):钩子每个 Bash 调用都跑,需要毫秒级启动;.cx 冷启动 1 秒以上。
// 这是工具链之外的配置脚本,不承载项目算法。fail-open:任何解析错误或不确定一律放行(exit 0)。
// 已知不能杜绝:eval、sh -c "..."(引号内容被剥离)、python/node 脚本文件写入、cd 后的相对路径、变量路径。
import { resolve } from "node:path";
import { readFileSync } from "node:fs";

const HOME = process.env.HOME ?? "/home/devali";
const REPO = process.env.GUARD_REPO ?? `${HOME}/repos/unisacc`;
const JOB = process.env.CLAUDE_JOB_DIR ?? "";
const SAFE = ["/tmp/", "/dev/", `${HOME}/cc-work/`, ...(JOB ? [JOB + "/"] : [])];

// 写域登记(tools/domain.cx):设置 UNISACC_AGENT 且登记表有未过期行时,Edit/Write 目标被他人占用则拒绝。
// 未设置 UNISACC_AGENT 时不做任何事(向后兼容);表缺失或解析失败一律放行。
function domainOwner(path: string, agent: string): string | null {
  const tab = process.env.UNISACC_DOMAINS ?? `${HOME}/.cache/unisacc/domains.tsv`;
  let txt = "";
  try { txt = readFileSync(tab, "utf8"); } catch { return null; }
  let p = resolve(path);
  if (!(p === REPO || p.startsWith(REPO + "/"))) return null;
  p = p.slice(REPO.length + 1);
  const now = Date.now() / 1000;
  let other: string | null = null;
  for (const line of txt.split("\n")) {
    const f = line.split("\t");
    if (f.length < 4 || !(Number(f[2]) > now) || !f[1] || !p.startsWith(f[1])) continue;
    if (f[0] === agent) return null;
    other ??= `${f[0]} (前缀 ${f[1]})`;
  }
  return other;
}

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

// 项目所有者 2026-11-14:仓库根 out/ 本身不得创建或写入。cur=当前目录(cd 追踪;未知为 "")。
function isRepoOut(p: string, cur: string): boolean {
  p = p.replace(/^["']|["']$/g, "");
  p = p.replace(/^~(?=\/|$)/, HOME).replace(/^\$\{?HOME\}?/, HOME);
  if (JOB) p = p.replace(/^\$\{?CLAUDE_JOB_DIR\}?/, JOB);
  if (p.includes("$") || p.includes("`") || p === "") return false;
  if (!p.startsWith("/")) { if (!cur) return false; p = resolve(cur, p); } else p = resolve(p);
  if (SAFE.some((s) => p.startsWith(s))) return false;
  return p === REPO + "/out" || p.startsWith(REPO + "/out/");
}

// git 子命令级违规:共享 main 上不得 stash/clean/reset --hard/checkout --/restore .,不得 --no-verify
function gitViolation(a: string[]): string | null {
  if (a.includes("--autostash")) return "git --autostash 含 stash:共享 main 上不得 stash";
  let i = 0;
  while (i < a.length && a[i].startsWith("-")) i += /^-[Cc]$|^--(git-dir|work-tree|namespace)$/.test(a[i]) ? 2 : 1;
  const sub = a[i] ?? "", rest = a.slice(i + 1);
  if (sub === "stash") return "git stash:共享 main 上不得 stash/pop,需干净输入用 git archive 解到 $CLAUDE_JOB_DIR/tmp";
  if (sub === "clean") return "git clean:共享 main 上不得 clean";
  if (sub === "reset" && rest.includes("--hard")) return "git reset --hard:共享 main 上不得 reset";
  if (sub === "checkout" && (rest.includes("--") || rest.includes("."))) return "git checkout -- / .:不得还原他人路径";
  if (sub === "restore" && rest.includes(".")) return "git restore .:不得还原他人路径";
  if ((sub === "commit" || sub === "push") && rest.includes("--no-verify")) return `git ${sub} --no-verify:不得绕过 precommit`;
  return null;
}

function check(cmd: string, cwd: string, depth = 0): string | null {
  const s = stripBodies(cmd);
  const hasCd = /(^|[;&|(\s])(cd|pushd)\s/.test(s);
  let cur = cwd || REPO;
  for (const seg of s.split(/;|&&|\|\||\||\n|\$\(|`/)) {
    const t = seg.trim();
    const w = t.split(/\s+/);
    // 跳过前置环境变量赋值取首词
    let k = 0;
    while (k < w.length && /^[A-Za-z_][A-Za-z0-9_]*=/.test(w[k])) k++;
    const first = (w[k] ?? "").replace(/^.*\//, "");
    if (first === "cd" || first === "pushd") {
      const d = (w[k + 1] ?? "").replace(/^["']|["']$/g, "").replace(/^~(?=\/|$)/, HOME).replace(/^\$\{?HOME\}?/, HOME);
      cur = d && !d.includes("$") && !d.includes("`") ? (d.startsWith("/") ? resolve(d) : cur ? resolve(cur, d) : "") : "";
      continue;
    }
    {
      const oa = w.slice(k + 1).filter((a) => !a.startsWith("-") && a !== "");
      let hit = "";
      if (first === "mkdir" || first === "touch") hit = oa.find((a) => isRepoOut(a, cur)) ?? "";
      else if (["cp", "mv", "install", "ln", "rsync"].includes(first) && oa.length > 1 && isRepoOut(oa[oa.length - 1], cur)) hit = oa[oa.length - 1];
      if (hit) return `${first} 写仓库根 out/ (${hit}):不得往仓库 out/ 放任何东西,改用 $CLAUDE_JOB_DIR/tmp 或 /tmp`;
    }
    // 项目所有者 2026-11-13:只在 ~/repos/unisacc 里工作;禁止克隆/worktree/外部工作目录/out/ 下建子树
    // 2026-11-14 版本分支试行:目标在 <仓库根>/.claude/worktrees/v0.0.数字 才放行
    const wtOk = (): boolean => {
      const rest = w.slice(k + 3);
      let tgt = "";
      for (let i = 0; i < rest.length; i++) {
        if (/^-[bB]$/.test(rest[i])) { i++; continue; }
        if (rest[i].startsWith("-")) continue;
        tgt = rest[i]; break;
      }
      tgt = tgt.replace(/^["']|["']$/g, "");
      if (!tgt || tgt.includes("$") || tgt.includes("`")) return false;
      const abs = tgt.startsWith("/") ? resolve(tgt) : cur ? resolve(cur, tgt) : "";
      return new RegExp("^" + REPO.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") + "/\\.claude/worktrees/v0\\.0\\.[0-9]+$").test(abs);
    };
    if (first === "git" && (w[k + 1] === "clone" || (w[k + 1] === "worktree" && w[k + 2] === "add" && !wtOk())))
      return "git clone/worktree add:不得另建克隆或工作树,直接在 ~/repos/unisacc 里工作";
    if (first === "mkdir" && w.slice(k + 1).some((a) => /(^|\/)out\/[^\s]+|\/work(\/|$)|^~\/work|cc-work/.test(a)))
      return "mkdir 在 out/ 或 ~/work 下建工作目录:不得另建工作目录";
    if (first === "git") {
      const g = gitViolation(w.slice(k + 1));
      if (g) return g;
    }
    // 项目规则:不新增 Python/Bun/Node 脚本,脚本一律 .cx(宿主钩子 bun .claude/hooks/ 例外)
    if (/^(python[0-9.]*|bun|node|perl|ruby)$/.test(first)) {
      const a0 = w[k + 1] ?? "";
      const hook = first === "bun" && (a0 === '""' || /(^|\/)\.claude\/hooks\//.test(a0) || /^\$\{?CLAUDE_PROJECT_DIR/.test(a0));
      if (!hook) return `${first} 作命令:不得用 python/bun/node/perl/ruby 写脚本,请用 sh ./unisacc.com FILE.cx -- ARGS 写 .cx`;
    }
    if (["git", "gh", "mux"].includes(first)) continue;
    const args = w.slice(k + 1);
    const fileArgs = () => args.filter((a) => !a.startsWith("-") && a !== "");
    if (first === "sed" && args.some((a) => /^-[A-Za-z]*i|^--in-place/.test(a)) ) {
      let f = fileArgs();
      const ef = args.flatMap((a, i) => (/^-[A-Za-z]*[ef]$/.test(a) ? [args[i + 1]] : []));
      if (ef.length) f = f.filter((a) => !ef.includes(a)); else f = f.slice(1); // 脚本不是文件
      if (f.some((a) => inRepo(a, cur, false))) return "sed -i 改仓内文件";
    }
    if (first === "perl" && args.some((a) => /^-[A-Za-z]*i/.test(a))) {
      if (fileArgs().some((a) => inRepo(a, cur, false))) return "perl -i 改仓内文件";
    }
    if ((first === "awk" || first === "gawk") && /-i\s+inplace/.test(args.join(" "))) {
      if (fileArgs().some((a) => inRepo(a, cur, false))) return "awk -i inplace 改仓内文件";
    }
    if (["ex", "ed", "vi", "vim", "nvim"].includes(first) && fileArgs().some((a) => inRepo(a, cur, false))) return `${first} 改仓内文件`;
    if (first === "tee") {
      if (fileArgs().some((a) => inRepo(a, cur, false))) return "tee 写仓内文件";
    }
    for (const m of t.matchAll(/(?:^|[^<>&0-9])(?:\d?>>?|&>>?)\s*([^\s;&|<>()]+)/g)) {
      if (m[1].startsWith("&")) continue;
      if (inRepo(m[1], cur, false) || isRepoOut(m[1], cur)) return `重定向写仓内文件 ${m[1]}`;
    }
  }
  // sh -c '...' / bash -c "...":对引号内正文递归检查一次(深度 1,避免无限递归)
  if (depth < 1) {
    for (const m of cmd.matchAll(/\b(?:ba|z|da)?sh\s+-[a-z]*c\s+(?:'([^']*)'|"((?:[^"\\]|\\.)*)")/g)) {
      const r = check(m[1] ?? m[2] ?? "", cur, depth + 1);
      if (r) return `sh -c 内 ${r}`;
    }
  }
  // python -c / node -e 内联写文件:看原始命令;仅当出现 open(...,'w'|'a') 且未提及 out/ 或 /tmp 时拦
  if (/\bpython3?\s+(-I\s+)?-c\b/.test(cmd) && /open\([^)]*['"][wa]b?\+?['"]/.test(cmd) && !/\/tmp\/|\bout\/|cc-work|CLAUDE_JOB_DIR/.test(cmd))
    return "python -c open(...,'w') 写文件";
  return null;
}

try {
  const input = JSON.parse(await Bun.stdin.text());
  if (input?.tool_name === "Edit" || input?.tool_name === "Write") {
    const agent = process.env.UNISACC_AGENT ?? "";
    const fp: string = input?.tool_input?.file_path ?? "";
    if (agent && fp) {
      const o = domainOwner(fp, agent);
      if (o) {
        console.error(`GUARD-EDIT-V1 拒绝:${fp} 的写域被 ${o} 占用,当前代理 ${agent}。先 sh ./unisacc.com tools/domain.cx -- check PATH 查看,或等对方 release/到期。`);
        process.exit(2);
      }
    }
    process.exit(0);
  }
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
