# 构造档位预研(部分交付;2026-11-25 ccogweco)

依据:fable backend-density-plan.md、release-build.cx:1274 `rb_whole`、entry-build.cx:373-377 档位逻辑
(六个每次构造都跑的 drive 单元为 -O1;inflate/model decode/executor 保持 -O2,-O0 运行慢约 45%)。
状态:**只完成 (一) 的尺寸读数;(二) 未开工**,原因见第 3 节。全部只读,未起整链。

## 1 (一) 10 分钟证伪:尺寸与符号表

    切片(0.1.0 链 fixedpoint/g2):
    osx-arm64/slice-osx-arm64  4,326,370 B(4.33 MB)
    win-arm64/slice-win-arm64  4,707,840 B(4.71 MB)
    slice-audit --macho osx:
    MACHO_AUDIT bytes=4326370 cputype=0x100000c text=4234764 seg_text=4243456 seg_data=49152
                seg_linkedit=33762 nsyms=0

- **osx/arm64 与 win/arm64 都大于 fable 的 3.6 MB 判据** → 按 fable 规则,**(1) 的中位估计作废**
  (即"whole-o 无 tl_dce"的主因解释不能按中位收益计)。
- osx 片 98% 是 `__TEXT,__text`(4.23 MB),不是数据或链接编辑段;体积集中在代码。
- **去重判定 UNMEASURED**:osx 片 `nsyms=0`(符号表被剥离),无法用符号表数 `malloc` 等函数定义份数;
  `strings` 中 malloc 仅 1 次,只能算弱证据,不足以证明 whole-o 已内部去重。
- win/arm64 是 PE32+,slice-audit 不支持 Mach-O 以外的段审计;仅得文件大小 4.71 MB;
  PE 符号表计数同上 UNMEASURED。

## 2 结论(基于已得数字)

- 仅凭尺寸:两片均 >3.6 MB,fable 的判据(片 >3.6 MB 或单作业 >45 秒 → (1) 中位估计作废)已由尺寸一侧触发;
  (1) 的收益估计不能按中位数承诺。
- "whole-o 是否已内部去重"没有证据:符号剥离使定义份数不可数;要回答需从对象层(单元 .o 或未剥离的中间映像)
  数同符号重复,而不是从最终片。
- (二) 的 -O2 whole-o 私有重建可以绕开符号剥离问题(可在私有副本产出未剥离映像前先看 .o),但本次未做。

## 3 为什么 (二) 未交付

- 上下文余量不足 25%(cc 预设的换人条件);(二) 需要私有副本、改档位、单作业 60 秒上限与 g2/g3 对比,
  不是 10 分钟量级。按 cc 指示先交已得数字并告知,不强行开工以免半成品污染私有树。
- 建议接续者:私有 archive 0.1.0 链树 → 改 `rb_whole` 的档位传参(-O2)→ 只构 osx/arm64 一片 → 量
  .text/墙钟/单作业最长/g2-vs-g3;并在未剥离的中间 .o 上数 malloc 等定义份数回答去重问题。

## 4 边界

- 只读;未改共享检出、未改私有副本、未跑构造。
- osx 片 text 读数为 slice-audit 原行;win 片只有文件大小。
- 不把"尺寸大"写成"密度差已归因";与 backend-density-probe 第二节的序言浪费是同方向证据,但不是同一测量。
