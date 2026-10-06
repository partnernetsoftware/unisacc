# unisacc.com -run 速度：2026-10-06 实测

产品：`/Users/wjc/repos/unisacc/unisacc.com`，`unisacc 0.0.30`，2.0MB。本机没有 tcc。对照是 `/usr/bin/clang -O0 -c`。计时是墙钟中位数；csih 那几行是单次。源在 `example/`，复现脚本 `example/time_run.py`。`-v` 在这个产品驱动上直接失败：`unisacc: model driver: option not migrated`，所以没有阶段计数。

## 数字

| 对象 | 秒 | 备注 |
|---|---:|---|
| `--version` | 0.011 | 进程装载不是卡顿 |
| `-run example/empty.c` | 0.046 | 空 main |
| `-E` 只含 `<stdio.h>` | 0.033 | 预处理 44033 字节 |
| `-fno-trim-libc -E` 同一文件 | 0.050 | 68933 字节。裁剪有效，但不是大头 |
| `-run` 调用 `printf` | 0.166 | 预处理 83003 字节；`printf` 把 `_unisa_fd` 正文拉进来 |
| `-c` / `-o` / `-run` 四个头文件 | 0.148 / 0.183 / 0.169 | `<stdio.h> <stdlib.h> <string.h> <unistd.h>`，预处理 127041 字节 |
| `-O0` 与 `-O2` `-run` 四个头文件 | 0.171 / 0.177 | 这个尺寸上优化级别没有差别 |
| 1 / 4 / 8 / 16 个空单元 `-run` | 0.048 / 0.051 / 0.064 / 0.082 | 空文件几乎不随个数涨 |
| 16 个函数写在一个 .c | 0.051 | 比 16 个空单元还快 |
| 8 个单元各含同一组四个头 | 0.445 | |
| 同样的头只出现在一个单元 | 0.163 | 重复头文件贵 0.28 秒 |
| clang `-O0 -c` 四个头文件 | 0.029 | 同一份 `example/headers.c` |
| csih 15 个 .c 各自 `-E` 相加 | 3.838 | 最慢是 `net.c` 0.655 秒、364957 字节 |
| csih 整表 `-o /tmp/ua-csih.bin` | 8.187 | 不运行程序 |
| csih 整表 `-run selftest` | 7.996 | 和 `-o` 同级。卡在编译，不在跑 |
| `UNISA_JOBS=1 / 4 / 8` 再编 csih | 9.820 / 8.174 / 7.591 | 默认已是 4。提到 8 只少 0.6 秒 |

csih 文件表：`csih.c render.c term.c chat.c clock.c tools.c file.c shell.c edit.c gate.c json.c session.c agent.c plugin.c net.c`，目录 `apps/csih`。

## 可以下手的地方

1. 预处理之后的串行段是大头。关掉并行（`UNISA_JOBS=1`）整次 csih `-o` 是 9.82 秒，而 15 个文件串行 `-E` 合计只有 3.84 秒。并行从 1 加到 8 只少 2.2 秒。剩下约 6 秒不随单元并行下降。产品驱动没有阶段计时。`exec/c/compiler.c` 的路线是 `osx/arm64/multi/run/O0`，后面还有 e3、prune、镜像。先给这一段打点，再改最慢的那一站。
2. 同一组头文件被每个 .c 重做一遍。8 个单元 0.445 秒，合成一个单元 0.163 秒。csih 的 `-E` 合计 3.84 秒里大部分是重复头。单元缓存（`ucache`，`UNISA_FWD_UNITCACHE` 不是 `0` 时开着）只在转发重启的第二遍复用，第一遍仍然每个单元读头文件。要的是第一遍的头文件缓存，或者允许一个 `-run` 里多单元共享已经展开的系统头。
3. 转发重启还在。`exec/c/compiler.c` 约 713 行：`fwd_restart` 时带着存根再进一次 `main`。第二遍有单元缓存。需要量一下第二遍是否还把 e3 整表再走一遍。2026-10-05 的 `research/f2-cdsh-profile-20261005.md` 把当时 13 秒里的两遍前端标成第一优先；这次产品已经是约 8 秒，那一刀可能已经吃掉一部分，不要按旧账再假设能省一半。
4. 不要先做这些：`--version` 的 0.011 秒；空程序的 0.046 秒；`-O2`（这次没变快）；把 `UNISA_JOBS` 再加大（8 相对 4 只少 0.6 秒）。`-fno-trim-libc` 会变慢，裁剪留着。

参考编译器 `src/front_parse.c` 的 `inf()` 已是表加载。产品走的是 `exec/c/run.c` 的模型执行器，`-run` 的路线名仍然是模型路线。这次没有证明产品在每个记号上还在做一次完整网络求值。
