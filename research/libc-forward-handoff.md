# libc 方向裁定：反对自研，系统有的尽量复用（移交 cc-unisacc 安排）

状态：**holder 纪要，不是设计、不是测量。** 未跑任何命令、未改代码。
本文件只记录一件事：主人对 libc 路线的裁定，与请 cc-unisacc 接手安排。
既有的诊断与方案在 [libc-unify-design.md](libc-unify-design.md)（design only），
本纪要与之不重复，只做"来龙去脉 + 裁定 + 移交"三件事。

## 1. 来龙去脉（一句话版）

- 出货 `unisacc.com` 里的随带 C 库不是转发，而是 `include/` 下一份自带的
  静态实现（每个函数 `static`，底层只依赖少数几个裸系统调用门，按
  OS/ISA 由后端发 `syscall`/`svc`）。字节上，它被编进了六个目标的原生
  映像，且按目标近似重复。
- 主人要求参照 tinycc / `tcc -run` 的思路：**系统已有的尽量复用（转发给
  系统 libc），不要自研一份 libc**，以省字节、去重复、少维护。
- `libc-unify-design.md` 早已把这条路线列为 D2（owner's framing /
  thin shim）：Windows 转发 Win32 + 自带 fd 表，macOS 转发 libSystem，
  Linux 保持 syscall，stdio/string/math 本体保留。该文档 status 是
  design only，未动工。

## 2. 主人的裁定

1. 反对自研 libc；系统有的能转发就转发。
2. 具体怎么做由 cc-unisacc 安排，不由本会话拍板。
3. 本会话此前的"来来回回"存在理解不到位，结论以 cc-unisacc 复核为准。

## 3. 移交 cc-unisacc

请 cc-unisacc 安排并复核以下几件事（按既有 D2 门禁口径，逐 family 用
probe 对 reference 逐字节相等才落地，否则保持 named refusal）：

- 平台拆分是否成立：macOS 转发（纯收缩，Mach-O writer 已有
  `LC_LOAD_DYLIB` 与 dl* 四个 eager bind，见 `unisa/image/macho.py` 注释）、
  Windows 转发 Win32 + 自带 fd 表（不能省 fd 表）、Linux 停 syscall、
  stdio/string/math 保留自研的理由是否仍然成立。
- step 2（macOS 文件 I/O + 时间转发 libSystem）是否可落地、改动边界在哪；
  `-run` 的 dlsym+libffi 通路能复用多少。
- 与"按需闭包（`__UNISA_FTRIM_LIBC`）套回编译器自身"、“六份映像去重”是否
  正交、先后顺序。
- 对自举不动点（N1=N2=N3）与六目标逐字节折叠的影响面，逐条在路由表里
  标注"转发/保留/拒绝"。

数字一律以 `research/model-bytes.json` 账本与 `prd.md` §3.14 生成区为准，
本纪要不再复写。

## 4. 由本会话已核实、可供复核的落点（供 cc-unisacc 取用，非结论）

- 每个原生映像里，通用执行核（`exec/c/core.c`）本身很小；`__text` 的大头
  是随带库按 `-O2` 整份编入、且六目标近似重复。字节归因口径见
  `prd.md` §3.14 生成区，不再此处重复。
- Mach-O 产出物带 `LC_LOAD_DYLIB → /usr/lib/libSystem.B.dylib`，当前
  未绑定任何导入符号（raw `svc` 直发）；`dlopen/dlsym/dlclose/dlerror`
  四个符号的 eager bind 机制已在 writer 内，转发通道具备最低限度前提。
- 随带库的 OS 门清单（`__read/__write/__open/__close/__lseek/__ioctl/
  __fstat/__mmap/__munmap/__exit/__getdirentries64|__getdents64` 等）与
  每条 `#if` 平台分支，散落在 `include/*.h`，尚未有一张"转发/保留/拒绝"
  的权威表——这是 D2 落地最先缺的那张表。

<!-- 以上为纪要，不含当前态数字；数字出处见第 3 条末。 -->
