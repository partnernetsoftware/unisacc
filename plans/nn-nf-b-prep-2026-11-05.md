# NN-NF-B 开工依据: e2 网里关键字逐字比对的 SBOUT 长序列(2026-11-05, HEAD 实测)

口径: 只读 HEAD 的 exec/pp/*.tsv, 用 `gsub(/\["SBOUT",[0-9]+\]/)` 数每行(每状态-观察-目标行)内
连续 SBOUT 动作数, 再按动作串字节(`match` 累加 `["SBOUT",NN]` 长度)算字节。不读任何构建产物。

## 总量

| 阈值 | 段数 | SBOUT 动作 | 动作串字节 |
|---|---|---|---|
| ≥2 | 22 | 230 | 2887 |
| ≥8 | 15 | 199 | 2511 |

全部 e2 网 SBOUT 动作合计 **448**(65 个 exec/pp/*.tsv 文件)。

## 长段明细(≥8, 按字节降序)

| 文件 | 状态 | SBOUT | 字节 |
|---|---|---|---|
| sourcefacts-byte.tsv | SF.start | 20 | 252 |
| ftrim-libc-byte.tsv | CLI.LN | 18 | 226 |
| ftrim-libc-byte.tsv | QIF | 16 | 202 |
| directive-body-byte.tsv | CLI.IP | 16 | 202 |
| cli-byte.tsv | CLI.U | 14 | 178 |
| directive-body-byte.tsv | CLI.QP | 14 | 177 |
| cli-byte.tsv | CLI.INC | 13 | 164 |
| cli-byte.tsv | CLI.FLAGS | 13 | 164 |
| shared-predefine-byte.tsv | PD.key | 12 | 153 |
| cli-byte.tsv | CLI.D | 12 | 152 |
| shared-predefine-byte.tsv | P3PD0 | 11 | 138 |
| ftrim-libc-byte.tsv | LQ0 | 11 | 138 |
| object-predefine-byte.tsv | OOBJ.start | 11 | 137 |
| object-predefine-byte.tsv | OUNIT.start | 10 | 126 |
| directive-body-result.tsv | INC.BUILTIN.c | 8 | 102 |
| **合计** | | **199** | **2511** |

## 形态

每个长段都是 `SBCLR` → N 个 `SBOUT`(逐字节写一个 ASCII 关键字/资源键) → `SBFIND` → `CMPI`。
例(sourcefacts-byte.tsv SF.start, 20 个 SBOUT):

```
["LDI","sf_enabled",0],["LDI","sf_clean",1],["SBCLR"],
["SBOUT",0],["SBOUT",108],["SBOUT",105],["SBOUT",98],... ["SBOUT",115],
["SBFIND","sf_resource"],["CMPI","sf_resource",0]
```

写出的字符串依次是 `\0library/sourcefacts`(SF.start)、`\0cli/includes`(CLI.INC)、
`\0cli/flags`→`\0cli/nostdinc`(CLI.FLAGS)等 —— 即 S+SBFIND 的目标形态:
`["SBCLR"],["@bytes","<同一串>"],["SBFIND",reg]` 或一次 `SBLOAD` 直接给 blob id。

## 收益上限

按动作串字节算, 15 个长段合计 **2511B**; 若压成 `@bytes` 一条(SBLOAD), 每段约 20-30B,
净省约 **2.1KB** 的网 JSON。相对 NN-NF 全量 14.1MB 是 0.015% —— 收益极小,
**这是正确性/同构收益(消除逐字节手写), 不是体积收益**。开工时应按此预期排优先级。

## 复算命令

```
awk -F'\t' '{s=$0;n=0;b=0;while(match(s,/\["SBOUT",[0-9]+\]/)){b+=RLENGTH;s=substr(s,RSTART+RLENGTH);n++}
  if(n>=8) printf "%s\t%s\t%d\t%d\n",FILENAME,$1,n,b}' exec/pp/*.tsv | sort -k4 -rn
```

---

## 裁定 c 的结论(2026-11-05, cc/督导)

15 段**全部**位于表单元格(`exec/pp/*-byte.tsv` / `*-result.tsv`), 无一段在 manifest 的 mapseq acts:

| 文件 | 段数 |
|---|---|
| cli-byte.tsv | 4 |
| ftrim-libc-byte.tsv | 3 |
| directive-body-byte.tsv | 2 |
| object-predefine-byte.tsv | 2 |
| shared-predefine-byte.tsv | 2 |
| sourcefacts-byte.tsv | 1 |
| directive-body-result.tsv | 1 |

`grep -c SBOUT exec/pp/*-manifest.tsv` 全部为 0 —— manifest 里没有 SBOUT 动作, 所以没有
"有 mapseq 入口"的段可改。

根因(seed/gen.c 两处不同展开器):
- `seed/gen.c:744`  mapseq 的 `["@bytes","…"]` 会逐字节展开成 SBOUT/OUT;
- `seed/gen.c:199`  `expanded_action()`(表单元格动作序列走这里)只认 `["constant",…]` 与
  `["observation",…]`, 其它原样输出 —— 表单元格里的 `@bytes` 不展开, 直接成 JSON 字面量。

实测证据: 把 SF.start 的 20 个 SBOUT 换成 `["@bytes","\u0000library/sourcefacts"]` 后,
`seed-gen pp` 产出的网 JSON 该处是 `["@bytes",""]`(空串), 网 sha 从 `ee790ce0ae589919`
变 `bd85f6d2fd17d063`, 即判据不成立。表已全部还原(`git checkout exec/pp/`), 基线可复现。

**本版 NN-NF-B 以"实测 + 缺口登记"收口**: 需 `expanded_action` 支持 `@bytes` 才能改,
那是 seed/gen.c 改动 → 构造链身份全变、要全链重建, 而总收益仅约 2.5KB, 不在 0.0.44 前做。
缺口已立 items: GEN-CELL-BYTES (0.0.45)。
