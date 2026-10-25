# UJS 测试套件（产品闸）

> 真源规格：[`ujs/prd.md`](../../ujs/prd.md) §0.1  
> 纪律：只用根 **`unisacc.com` + `.cx`（`-run`）**；不恢复已归档的 Python／shell 出货闸。

## 入口

```bash
# 仓库根
./unisacc.com -run tests/ujs/run.cx -- --list
./unisacc.com -run tests/ujs/run.cx -- --smoke      # 本机 build + 语料 28/0
./unisacc.com -run tests/ujs/run.cx -- --complete   # APE + 语料 + 六目标
```

| 模式 | 调用 | 证明什么 |
|---|---|---|
| `--smoke` | `ujs/product/build.cx` + `corpus-host.cx` | host1 能编、语料合同绿 |
| `--complete` | `ujs/product/complete.cx` | APE 形 + 28/0 + 六目标原生 |
| `--list` | （清单） | 闸／夹具／遗留／另门 |

产品实现仍在 [`ujs/product/`](../../ujs/product/)；本目录只做**套件入口**，不复制编译器。

## 夹具

| 路径 | 角色 |
|---|---|
| [`../ujs2wasm/corpus/`](../ujs2wasm/corpus/) | host1 PASS 语料（`corpus-host.cx` 合同） |
| [`../ujs2wasm/neg/`](../ujs2wasm/neg/) | 构造／全表拒例（非 host1 必经） |
| [`../ujs2wasm/`](../ujs2wasm/) 其余 | 历史 Node／step 探针；**不是** `ujs.com` 出货闸 |

## 刻意不在本套件

| 项 | 去哪 |
|---|---|
| `tests/run.cx` | unisacc CX MVP，与 `ujs.com` 无关 |
| `archive/testing-c43-legacy/tests/ujs*.sh` | 旧 Python／Node 语言闸；只作考古 |
| UXE／Pages | `npm run test:uxe:*` · `uxe_ship_js`（实践脊另门） |
| M3 `compiler_core` 自举 | 实践脊；未并入 host1 `complete.cx` |

不 tip／Draft／签／公开，除非另授。
