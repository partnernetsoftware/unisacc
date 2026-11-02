# omp-ds 探针记录（2026-11-02，产品 14b906fd…）

复现一律 `cd <仓>; sh ./unisacc.com FILE.cx </dev/null`（必须带 `</dev/null`）。
源码在 `/tmp/std01/`；重做只需照抄下面各段。

## 1. errno 桥接（清单第 11 条）

```c
int main(void) {
    int fd = open("/nonexistent-unisacc-probe", O_RDONLY);
    printf("fd=%d errno=%d\n", fd, errno);
    int fd2 = open("/dev/null", O_RDONLY);
    printf("fd2=%d errno=%d\n", fd2, errno);
    return 0;
}
```

实测 `fd=-1 errno=2` / `fd2=3 errno=2`（成功不改 errno，与 glibc 一致）⇒ 标准运行库路径**是桥接的**。
原始"errno=0"来自 `flock(-1, LOCK_EX)` 探针，而 `flock`/`LOCK_EX` 在 `.cx` 里没有声明
（`error: unknown identifier`），是探针错误。宿主导入符号（`--host-provider`）路径尚未测。

## 2. `--emit-host-symbols`（清单第 20 条）

```sh
printf 'int main(void){return 0;}\n' > ok.cx
sh ./unisacc.com -S --emit-host-symbols ok.sym -o ok.tape ok.cx </dev/null   # rc 0
xxd ok.sym   # 554e 4953 594d 310a 0100 0000  => "UNISYM1\n" + 01 00 00 00
```

文件确实写出 ⇒ "接受但不写旁文件"不成立；12 字节里那个 `1` 的语义未核。

## 3. `UNISA_MAXSTEPS`（清单第 12 条）

```sh
printf 'int main(void){return 0;}\n' > ok.cx
env UNISA_MAXSTEPS=1000 sh ./unisacc.com ok.cx </dev/null   # rc 0：预算被忽略
env UNISA_MAXSTEPS=abc  sh ./unisacc.com ok.cx </dev/null   # rc 0：非法值也被忽略
```

`src/executor.cx:124` 与 `:219` 两处 `core_run(..., 400000000000LL, ...)` 是常量；
`src/executor.cx:238`（`ex_exec` 的 st==3 分支）打印 `step budget exhausted (budget 400000000000 steps)`，
即耗尽路径存在但走不到。`cx/runtime/runexec.cx` 的 `ex_budget` 已经会解析/校验并带
`run: UNISA_MAXSTEPS must be a positive decimal integer <= 9223372036854775807`,
但产品里没有调用者（只在 `archive/` 命中）。

注意：该预算限**模型解释器（编译）步数**，管不到用户原生程序的死循环
（`for(;;) i++;` 用 `UNISA_MAXSTEPS=1000` 仍挂到我 20s 超时）。死循环保护是另一件事。

## 4. stdio 三条写路径（清单第 9 条，修复后）

`tools/check-stdio.cx` 10 项：现装 10/10；`93aa6f0d59^` 的仓外树 `failed=6`
（`/dev/full-all-four`、`puts-eof`、`fflush-eof`、`ferror-set`、`stderr-full-four`、
`file-write-short-and-error`），各自退出位 8 / 8 / 1。

- fd 2 到 `/dev/full`：四位全对（15），与 fd 1 同源 `_u_st_serr[3]`。
- `fopen("/dev/full","w")` 后写 16 KiB：退出位 3 = 短写 + `ferror` 置；
  `fflush` 返回 0、`clearerr` 清位。**同程序用 `cc` + glibc 也是 3**，所以 3 是参考答案。
  小写入（4 字节）不会失败，只是进了 stdio 缓冲——`fputs` 返回 0 属正确行为，不是缺陷。

## 5. 有效 C 被拒（清单第 6 条）

```c
int main(void){ int vla_fs[(int)1.0 + 0]; vla_fs[0]=1; return 0; }
```

`v6.cx:1:34: error: not covered: constant expression`。诊断取自
`exec/parse2/constexpr-manifest.tsv` 的 `reject2=@rej:not covered: constant expression`
（域 `k2-constexpr`）⇒ 属模型事实表，归慢车道，不是快车道条目。
