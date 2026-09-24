"""Direct UJS → real Wasm (\\0asm), no C VM. [LW-3]

Heap-handle executor (tags aligned with ``native/ujs_vm.c``) as WAT → wat2wasm.
Every jtape op must have a non-trap ``isel`` form; gaps are catalog/codegen
bugs to fix, not silent C-VM bypasses.
"""
from __future__ import annotations

import os
import subprocess
import tempfile

from .jtape import Fn, Ins
from .oracle import Oracle

TAG_NULL, TAG_BOOL, TAG_I64, TAG_F64, TAG_STR = 0, 1, 2, 3, 4
TAG_LIST, TAG_DICT, TAG_FN, TAG_TUP = 5, 6, 7, 8
# Heap must fit game sims (hundreds of f64 list cells per step). Binders use
# scratch at HOST_SCRATCH for host_mk_str (same idea as wasm_run.js).
HEAP0, STACK0, LOCALS0, SAVE0 = 65536, 524288, 589824, 590080
GMAP0, LMAP0, STRDATA0 = 590800, 591824, 593000
HOST_SCRATCH = 950000
MEM_PAGES = 16
MEM_BYTES = MEM_PAGES * 65536


class DirectEmitError(Exception):
    pass


def _check_isel(fn: Fn, oracle: Oracle) -> None:
    for ins in fn.code:
        form = oracle.ask("isel", (ins.op,))
        if form == "trap":
            raise DirectEmitError("isel trap for op %s — extend catalog" % ins.op)
        imm = "none"
        if ins.op == "const":
            imm = {"i64": "i64", "f64": "f64", "bool": "bool", "str": "str_ix"}.get(
                ins.a, "none")
        elif ins.op in ("load_l", "store_l", "load_g", "store_g"):
            imm = "slot"
        elif ins.op in ("jump", "jumpz", "jumpnz"):
            imm = "label"
        elif ins.op in ("mklist", "mkdict", "mktup", "call"):
            imm = "count"
        elif ins.op == "dot":
            imm = "str_ix"
        if oracle.ask("enc", (form, imm)) == "trap":
            raise DirectEmitError(
                "enc trap form=%s imm=%s op=%s — extend catalog" % (form, imm, ins.op))


def can_emit_direct(fn: Fn, oracle: Oracle | None = None) -> tuple[bool, str]:
    o = oracle or Oracle(drive="gold")
    try:
        _check_isel(fn, o)
        for nested in (fn.meta.get("fns") or {}).values():
            _check_isel(nested, o)
    except DirectEmitError as e:
        return False, str(e)
    unsupported = ("print", "spread", "restpack", "keys", "in",
                   "shapeof", "switch")
    for ins in fn.code:
        if ins.op in unsupported:
            return False, "emit gap " + ins.op
    for nested in (fn.meta.get("fns") or {}).values():
        for ins in nested.code:
            if ins.op in unsupported:
                return False, "emit gap " + ins.op
    return True, "ok"


def _slot_names(fn: Fn) -> list[str]:
    names = list(fn.localslot)
    for ins in fn.code:
        if ins.op in ("load_l", "store_l", "load_g", "store_g") and isinstance(ins.a, str):
            if ins.a not in names:
                names.append(ins.a)
    return names


def _gl_names(fn: Fn) -> tuple[list[str], list[str]]:
    """Local / global name tables matching ``bc_encode.encode_fn`` order."""
    slots = {n: i for i, n in enumerate(fn.localslot)}
    for ins in fn.code:
        if ins.op in ("load_l", "store_l") and isinstance(ins.a, str) and ins.a not in slots:
            slots[ins.a] = len(slots)
    inv = [None] * len(slots)
    for n, i in slots.items():
        inv[i] = n
    local_names = list(inv)
    globals_used: list[str] = []
    g_ix: dict[str, int] = {}
    for ins in fn.code:
        if ins.op in ("load_g", "store_g") and isinstance(ins.a, str) and ins.a not in g_ix:
            g_ix[ins.a] = len(globals_used)
            globals_used.append(ins.a)
    return local_names, globals_used


def _esc(bs: bytes) -> str:
    return "".join("\\%02x" % b for b in bs)


def _helpers() -> str:
    return f"""
  (memory (export "memory") {MEM_PAGES})
  (global $freep (mut i32) (i32.const {HEAP0}))
  (global $sp (mut i32) (i32.const {STACK0}))
  (global $call_argc (mut i32) (i32.const 0))
  (func $alloc (param $n i32) (result i32)
    (local $p i32)
    (local.set $p (global.get $freep))
    (global.set $freep
      (i32.and (i32.add (global.get $freep) (i32.add (local.get $n) (i32.const 7)))
               (i32.const -8)))
    (local.get $p))
  (func $push (param $h i32)
    (i32.store (global.get $sp) (local.get $h))
    (global.set $sp (i32.add (global.get $sp) (i32.const 4))))
  (func $pop (result i32)
    (global.set $sp (i32.sub (global.get $sp) (i32.const 4)))
    (i32.load (global.get $sp)))
  (func $tag_of (param $h i32) (result i32)
    (if (result i32) (i32.eqz (local.get $h))
      (then (i32.const {TAG_NULL}))
      (else (i32.load8_u (local.get $h)))))
  (func $len_of (param $h i32) (result i32)
    (if (result i32) (i32.eqz (local.get $h))
      (then (i32.const 0))
      (else (i32.load16_u (i32.add (local.get $h) (i32.const 2))))))
  (func $i64_of (param $h i32) (result i64)
    (i64.load (i32.add (local.get $h) (i32.const 8))))
  (func $mk_i64 (param $v i64) (result i32)
    (local $p i32)
    (local.set $p (call $alloc (i32.const 16)))
    (i32.store8 (local.get $p) (i32.const {TAG_I64}))
    (i64.store (i32.add (local.get $p) (i32.const 8)) (local.get $v))
    (local.get $p))
  (func $mk_f64 (param $v f64) (result i32)
    (local $p i32)
    (local.set $p (call $alloc (i32.const 16)))
    (i32.store8 (local.get $p) (i32.const {TAG_F64}))
    (f64.store (i32.add (local.get $p) (i32.const 8)) (local.get $v))
    (local.get $p))
  (func $f64_of (param $h i32) (result f64)
    (f64.load (i32.add (local.get $h) (i32.const 8))))
  (func $is_num (param $h i32) (result i32)
    (local $t i32)
    (local.set $t (call $tag_of (local.get $h)))
    (i32.or (i32.eq (local.get $t) (i32.const {TAG_I64}))
            (i32.eq (local.get $t) (i32.const {TAG_F64}))))
  (func $as_f64 (param $h i32) (result f64)
    (if (result f64) (i32.eq (call $tag_of (local.get $h)) (i32.const {TAG_F64}))
      (then (call $f64_of (local.get $h)))
      (else (f64.convert_i64_s (call $i64_of (local.get $h))))))
  (func $mk_bool (param $b i32) (result i32)
    (local $p i32)
    (local.set $p (call $alloc (i32.const 8)))
    (i32.store8 (local.get $p) (i32.const {TAG_BOOL}))
    (i32.store8 (i32.add (local.get $p) (i32.const 4)) (local.get $b))
    (local.get $p))
  (func $mk_str (param $src i32) (param $n i32) (result i32)
    (local $p i32) (local $i i32)
    (local.set $p (call $alloc (i32.add (i32.const 8) (local.get $n))))
    (i32.store8 (local.get $p) (i32.const {TAG_STR}))
    (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $n))
    (local.set $i (i32.const 0))
    (block $done (loop $L
      (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
      (i32.store8
        (i32.add (i32.add (local.get $p) (i32.const 8)) (local.get $i))
        (i32.load8_u (i32.add (local.get $src) (local.get $i))))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br $L)))
    (local.get $p))
  (func $mk_fn (param $ix i32) (result i32)
    (local $p i32)
    (local.set $p (call $alloc (i32.const 8)))
    (i32.store8 (local.get $p) (i32.const {TAG_FN}))
    (i32.store (i32.add (local.get $p) (i32.const 4)) (local.get $ix))
    (local.get $p))
  (func $truthy (param $h i32) (result i32)
    (local $t i32)
    (local.set $t (call $tag_of (local.get $h)))
    (if (result i32) (i32.eq (local.get $t) (i32.const {TAG_NULL}))
      (then (i32.const 0))
      (else (if (result i32) (i32.eq (local.get $t) (i32.const {TAG_BOOL}))
        (then (i32.load8_u (i32.add (local.get $h) (i32.const 4))))
        (else (if (result i32) (i32.eq (local.get $t) (i32.const {TAG_I64}))
          (then (i64.ne (call $i64_of (local.get $h)) (i64.const 0)))
          (else (if (result i32) (i32.eq (local.get $t) (i32.const {TAG_F64}))
            (then (f64.ne (call $f64_of (local.get $h)) (f64.const 0)))
            (else (i32.ne (call $len_of (local.get $h)) (i32.const 0)))))))))))
  (func $str_eq (param $a i32) (param $b i32) (result i32)
    (local $n i32) (local $i i32)
    (local.set $n (call $len_of (local.get $a)))
    (if (i32.ne (local.get $n) (call $len_of (local.get $b)))
      (then (return (i32.const 0))))
    (local.set $i (i32.const 0))
    (block $done (loop $L
      (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
      (if (i32.ne
            (i32.load8_u (i32.add (i32.add (local.get $a) (i32.const 8)) (local.get $i)))
            (i32.load8_u (i32.add (i32.add (local.get $b) (i32.const 8)) (local.get $i))))
        (then (return (i32.const 0))))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br $L)))
    (i32.const 1))
  (func $op_add (param $a i32) (param $b i32) (result i32)
    (local $na i32) (local $nb i32) (local $q i32) (local $i i32)
    (if (result i32)
        (i32.and (i32.eq (call $tag_of (local.get $a)) (i32.const {TAG_STR}))
                 (i32.eq (call $tag_of (local.get $b)) (i32.const {TAG_STR})))
      (then
        (local.set $na (call $len_of (local.get $a)))
        (local.set $nb (call $len_of (local.get $b)))
        (local.set $q (call $alloc (i32.add (i32.const 8) (i32.add (local.get $na) (local.get $nb)))))
        (i32.store8 (local.get $q) (i32.const {TAG_STR}))
        (i32.store16 (i32.add (local.get $q) (i32.const 2)) (i32.add (local.get $na) (local.get $nb)))
        (local.set $i (i32.const 0))
        (block $c1 (loop $L1
          (br_if $c1 (i32.ge_u (local.get $i) (local.get $na)))
          (i32.store8 (i32.add (i32.add (local.get $q) (i32.const 8)) (local.get $i))
                      (i32.load8_u (i32.add (i32.add (local.get $a) (i32.const 8)) (local.get $i))))
          (local.set $i (i32.add (local.get $i) (i32.const 1)))
          (br $L1)))
        (local.set $i (i32.const 0))
        (block $c2 (loop $L2
          (br_if $c2 (i32.ge_u (local.get $i) (local.get $nb)))
          (i32.store8 (i32.add (i32.add (i32.add (local.get $q) (i32.const 8)) (local.get $na)) (local.get $i))
                      (i32.load8_u (i32.add (i32.add (local.get $b) (i32.const 8)) (local.get $i))))
          (local.set $i (i32.add (local.get $i) (i32.const 1)))
          (br $L2)))
        (local.get $q))
      (else (if (result i32)
          (i32.and (i32.eq (call $tag_of (local.get $a)) (i32.const {TAG_I64}))
                   (i32.eq (call $tag_of (local.get $b)) (i32.const {TAG_I64})))
        (then (call $mk_i64 (i64.add (call $i64_of (local.get $a)) (call $i64_of (local.get $b)))))
        (else (call $mk_f64 (f64.add (call $as_f64 (local.get $a)) (call $as_f64 (local.get $b)))))))))
  (func $op_bin (param $op i32) (param $a i32) (param $b i32) (result i32)
    (local $va i64) (local $vb i64) (local $r i64) (local $fa f64) (local $fb f64) (local $fr f64)
    (if (result i32)
        (i32.and (i32.eq (call $tag_of (local.get $a)) (i32.const {TAG_I64}))
                 (i32.eq (call $tag_of (local.get $b)) (i32.const {TAG_I64})))
      (then
        (local.set $va (call $i64_of (local.get $a)))
        (local.set $vb (call $i64_of (local.get $b)))
        (local.set $r (i64.const 0))
        (if (i32.eq (local.get $op) (i32.const 0))
          (then (local.set $r (i64.sub (local.get $va) (local.get $vb)))))
        (if (i32.eq (local.get $op) (i32.const 1))
          (then (local.set $r (i64.mul (local.get $va) (local.get $vb)))))
        (if (i32.eq (local.get $op) (i32.const 2))
          (then (local.set $r (i64.div_s (local.get $va) (local.get $vb)))))
        (if (i32.eq (local.get $op) (i32.const 3))
          (then (local.set $r (i64.rem_s (local.get $va) (local.get $vb)))))
        (call $mk_i64 (local.get $r)))
      (else
        (local.set $fa (call $as_f64 (local.get $a)))
        (local.set $fb (call $as_f64 (local.get $b)))
        (local.set $fr (f64.const 0))
        (if (i32.eq (local.get $op) (i32.const 0))
          (then (local.set $fr (f64.sub (local.get $fa) (local.get $fb)))))
        (if (i32.eq (local.get $op) (i32.const 1))
          (then (local.set $fr (f64.mul (local.get $fa) (local.get $fb)))))
        (if (i32.eq (local.get $op) (i32.const 2))
          (then (local.set $fr (f64.div (local.get $fa) (local.get $fb)))))
        (if (i32.eq (local.get $op) (i32.const 3))
          (then (local.set $fr (f64.sub (local.get $fa)
            (f64.mul (local.get $fb) (f64.trunc (f64.div (local.get $fa) (local.get $fb))))))))
        (call $mk_f64 (local.get $fr)))))
  (func $op_cmp (param $op i32) (param $a i32) (param $b i32) (result i32)
    (local $c i32) (local $va i64) (local $vb i64) (local $fa f64) (local $fb f64)
    (local.set $c (i32.const 0))
    (if (result i32)
        (i32.and (i32.eq (call $tag_of (local.get $a)) (i32.const {TAG_STR}))
                 (i32.eq (call $tag_of (local.get $b)) (i32.const {TAG_STR})))
      (then
        (if (i32.eq (local.get $op) (i32.const 4))
          (then (local.set $c (call $str_eq (local.get $a) (local.get $b))))
          (else (local.set $c (i32.eqz (call $str_eq (local.get $a) (local.get $b))))))
        (call $mk_bool (local.get $c)))
      (else (if (result i32)
          (i32.and (i32.eq (call $tag_of (local.get $a)) (i32.const {TAG_I64}))
                   (i32.eq (call $tag_of (local.get $b)) (i32.const {TAG_I64})))
        (then
          (local.set $va (call $i64_of (local.get $a)))
          (local.set $vb (call $i64_of (local.get $b)))
          (if (i32.eq (local.get $op) (i32.const 0))
            (then (local.set $c (i64.lt_s (local.get $va) (local.get $vb)))))
          (if (i32.eq (local.get $op) (i32.const 1))
            (then (local.set $c (i64.le_s (local.get $va) (local.get $vb)))))
          (if (i32.eq (local.get $op) (i32.const 2))
            (then (local.set $c (i64.gt_s (local.get $va) (local.get $vb)))))
          (if (i32.eq (local.get $op) (i32.const 3))
            (then (local.set $c (i64.ge_s (local.get $va) (local.get $vb)))))
          (if (i32.eq (local.get $op) (i32.const 4))
            (then (local.set $c (i64.eq (local.get $va) (local.get $vb)))))
          (if (i32.eq (local.get $op) (i32.const 5))
            (then (local.set $c (i64.ne (local.get $va) (local.get $vb)))))
          (call $mk_bool (local.get $c)))
        (else
          (local.set $fa (call $as_f64 (local.get $a)))
          (local.set $fb (call $as_f64 (local.get $b)))
          (if (i32.eq (local.get $op) (i32.const 0))
            (then (local.set $c (f64.lt (local.get $fa) (local.get $fb)))))
          (if (i32.eq (local.get $op) (i32.const 1))
            (then (local.set $c (f64.le (local.get $fa) (local.get $fb)))))
          (if (i32.eq (local.get $op) (i32.const 2))
            (then (local.set $c (f64.gt (local.get $fa) (local.get $fb)))))
          (if (i32.eq (local.get $op) (i32.const 3))
            (then (local.set $c (f64.ge (local.get $fa) (local.get $fb)))))
          (if (i32.eq (local.get $op) (i32.const 4))
            (then (local.set $c (f64.eq (local.get $fa) (local.get $fb)))))
          (if (i32.eq (local.get $op) (i32.const 5))
            (then (local.set $c (f64.ne (local.get $fa) (local.get $fb)))))
          (call $mk_bool (local.get $c)))))))
  (func $dict_get (param $d i32) (param $key i32) (result i32)
    (local $n i32) (local $i i32) (local $k i32)
    (local.set $n (call $len_of (local.get $d)))
    (local.set $i (i32.const 0))
    (block $done (loop $L
      (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
      (local.set $k (i32.load (i32.add (i32.add (local.get $d) (i32.const 8))
                                       (i32.mul (local.get $i) (i32.const 8)))))
      (if (call $str_eq (local.get $k) (local.get $key))
        (then (return (i32.load (i32.add (i32.add (i32.add (local.get $d) (i32.const 8))
                                                  (i32.mul (local.get $i) (i32.const 8)))
                                        (i32.const 4))))))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br $L)))
    (i32.const 0))
  (func $dict_set (param $d i32) (param $key i32) (param $val i32)
    (local $n i32) (local $i i32) (local $k i32)
    (local.set $n (call $len_of (local.get $d)))
    (local.set $i (i32.const 0))
    (block $done (loop $L
      (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
      (local.set $k (i32.load (i32.add (i32.add (local.get $d) (i32.const 8))
                                       (i32.mul (local.get $i) (i32.const 8)))))
      (if (call $str_eq (local.get $k) (local.get $key))
        (then
          (i32.store (i32.add (i32.add (i32.add (local.get $d) (i32.const 8))
                                       (i32.mul (local.get $i) (i32.const 8)))
                             (i32.const 4)) (local.get $val))
          (return)))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br $L)))
    (unreachable))
  (func $typeof_name (param $h i32) (result i32)
    (local $t i32) (local $s i32) (local $n i32)
    (local.set $t (call $tag_of (local.get $h)))
    (local.set $s (i32.const {SAVE0}))
    (local.set $n (i32.const 0))
    (if (i32.eq (local.get $t) (i32.const {TAG_NULL})) (then
      (i32.store8 (local.get $s) (i32.const 110))
      (i32.store8 (i32.add (local.get $s) (i32.const 1)) (i32.const 117))
      (i32.store8 (i32.add (local.get $s) (i32.const 2)) (i32.const 108))
      (i32.store8 (i32.add (local.get $s) (i32.const 3)) (i32.const 108))
      (local.set $n (i32.const 4))))
    (if (i32.eq (local.get $t) (i32.const {TAG_I64})) (then
      (i32.store8 (local.get $s) (i32.const 105))
      (i32.store8 (i32.add (local.get $s) (i32.const 1)) (i32.const 54))
      (i32.store8 (i32.add (local.get $s) (i32.const 2)) (i32.const 52))
      (local.set $n (i32.const 3))))
    (if (i32.eq (local.get $t) (i32.const {TAG_STR})) (then
      (i32.store8 (local.get $s) (i32.const 115))
      (i32.store8 (i32.add (local.get $s) (i32.const 1)) (i32.const 116))
      (i32.store8 (i32.add (local.get $s) (i32.const 2)) (i32.const 114))
      (local.set $n (i32.const 3))))
    (call $mk_str (local.get $s) (local.get $n)))
"""


def _map_data(addr: int, indices: list[int]) -> str:
    if not indices:
        return ""
    blob = b"".join(i.to_bytes(4, "little") for i in indices)
    return f'(data (i32.const {addr}) "{_esc(blob)}")'


def _host_api_wat(flat_slots: list[str], local_names: list[str],
                  globals_used: list[str]) -> str:
    """Host ABI aligned with ``native/ujs_vm.c`` host_* (path-A binders)."""
    six = {n: i for i, n in enumerate(flat_slots)}
    lmap = [six[n] for n in local_names]
    gmap = [six[n] for n in globals_used]
    parts = [_map_data(LMAP0, lmap), _map_data(GMAP0, gmap)]
    parts.append(f"""
  ;; clear/set slots used by load_g/store_g (and locals): flat LOCALS0 table
  (func $clear_slots (result i32)
    (local $i i32)
    (local.set $i (i32.const 0))
    (block $c (loop $L
      (br_if $c (i32.ge_u (local.get $i) (i32.const 64)))
      (i32.store (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4))) (i32.const 0))
      (local.set $i (i32.add (local.get $i) (i32.const 1)))
      (br $L)))
    (i32.const 0))
  (func $host_reset (result i32)
    (global.set $freep (i32.const {HEAP0}))
    (global.set $sp (i32.const {STACK0}))
    (call $clear_slots))
  ;; run body without wiping injected slots / heap (sp only)
  (func $run_step (result i32)
    (global.set $sp (i32.const {STACK0}))
    (call $main_body))
  (func $host_run (result i32) (call $run_step))
  (func $host_set_local (param $i i32) (param $h i32)
    (if (i32.ge_u (local.get $i) (i32.const {len(lmap)})) (then (return)))
    (i32.store
      (i32.add (i32.const {LOCALS0})
        (i32.mul (i32.load (i32.add (i32.const {LMAP0})
          (i32.mul (local.get $i) (i32.const 4)))) (i32.const 4)))
      (local.get $h)))
  (func $host_set_global (param $i i32) (param $h i32)
    (if (i32.ge_u (local.get $i) (i32.const {len(gmap)})) (then (return)))
    (i32.store
      (i32.add (i32.const {LOCALS0})
        (i32.mul (i32.load (i32.add (i32.const {GMAP0})
          (i32.mul (local.get $i) (i32.const 4)))) (i32.const 4)))
      (local.get $h)))
  (func $host_get_local (param $i i32) (result i32)
    (if (result i32) (i32.ge_u (local.get $i) (i32.const {len(lmap)}))
      (then (i32.const 0))
      (else (i32.load
        (i32.add (i32.const {LOCALS0})
          (i32.mul (i32.load (i32.add (i32.const {LMAP0})
            (i32.mul (local.get $i) (i32.const 4)))) (i32.const 4)))))))
  (func $host_get_global (param $i i32) (result i32)
    (if (result i32) (i32.ge_u (local.get $i) (i32.const {len(gmap)}))
      (then (i32.const 0))
      (else (i32.load
        (i32.add (i32.const {LOCALS0})
          (i32.mul (i32.load (i32.add (i32.const {GMAP0})
            (i32.mul (local.get $i) (i32.const 4)))) (i32.const 4)))))))
  (func $host_mk_null (result i32) (i32.const 0))
  (func $host_mk_bool (param $b i32) (result i32) (call $mk_bool (local.get $b)))
  (func $host_mk_i64 (param $v i64) (result i32) (call $mk_i64 (local.get $v)))
  (func $host_mk_f64 (param $v f64) (result i32) (call $mk_f64 (local.get $v)))
  (func $host_mk_str (param $ptr i32) (param $n i32) (result i32)
    (call $mk_str (local.get $ptr) (local.get $n)))
  (func $host_mk_list (param $n i32) (result i32)
    (local $p i32) (local $i i32)
    (if (result i32) (i32.gt_u (local.get $n) (i32.const 4096))
      (then (i32.const 0))
      (else
        (local.set $p (call $alloc (i32.add (i32.const 8)
          (i32.mul (local.get $n) (i32.const 4)))))
        (i32.store8 (local.get $p) (i32.const {TAG_LIST}))
        (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $n))
        (local.set $i (i32.const 0))
        (block $done (loop $L
          (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
          (i32.store (i32.add (i32.add (local.get $p) (i32.const 8))
            (i32.mul (local.get $i) (i32.const 4))) (i32.const 0))
          (local.set $i (i32.add (local.get $i) (i32.const 1)))
          (br $L)))
        (local.get $p))))
  (func $host_list_set (param $h i32) (param $i i32) (param $v i32)
    (if (i32.ne (call $tag_of (local.get $h)) (i32.const {TAG_LIST})) (then (return)))
    (if (i32.ge_u (local.get $i) (call $len_of (local.get $h))) (then (return)))
    (i32.store (i32.add (i32.add (local.get $h) (i32.const 8))
      (i32.mul (local.get $i) (i32.const 4))) (local.get $v)))
  (func $host_list_get (param $h i32) (param $i i32) (result i32)
    (if (result i32) (i32.ne (call $tag_of (local.get $h)) (i32.const {TAG_LIST}))
      (then (i32.const 0))
      (else (if (result i32) (i32.ge_u (local.get $i) (call $len_of (local.get $h)))
        (then (i32.const 0))
        (else (i32.load (i32.add (i32.add (local.get $h) (i32.const 8))
          (i32.mul (local.get $i) (i32.const 4)))))))))
  (func $host_mk_dict (param $n i32) (result i32)
    (local $p i32) (local $i i32)
    (if (result i32) (i32.gt_u (local.get $n) (i32.const 4096))
      (then (i32.const 0))
      (else
        (local.set $p (call $alloc (i32.add (i32.const 8)
          (i32.mul (local.get $n) (i32.const 8)))))
        (i32.store8 (local.get $p) (i32.const {TAG_DICT}))
        (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $n))
        (local.set $i (i32.const 0))
        (block $done (loop $L
          (br_if $done (i32.ge_u (local.get $i) (local.get $n)))
          (i32.store (i32.add (i32.add (local.get $p) (i32.const 8))
            (i32.mul (local.get $i) (i32.const 8))) (i32.const 0))
          (i32.store (i32.add (i32.add (i32.add (local.get $p) (i32.const 8))
            (i32.mul (local.get $i) (i32.const 8))) (i32.const 4)) (i32.const 0))
          (local.set $i (i32.add (local.get $i) (i32.const 1)))
          (br $L)))
        (local.get $p))))
  (func $host_dict_set (param $h i32) (param $i i32) (param $k i32) (param $v i32)
    (if (i32.ne (call $tag_of (local.get $h)) (i32.const {TAG_DICT})) (then (return)))
    (if (i32.ge_u (local.get $i) (call $len_of (local.get $h))) (then (return)))
    (i32.store (i32.add (i32.add (local.get $h) (i32.const 8))
      (i32.mul (local.get $i) (i32.const 8))) (local.get $k))
    (i32.store (i32.add (i32.add (i32.add (local.get $h) (i32.const 8))
      (i32.mul (local.get $i) (i32.const 8))) (i32.const 4)) (local.get $v)))
  (func $host_dict_key (param $h i32) (param $i i32) (result i32)
    (if (result i32) (i32.ne (call $tag_of (local.get $h)) (i32.const {TAG_DICT}))
      (then (i32.const 0))
      (else (if (result i32) (i32.ge_u (local.get $i) (call $len_of (local.get $h)))
        (then (i32.const 0))
        (else (i32.load (i32.add (i32.add (local.get $h) (i32.const 8))
          (i32.mul (local.get $i) (i32.const 8)))))))))
  (func $host_dict_val (param $h i32) (param $i i32) (result i32)
    (if (result i32) (i32.ne (call $tag_of (local.get $h)) (i32.const {TAG_DICT}))
      (then (i32.const 0))
      (else (if (result i32) (i32.ge_u (local.get $i) (call $len_of (local.get $h)))
        (then (i32.const 0))
        (else (i32.load (i32.add (i32.add (i32.add (local.get $h) (i32.const 8))
          (i32.mul (local.get $i) (i32.const 8))) (i32.const 4))))))))
  (func $host_len (param $h i32) (result i32) (call $len_of (local.get $h)))
  (func $host_scratch (result i32) (i32.const {HOST_SCRATCH}))
  (func $last_ic_stub_export (result i32) (i32.const 0))
  (func $host_prog_addr (result i32) (i32.const 0))
  (func $host_load_image (result i32) (i32.const 0))
""")
    return "\n".join(p for p in parts if p)


def _normalize(fn: Fn, strings: list[str]) -> Fn:
    code = []
    for ins in fn.code:
        if ins.op == "const" and ins.a == "str":
            code.append(Ins("const", "str", strings.index(fn.strings[int(ins.b)])))
        elif ins.op == "dot" and isinstance(ins.a, str):
            code.append(Ins("dot", strings.index(ins.a)))
        else:
            code.append(ins)
    return Fn(fn.src, code, _slot_names(fn), strings, dict(fn.meta or {}))


def _body(fn: Fn, str_off: list[tuple[int, int]], fn_index: dict[str, int],
          name: str) -> str:
    slots = list(fn.localslot)
    for ins in fn.code:
        if ins.op in ("load_l", "store_l", "load_g", "store_g") and ins.a not in slots:
            slots.append(ins.a)
    six = {n: i for i, n in enumerate(slots)}
    n = len(fn.code)

    def la(nm):
        return f"(i32.add (i32.const {LOCALS0}) (i32.const {six[nm]*4}))"

    def npc(i):
        return f"(i32.const {i+1}) (local.set $pc) (br $dispatch)"

    def spc(t):
        return f"(i32.const {t}) (local.set $pc) (br $dispatch)"

    nparams = int((fn.meta or {}).get("nparams") or 0)
    rest_name = (fn.meta or {}).get("rest")
    rest_ix = six[rest_name] if rest_name in six else None

    parts = [f"(func ${name} (result i32)",
             "(local $pc i32) (local $a i32) (local $b i32) (local $c i32)",
             "(local $i i32) (local $p i32) (local $t i32) (local $nn i32)"]
    if name != "main_body":
        parts.append("(local.set $nn (global.get $call_argc))")
        for pi in range(nparams):
            parts.append(
                f"(i32.store (i32.add (i32.const {LOCALS0}) (i32.const {pi * 4})) "
                f"(i32.load (i32.add (i32.const {SAVE0 + 256}) (i32.const {pi * 4}))))")
        if rest_ix is not None:
            parts.append(f"""
          (local.set $i (i32.sub (local.get $nn) (i32.const {nparams})))
          (if (i32.lt_s (local.get $i) (i32.const 0)) (then (local.set $i (i32.const 0))))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $i) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_LIST}))
          (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $i))
          (local.set $t (i32.const 0))
          (block $rd (loop $RL
            (br_if $rd (i32.ge_u (local.get $t) (local.get $i)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $t) (i32.const 4)))
              (i32.load (i32.add (i32.const {SAVE0 + 256})
                (i32.mul (i32.add (local.get $t) (i32.const {nparams})) (i32.const 4)))))
            (local.set $t (i32.add (local.get $t) (i32.const 1)))
            (br $RL)))
          (i32.store (i32.add (i32.const {LOCALS0}) (i32.const {rest_ix * 4})) (local.get $p))
""")
    parts += ["(local.set $pc (i32.const 0))", "(loop $dispatch"]
    for i in range(n - 1, -1, -1):
        parts.append(f"(block $bb{i}")
    parts.append("(block $trap")
    parts.append("(local.get $pc)")
    parts.append("(br_table " + " ".join(str(j + 1) for j in range(n)) + " 0)")
    parts.append(") (unreachable)")

    for i, ins in enumerate(fn.code):
        parts.append(")")
        op, a, b = ins.op, ins.a, ins.b
        if op in ("nop", "ic_enter"):
            parts.append(npc(i))
        elif op == "const":
            if a == "null":
                parts.append("(call $push (i32.const 0))")
            elif a == "bool":
                parts.append(f"(call $push (call $mk_bool (i32.const {1 if b else 0})))")
            elif a == "i64":
                parts.append(f"(call $push (call $mk_i64 (i64.const {int(b)})))")
            elif a == "f64":
                parts.append(f"(call $push (call $mk_f64 (f64.const {float(b)})))")
            elif a == "str":
                off, ln = str_off[int(b)]
                parts.append(f"(call $push (call $mk_str (i32.const {off}) (i32.const {ln})))")
            elif a == "fn":
                parts.append(f"(call $push (call $mk_fn (i32.const {fn_index[b]})))")
            else:
                raise DirectEmitError("const " + str(a))
            parts.append(npc(i))
        elif op in ("load_l", "load_g"):
            parts.append(f"(call $push (i32.load {la(a)}))")
            parts.append(npc(i))
        elif op in ("store_l", "store_g"):
            parts.append(f"(i32.store {la(a)} (call $pop))")
            parts.append(npc(i))
        elif op == "drop":
            parts.append("(drop (call $pop))")
            parts.append(npc(i))
        elif op == "add":
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            parts.append("(call $push (call $op_add (local.get $a) (local.get $b)))")
            parts.append(npc(i))
        elif op in ("sub", "mul", "div", "mod"):
            code = {"sub": 0, "mul": 1, "div": 2, "mod": 3}[op]
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            parts.append(f"(call $push (call $op_bin (i32.const {code}) (local.get $a) (local.get $b)))")
            parts.append(npc(i))
        elif op in ("lt", "le", "gt", "ge", "eq", "ne"):
            code = {"lt": 0, "le": 1, "gt": 2, "ge": 3, "eq": 4, "ne": 5}[op]
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            parts.append(f"(call $push (call $op_cmp (i32.const {code}) (local.get $a) (local.get $b)))")
            parts.append(npc(i))
        elif op == "not":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $mk_bool (i32.eqz (call $truthy (local.get $a)))))")
            parts.append(npc(i))
        elif op == "and":
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            parts.append("(call $push (call $mk_bool (i32.and (call $truthy (local.get $a)) (call $truthy (local.get $b)))))")
            parts.append(npc(i))
        elif op == "or":
            parts.append("(local.set $b (call $pop)) (local.set $a (call $pop))")
            parts.append("(call $push (call $mk_bool (i32.or (call $truthy (local.get $a)) (call $truthy (local.get $b)))))")
            parts.append(npc(i))
        elif op == "jump":
            parts.append(spc(int(a)))
        elif op == "jumpz":
            parts.append("(local.set $a (call $pop))")
            parts.append(
                "(if (i32.eqz (call $truthy (local.get $a))) "
                f"(then (i32.const {int(a)}) (local.set $pc) (br $dispatch)))")
            parts.append(npc(i))
        elif op == "jumpnz":
            parts.append("(local.set $a (call $pop))")
            parts.append(
                "(if (call $truthy (local.get $a)) "
                f"(then (i32.const {int(a)}) (local.set $pc) (br $dispatch)))")
            parts.append(npc(i))
        elif op == "ret":
            parts.append("(return (call $pop))")
        elif op == "mklist":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_LIST}))
          (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 4))) (call $pop))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "mkdict":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 8)))))
          (i32.store8 (local.get $p) (i32.const {TAG_DICT}))
          (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (local.set $b (call $pop)) (local.set $a (call $pop))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 8))) (local.get $a))
            (i32.store (i32.add (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 8))) (i32.const 4)) (local.get $b))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "mktup":
            cnt = int(a)
            parts.append(f"""
          (local.set $nn (i32.const {cnt}))
          (local.set $p (call $alloc (i32.add (i32.const 8) (i32.mul (local.get $nn) (i32.const 4)))))
          (i32.store8 (local.get $p) (i32.const {TAG_TUP}))
          (i32.store16 (i32.add (local.get $p) (i32.const 2)) (local.get $nn))
          (local.set $i (local.get $nn))
          (block $done (loop $L
            (br_if $done (i32.eqz (local.get $i)))
            (local.set $i (i32.sub (local.get $i) (i32.const 1)))
            (i32.store (i32.add (i32.add (local.get $p) (i32.const 8)) (i32.mul (local.get $i) (i32.const 4))) (call $pop))
            (br $L)))
          (call $push (local.get $p))
""")
            parts.append(npc(i))
        elif op == "len":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $mk_i64 (i64.extend_i32_u (call $len_of (local.get $a)))))")
            parts.append(npc(i))
        elif op == "idx":
            parts.append(f"""
          (local.set $b (call $pop)) (local.set $a (call $pop))
          (local.set $t (call $tag_of (local.get $a)))
          (if (i32.or (i32.eq (local.get $t) (i32.const {TAG_LIST}))
                      (i32.eq (local.get $t) (i32.const {TAG_TUP})))
            (then (call $push (i32.load (i32.add (i32.add (local.get $a) (i32.const 8))
              (i32.mul (i32.wrap_i64 (call $i64_of (local.get $b))) (i32.const 4))))))
            (else (if (i32.eq (local.get $t) (i32.const {TAG_DICT}))
              (then (call $push (call $dict_get (local.get $a) (local.get $b))))
              (else (unreachable)))))
""")
            parts.append(npc(i))
        elif op == "setidx":
            # stack: … xs ix val  → pop val, ix, xs; store; push val
            parts.append(f"""
          (local.set $c (call $pop)) (local.set $b (call $pop)) (local.set $a (call $pop))
          (local.set $t (call $tag_of (local.get $a)))
          (if (i32.or (i32.eq (local.get $t) (i32.const {TAG_LIST}))
                      (i32.eq (local.get $t) (i32.const {TAG_TUP})))
            (then (i32.store (i32.add (i32.add (local.get $a) (i32.const 8))
              (i32.mul (i32.wrap_i64 (call $i64_of (local.get $b))) (i32.const 4))) (local.get $c)))
            (else (if (i32.eq (local.get $t) (i32.const {TAG_DICT}))
              (then (call $dict_set (local.get $a) (local.get $b) (local.get $c)))
              (else (unreachable)))))
          (call $push (local.get $c))
""")
            parts.append(npc(i))
        elif op == "dot":
            off, ln = str_off[int(a)]
            parts.append(f"""
          (local.set $a (call $pop))
          (local.set $b (call $mk_str (i32.const {off}) (i32.const {ln})))
          (call $push (call $dict_get (local.get $a) (local.get $b)))
""")
            parts.append(npc(i))
        elif op == "typeof":
            parts.append("(local.set $a (call $pop))")
            parts.append("(call $push (call $typeof_name (local.get $a)))")
            parts.append(npc(i))
        elif op == "call":
            argc = int(a)
            for ai in range(argc):
                parts.append("(local.set $a (call $pop))")
                parts.append(
                    f"(i32.store (i32.add (i32.const {SAVE0+256}) (i32.const {(argc-1-ai)*4})) (local.get $a))")
            parts.append(f"""
          (local.set $a (call $pop))
          (if (i32.ne (call $tag_of (local.get $a)) (i32.const {TAG_FN})) (then (unreachable)))
          (local.set $t (i32.load (i32.add (local.get $a) (i32.const 4))))
          (local.set $i (i32.const 0))
          (block $ds (loop $LS
            (br_if $ds (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {SAVE0}) (i32.mul (local.get $i) (i32.const 4)))
              (i32.load (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4)))))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $LS)))
          (local.set $i (i32.const 0))
          (block $dc (loop $Lc
            (br_if $dc (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4))) (i32.const 0))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $Lc)))
""")
            parts.append(f"(global.set $call_argc (i32.const {argc}))")
            parts.append("(call $push (call $dispatch_fn (local.get $t)))")
            parts.append(f"""
          (local.set $i (i32.const 0))
          (block $dr (loop $Lr
            (br_if $dr (i32.ge_u (local.get $i) (i32.const 64)))
            (i32.store (i32.add (i32.const {LOCALS0}) (i32.mul (local.get $i) (i32.const 4)))
              (i32.load (i32.add (i32.const {SAVE0}) (i32.mul (local.get $i) (i32.const 4)))))
            (local.set $i (i32.add (local.get $i) (i32.const 1)))
            (br $Lr)))
""")
            parts.append(npc(i))
        else:
            raise DirectEmitError("emit gap " + op)

    parts.append("(unreachable)) (unreachable))")
    return "\n".join(parts)


def emit_wat(fn: Fn, oracle: Oracle | None = None) -> str:
    o = oracle or Oracle(drive="gold")
    ok, why = can_emit_direct(fn, o)
    if not ok:
        raise DirectEmitError(why)

    nested = dict(fn.meta.get("fns") or {})
    strings = list(fn.strings)
    for nf in nested.values():
        for s in nf.strings:
            if s not in strings:
                strings.append(s)
    for fobj in [fn, *nested.values()]:
        for ins in fobj.code:
            if ins.op == "dot" and isinstance(ins.a, str) and ins.a not in strings:
                strings.append(ins.a)

    blob = bytearray()
    str_off = []
    for s in strings:
        b = s.encode("utf-8")
        str_off.append((STRDATA0 + len(blob), len(b)))
        blob += b

    fn_names = sorted(nested.keys())
    fn_index = {name: i for i, name in enumerate(fn_names)}

    out = ["(module", _helpers()]
    if blob:
        out.append(f'(data (i32.const {STRDATA0}) "{_esc(bytes(blob))}")')

    # nested functions first (so dispatch can call them)
    for i, name in enumerate(fn_names):
        out.append(_body(_normalize(nested[name], strings), str_off, fn_index, f"fn_{i}"))

    nfn = len(fn_names)
    if nfn == 0:
        out.append("(func $dispatch_fn (param $ix i32) (result i32) (unreachable))")
    else:
        d = ["(func $dispatch_fn (param $ix i32) (result i32)", "(block $bad"]
        for i in range(nfn - 1, -1, -1):
            d.append(f"(block $f{i}")
        d.append("(local.get $ix)")
        d.append("(br_table " + " ".join(str(i) for i in range(nfn)) + f" {nfn})")
        for i in range(nfn):
            d.append(")")  # close $f_i
            d.append(f"(return (call $fn_{i}))")
        d.append(")")  # close $bad
        d.append("(unreachable))")  # default + close func
        out.append("\n".join(d))

    out.append(_body(_normalize(fn, strings), str_off, fn_index, "main_body"))
    flat = _slot_names(fn)
    local_names, globals_used = _gl_names(fn)
    out.append(_host_api_wat(flat, local_names, globals_used))
    out.append(f"""
  (func $main_export (result i32)
    (drop (call $host_reset))
    (call $main_body))
  (func $tag_of_export (param $h i32) (result i32) (call $tag_of (local.get $h)))
  (func $i64_of_export (param $h i32) (result i64) (call $i64_of (local.get $h)))
  (func $f64_of_export (param $h i32) (result f64)
    (if (result f64) (i32.eq (call $tag_of (local.get $h)) (i32.const {TAG_F64}))
      (then (call $f64_of (local.get $h)))
      (else (f64.convert_i64_s (call $i64_of (local.get $h))))))
  (func $str_len_export (param $h i32) (result i32) (call $len_of (local.get $h)))
  (func $str_ptr_export (param $h i32) (result i32)
    (if (result i32) (i32.eqz (local.get $h)) (then (i32.const 0))
      (else (i32.add (local.get $h) (i32.const 8)))))
  (func $mem_base (result i32) (i32.const 0))
  (func $mem_size (result i32) (i32.const {MEM_BYTES}))
  (export "main_export" (func $main_export))
  (export "clear_slots" (func $clear_slots))
  (export "run_step" (func $run_step))
  (export "host_reset" (func $host_reset))
  (export "host_run" (func $host_run))
  (export "host_set_local" (func $host_set_local))
  (export "host_set_global" (func $host_set_global))
  (export "host_get_local" (func $host_get_local))
  (export "host_get_global" (func $host_get_global))
  (export "host_mk_null" (func $host_mk_null))
  (export "host_mk_bool" (func $host_mk_bool))
  (export "host_mk_i64" (func $host_mk_i64))
  (export "host_mk_f64" (func $host_mk_f64))
  (export "host_mk_str" (func $host_mk_str))
  (export "host_mk_list" (func $host_mk_list))
  (export "host_list_set" (func $host_list_set))
  (export "host_list_get" (func $host_list_get))
  (export "host_mk_dict" (func $host_mk_dict))
  (export "host_dict_set" (func $host_dict_set))
  (export "host_dict_key" (func $host_dict_key))
  (export "host_dict_val" (func $host_dict_val))
  (export "host_len" (func $host_len))
  (export "host_scratch" (func $host_scratch))
  (export "host_prog_addr" (func $host_prog_addr))
  (export "host_load_image" (func $host_load_image))
  (export "last_ic_stub_export" (func $last_ic_stub_export))
  (export "tag_of_export" (func $tag_of_export))
  (export "i64_of_export" (func $i64_of_export))
  (export "f64_of_export" (func $f64_of_export))
  (export "str_len_export" (func $str_len_export))
  (export "str_ptr_export" (func $str_ptr_export))
  (export "mem_base" (func $mem_base))
  (export "mem_size" (func $mem_size))
)
""")
    return "\n".join(out)


def emit_wasm(fn: Fn, out_path: str, oracle: Oracle | None = None) -> dict:
    wat = emit_wat(fn, oracle)
    with tempfile.TemporaryDirectory() as td:
        wat_path = os.path.join(td, "p.wat")
        open(wat_path, "w").write(wat)
        try:
            subprocess.check_call(["wat2wasm", wat_path, "-o", out_path])
        except subprocess.CalledProcessError as e:
            open(out_path + ".wat", "w").write(wat)
            raise DirectEmitError("wat2wasm failed; wrote %s.wat" % out_path) from e
    if open(out_path, "rb").read(4) != b"\0asm":
        raise DirectEmitError("not wasm")
    local_names, globals_used = _gl_names(fn)
    return {
        "wasm": out_path,
        "bytes": os.path.getsize(out_path),
        "direct": True,
        "image": len(fn.code),
        "locals": local_names,
        "globals": globals_used,
        "slots": _slot_names(fn),
    }
