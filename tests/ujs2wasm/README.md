# ujs2wasm suite

独立门禁：`./tests/ujs2wasm.sh`（`./tests/ujs.sh` 有 node 时会调用；内含 `ujs2wasm_step.sh`）。

| 路径 | 作用 |
|---|---|
| `corpus/*.ujs` | 直出语料（含 setidx / f64） |
| `expect.json` | 期望值 |
| `neg/*.ujs` | 必须失败 |
| `run_wasm.mjs` | Node：`main_export` / tag / i64\|f64\|str |
| `run_step.mjs` | Node：host inject → `run_step` → dict/list/f64 |
| `step_corpus/` · `step_expect.json` | 步进 inject 语料 |
| suite 内 game-ready | `sim.ujs`/`drone.ujs` 必须 `can_emit_direct` |

另：`./tests/ujs2wasm_step.sh` — 路径 B 步进 fold（inject + `run_step` ≡ jtape）。  
`./tests/uxe_ship_js.sh` — ship-js 合同（无 eng_*、无 C `asteroid.wasm`）。

## Direct host ABI（路径 B 步进）

与路径 A `ujs_full.wasm` 同名导出，便于共用绑定：

```
clear_slots / host_reset
host_mk_{null,bool,i64,f64,str,list,dict} · host_list_* · host_dict_* · host_len
host_set_global(i,h) / host_get_global · host_set_local / host_get_local
run_step | host_run   # 不擦注入槽；host_reset 才清堆+槽
tag_of_export · f64_of_export · host_list_get · host_dict_*  # Node 读回
```

emit 返回 `locals` / `globals` / `slots`（globals 序 = `host_set_global` 下标）。`host_scratch` 给 `host_mk_str`。

Asteroid + drone Pages 默认：`opts.directSim = { wasm, meta }` → `uxe/direct_step.js`。

```bash
./tests/ujs2wasm_step.sh
```
