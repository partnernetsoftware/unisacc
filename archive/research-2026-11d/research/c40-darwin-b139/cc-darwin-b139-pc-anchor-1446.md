# cc 主刀：PC 锚 0x10000b2d0 对照 U（1446）

## B 崩溃序列（节选）
```
; caller stores mmap return into local @(fp-0x20), then uses it as pointer
10000b264  bl  0x10000aee8          ; __mmap wrapper
10000b268  ldr x1,[x7]; str x0,[x1] ; *(&local) = ret
…
10000b2a8  mov x2,#0x20; sub x0,x6,x2; ldr x0,[x0]  ; ptr = *(fp-0x20) == -9
10000b2bc  mov x2,#0x10; … ldr x0,[x0]              ; val = *(fp-0x10)
10000b2d0  str x0,[x1]                               ; CRASH
```

## B `__mmap` 门（aee8）
- flags 立即数 **`mov x0,#0x22`**；svc `x16=0x20000c5`（Darwin mmap）
- 失败：`b.lo` / `neg x0` → 返回 −9（EBADF）

## U 同语义 `__mmap`
- flags **`mov x0,#0x1002`**（首见于 ≈0x10001408c）后同形 svc
- 故 U 同输入 rc0

## tape 证据
- `bs_B.tape`：`imm r0, 34` ×129；`imm … 4098` ×0；`.sys6 mmap` 存在
- from-tape 只负责把已烤立即数编进 Mach-O；osx 降的是 syscall 号不是 MAP_* 宏

## 修域请示 → 已代裁
见 `ruling-b139-pc-fix-1446.md`：`tests/bootstrap.sh` 三处 `-t "$HOST_TARGET" -c`。
