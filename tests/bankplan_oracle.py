#!/usr/bin/env python3
"""Independent oracle for register-bank call plans (R12-1 ①, design: archive/research/r11/r11-bank-design.md).

A plan says, for one prototype on one profile, how each argument's bytes move into the
GP bank, the FP bank or the stack image, and how the result is captured. This oracle is
the reference the nativeabi table will be compared against byte for byte; it is written
with counters and bounded loops only, exactly as the table must be.

Types are already-classified facts (what the certifier computes for a graph):
  I(width, signed)          integer scalar or pointer
  F(width)                  float (4) / double (8)
  AGG(size, align, lanes, memory=False, hfa=None)
      lanes: per eightbyte class 'INT' or 'SSE' (SysV only, len 1..2, size<=16)
      memory: True when SysV must pass the object in memory (unaligned field, >16 B)
      hfa: (count, width) when AAPCS64 treats it as a homogeneous FP aggregate
Refusals are explicit: RuntimeError('refuse: ...').

Plan record BNK1 (little-endian):
  header 32 B: magic 'BNK1', version u16=1, profile u8, flags u8 (b0 result MEMORY),
               total length u32, prototype length u32, nparams u8, gp_used u8, fp_used u8,
               al u8, stack bytes u16, move count u16, result count u8, hidden register u8
               (0xFF none), scratch bytes u16, crc32 u32 of everything after the header
  prototype bytes (owned, replayed by the host)
  M moves x 8 B: param u8, src kind u8 (0 slot value, 1 bytes at slot pointer, 2 scratch),
                 src offset u16, dst kind u8 (0 GP, 1 FP-lo, 2 FP-hi, 3 STACK, 4 SCRATCH),
                 dst index u8, width u8, ext/hi u8 (GP: 0 zero-ext 1 sign-ext; STACK/SCRATCH: high byte of offset)
  R results x 4 B: source u8 (0 rax/x0, 1 rdx/x1, 2 xmm0/v0, 3 xmm1/v1, 4 v2, 5 v3), width u8, dst offset u16
"""
import json, struct, sys, zlib
from dataclasses import dataclass, field
from typing import List, Optional, Tuple

PROFILES = {'osx/arm64': 0, 'lnx/arm64': 1, 'win/arm64': 2, 'osx/x86_64': 3, 'lnx/x86_64': 4, 'win/x86_64': 5}
GP, FPLO, FPHI, STACK, SCRATCH = 0, 1, 2, 3, 4
SLOT, BYTES, SCR = 0, 1, 2

@dataclass
class I:
    width: int; signed: bool = False
    size = property(lambda s: s.width); align = property(lambda s: s.width)
@dataclass
class F:
    width: int
    size = property(lambda s: s.width); align = property(lambda s: s.width)
@dataclass
class AGG:
    size: int; align: int; lanes: List[str] = field(default_factory=list); memory: bool = False; hfa: Optional[Tuple[int, int]] = None

@dataclass
class Move:
    param: int; src_kind: int; src_off: int; dst_kind: int; dst_index: int; width: int; ext: int = 0
    def pack(self):
        if self.dst_kind in (STACK, SCRATCH):
            return struct.pack('<BBHBBBB', self.param, self.src_kind, self.src_off, self.dst_kind, self.dst_index & 255, self.width, self.dst_index >> 8)
        return struct.pack('<BBHBBBB', self.param, self.src_kind, self.src_off, self.dst_kind, self.dst_index, self.width, self.ext)

def refuse(why): raise RuntimeError('refuse: ' + why)
def roundup(x, a): return (x + a - 1) // a * a

def sysv(params, result):
    """SysV x86_64: 6 GP, 8 SSE; whole-object spill leaves counters alone; MEMORY objects copied to the stack."""
    ngrn = nsrn = nsaa = 0; moves = []; results = []; hidden = 0xFF; flags = 0
    if isinstance(result, AGG) and (result.memory or result.size > 16):
        flags |= 1; hidden = 0; ngrn = 1                     # rdi carries the result address
    for p, t in enumerate(params):
        if isinstance(t, I):
            if t.width not in (1, 2, 4, 8): refuse('integer width')
            if ngrn < 6: moves.append(Move(p, SLOT, 0, GP, ngrn, t.width, int(t.signed))); ngrn += 1
            else: moves.append(Move(p, SLOT, 0, STACK, nsaa, 8)); nsaa += 8
        elif isinstance(t, F):
            if t.width not in (4, 8): refuse('fp width (x87/IEEE128)')
            if nsrn < 8: moves.append(Move(p, SLOT, 0, FPLO, nsrn, t.width)); nsrn += 1
            else: moves.append(Move(p, SLOT, 0, STACK, nsaa, 8)); nsaa += 8
        elif isinstance(t, AGG):
            if t.size == 0 or t.size > 64: refuse('aggregate extent')
            if t.align > 16: refuse('alignment above 16')
            if t.memory or t.size > 16:
                nsaa = roundup(nsaa, max(8, t.align))
                for j in range(0, (t.size + 7) // 8):
                    moves.append(Move(p, BYTES, 8 * j, STACK, nsaa + 8 * j, min(8, t.size - 8 * j)))
                nsaa += roundup(t.size, 8)
            else:
                k = (t.size + 7) // 8
                if len(t.lanes) != k: refuse('lane count')
                needg = sum(1 for c in t.lanes if c == 'INT'); needs = k - needg
                if ngrn + needg <= 6 and nsrn + needs <= 8:
                    for j, c in enumerate(t.lanes):
                        w = min(8, t.size - 8 * j)
                        if c == 'INT': moves.append(Move(p, BYTES, 8 * j, GP, ngrn, w)); ngrn += 1
                        else: moves.append(Move(p, BYTES, 8 * j, FPLO, nsrn, w)); nsrn += 1
                else:                                       # whole spill, counters untouched
                    nsaa = roundup(nsaa, max(8, t.align))
                    for j in range(k): moves.append(Move(p, BYTES, 8 * j, STACK, nsaa + 8 * j, min(8, t.size - 8 * j)))
                    nsaa += 8 * k
        else: refuse('kind')
    # result capture
    if isinstance(result, I): results.append((0, result.width, 0))
    elif isinstance(result, F): results.append((2, result.width, 0))
    elif isinstance(result, AGG) and not (flags & 1):
        gi = fi = 0
        for j, c in enumerate(result.lanes):
            w = min(8, result.size - 8 * j)
            if c == 'INT': results.append(((0, 1)[gi], w, 8 * j)); gi += 1
            else: results.append(((2, 3)[fi], w, 8 * j)); fi += 1
    elif result is None: pass
    return dict(moves=moves, results=results, gp=ngrn, fp=nsrn, al=nsrn, stack=roundup(nsaa, 16), hidden=hidden, flags=flags, scratch=0)

def aapcs64(params, result, profile):
    """AAPCS64 (macOS/Linux, non-variadic): NGRN/NSRN/NSAA; HFA all-or-nothing; >16 B by reference (caller copy)."""
    ngrn = nsrn = nsaa = scr = 0; moves = []; results = []; hidden = 0xFF; flags = 0
    if isinstance(result, AGG) and result.size > 16 and not result.hfa:
        flags |= 1; hidden = 8                              # x8, does not consume x0
    for p, t in enumerate(params):
        if isinstance(t, F) or (isinstance(t, AGG) and t.hfa):
            n, w = (1, t.width) if isinstance(t, F) else t.hfa
            if w not in (4, 8): refuse('fp width')
            if nsrn + n <= 8:
                for e in range(n): moves.append(Move(p, SLOT if n == 1 else BYTES, e * w, FPLO, nsrn + e, w))
                nsrn += n
            else:
                nsrn = 8                                    # C.3: no more SIMD registers
                if profile == 'osx/arm64' and w < 8: refuse('Apple arm64 sub-8-byte stack scalars (not probed yet)')
                nsaa = roundup(nsaa, 8)
                for e in range(n): moves.append(Move(p, SLOT if n == 1 else BYTES, e * w, STACK, nsaa + e * w, w))
                nsaa += roundup(n * w, 8)
            continue
        if isinstance(t, AGG):
            if t.size == 0 or t.size > 64: refuse('aggregate extent')
            if t.size > 16:                                  # B.4: copy to caller memory, pass the address
                scr = roundup(scr, 16)
                for j in range(0, (t.size + 7) // 8): moves.append(Move(p, BYTES, 8 * j, SCRATCH, scr + 8 * j, min(8, t.size - 8 * j)))
                src = (SCR, scr); scr += roundup(t.size, 16); words = 1; w0 = 8
            else:
                src = (BYTES, 0); words = (t.size + 7) // 8; w0 = None
            if t.align == 16: refuse('16-aligned composite: Apple arm64 passes it by reference where AAPCS64 says an even register pair (probe pending, 0.0.13)')
            if ngrn + words <= 8:
                for j in range(words):
                    moves.append(Move(p, src[0], src[1] + 8 * j, GP, ngrn + j, w0 or min(8, t.size - 8 * j)))
                ngrn += words
            else:
                ngrn = 8                                    # C.13
                nsaa = roundup(nsaa, max(8, t.align))
                for j in range(words): moves.append(Move(p, src[0], src[1] + 8 * j, STACK, nsaa + 8 * j, w0 or min(8, t.size - 8 * j)))
                nsaa += 8 * words
            continue
        if isinstance(t, I):
            if t.width not in (1, 2, 4, 8): refuse('integer width')
            if ngrn < 8: moves.append(Move(p, SLOT, 0, GP, ngrn, t.width, int(t.signed))); ngrn += 1
            else:
                if profile == 'osx/arm64' and t.width < 8: refuse('Apple arm64 sub-8-byte stack scalars (not probed yet)')
                moves.append(Move(p, SLOT, 0, STACK, nsaa, 8)); nsaa += 8
            continue
        refuse('kind')
    if isinstance(result, I): results.append((0, result.width, 0))
    elif isinstance(result, F): results.append((2, result.width, 0))
    elif isinstance(result, AGG) and not (flags & 1):
        if result.hfa:
            n, w = result.hfa
            for e in range(n): results.append((2 + e, w, e * w))
        else:
            for j in range(0, (result.size + 7) // 8): results.append((j, min(8, result.size - 8 * j), 8 * j))
    return dict(moves=moves, results=results, gp=ngrn, fp=nsrn, al=0, stack=roundup(nsaa, 16), hidden=hidden, flags=flags, scratch=roundup(scr, 16))

def plan(profile, params, result, prototype: bytes):
    if profile not in PROFILES: refuse('profile')
    if len(params) > 32: refuse('parameter count')
    if profile.startswith('win/'): refuse('Windows profiles are not on the BANK route yet')
    alloc = sysv(params, result) if profile.endswith('x86_64') else aapcs64(params, result, profile)
    moves = alloc['moves']; results = alloc['results']
    if alloc['stack'] > 512 or alloc['scratch'] > 1024 or len(moves) > 256 or len(results) > 8: refuse('plan bounds')
    body = prototype + b''.join(m.pack() for m in moves) + b''.join(struct.pack('<BBH', s, w, o) for s, w, o in results)
    L = 32 + len(body)
    head = struct.pack('<4sHBBIIBBBBHHBBHI', b'BNK1', 1, PROFILES[profile], alloc['flags'], L, len(prototype), len(params),
                       alloc['gp'], alloc['fp'], alloc['al'], alloc['stack'], len(moves), len(results), alloc['hidden'], alloc['scratch'],
                       zlib.crc32(body) & 0xffffffff)
    assert len(head) == 32
    return head + body, alloc

def describe(alloc):
    names = {GP: 'GP', FPLO: 'FP', FPHI: 'FPhi', STACK: 'STACK', SCRATCH: 'SCR'}
    return [f"p{m.param}:{('slot','bytes','scr')[m.src_kind]}+{m.src_off} -> {names[m.dst_kind]}[{m.dst_index}] w{m.width}" for m in alloc['moves']]

# ---- fixtures from the design report (family A, family B, and refusals) ----
def selftest():
    out = {}
    # Family A: SysV 5-byte packed struct -> MEMORY; alone, after 5 GP, after 7 GP; and as return (hidden rdi)
    P5 = AGG(5, 1, memory=True)
    for pre, label in ((0, 'alone'), (5, 'after5gp'), (7, 'after7gp')):
        rec, a = plan('lnx/x86_64', [I(8)] * pre + [P5], I(4), b'protoA')
        st = [m for m in a['moves'] if m.dst_kind == STACK]
        assert st[-1].width == 5 and st[-1].src_kind == BYTES and a['stack'] == 16 * ((8 * max(0, pre - 6) + 8 + 15) // 16), (label, describe(a), a['stack'])
        assert a['gp'] == min(pre, 6), (label, a['gp'])
        out['A-' + label] = describe(a)
    rec, a = plan('lnx/x86_64', [I(4)], P5, b'protoAret'); assert a['flags'] & 1 and a['hidden'] == 0 and a['moves'][0].dst_index == 1
    out['A-return'] = describe(a)
    # Family B: {u64,double} under GP exhaustion spills wholly; trailing double still takes xmm0
    UD = AGG(16, 8, lanes=['INT', 'SSE'])
    rec, a = plan('lnx/x86_64', [I(8)] * 6 + [UD, F(8)], I(8), b'protoB')
    assert [(m.dst_kind, m.dst_index) for m in a['moves'][6:]] == [(STACK, 0), (STACK, 8), (FPLO, 0)], describe(a)
    assert a['al'] == 1 and a['stack'] == 16, a
    out['B-gp-exhausted'] = describe(a)
    rec, a = plan('lnx/x86_64', [F(8)] * 8 + [AGG(16, 8, lanes=['SSE', 'INT']), I(8)], I(8), b'protoB2')
    assert [(m.dst_kind, m.dst_index) for m in a['moves'][8:]] == [(STACK, 0), (STACK, 8), (GP, 0)], describe(a)
    out['B-sse-exhausted'] = describe(a)
    rec, a = plan('lnx/x86_64', [UD], I(8), b'protoB3'); assert [(m.dst_kind, m.dst_index) for m in a['moves']] == [(GP, 0), (FPLO, 0)]
    out['B-both-available'] = describe(a)
    # AAPCS64: HFA all-or-nothing, >16 B by reference, x8 hidden result, 16-aligned pairs
    rec, a = plan('lnx/arm64', [F(8)] * 6 + [AGG(16, 8, hfa=(2, 8)), AGG(24, 8, hfa=(3, 8)), F(4)], AGG(32, 8), b'protoC')
    kinds = [(m.dst_kind, m.dst_index) for m in a['moves']]
    assert kinds[6:8] == [(FPLO, 6), (FPLO, 7)] and kinds[8:11] == [(STACK, 0), (STACK, 8), (STACK, 16)] and kinds[11] == (STACK, 24), describe(a)
    assert a['flags'] & 1 and a['hidden'] == 8 and a['fp'] == 8, a
    out['C-aapcs-hfa'] = describe(a)
    rec, a = plan('lnx/arm64', [I(8)] * 7 + [AGG(16, 8), AGG(40, 8)], I(8), b'protoD')
    kinds = [(m.dst_kind, m.dst_index) for m in a['moves']]
    assert kinds[7:9] == [(STACK, 0), (STACK, 8)] and kinds[9:14] == [(SCRATCH, 0), (SCRATCH, 8), (SCRATCH, 16), (SCRATCH, 24), (SCRATCH, 32)] and kinds[14] == (STACK, 16), describe(a)
    out['D-aapcs-spill-byref'] = describe(a)
    # refusals
    for prof, ps, why in (('lnx/x86_64', [F(16)], 'x87/IEEE128'), ('osx/arm64', [I(8)] * 8 + [I(2)], 'Apple sub-8 stack scalar'),
                          ('win/x86_64', [I(8)], 'Windows'), ('lnx/x86_64', [AGG(80, 8, memory=True)], 'extent'), ('osx/arm64', [AGG(16, 16)], '16-aligned composite')):
        try: plan(prof, ps, I(4), b'x'); raise AssertionError('accepted ' + why)
        except RuntimeError as e: out['refuse-' + why] = str(e)
    # record framing round trip
    rec, a = plan('lnx/x86_64', [I(4), F(8), UD], UD, b'USLSIG3\nproto')
    magic, ver, prof, flags, L, P, N, gp, fp, al, stack, M, R, hidden, scratch, crc = struct.unpack_from('<4sHBBIIBBBBHHBBHI', rec)
    assert magic == b'BNK1' and L == len(rec) and 32 + P + 8 * M + 4 * R == L and crc == zlib.crc32(rec[32:]) & 0xffffffff
    out['frame'] = dict(length=L, moves=M, results=R, gp=gp, fp=fp, al=al, stack=stack)
    return out

if __name__ == '__main__':
    r = selftest(); print(json.dumps(r, indent=1, ensure_ascii=False)); print('bankplan oracle: self-test passed', len(r), 'fixtures')
