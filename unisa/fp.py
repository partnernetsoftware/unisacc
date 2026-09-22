"""Floating point on the tape. [TP]

A floating value is its IEEE-754 bit pattern in a general register: binary64
for double, binary32 in the low 32 bits for float.  The tape's calling
convention is ours on both ends of every call, so nothing has to move into a
floating register except inside these ops -- which is also why this module
is the ONE definition of what they mean, shared by the reference VM and the
target interpreter.

Every result is what the hardware gives (SSE on x86_64, the FP unit on
AArch64), round-to-nearest-even:
- float arithmetic is done in double and rounded once to float.  That is
  exact for + - * / and sqrt: a double holds more than 2*24+2 bits, so the
  double result rounds to the same float as the infinitely precise one.
- int -> float of a value past 2^53 is rounded straight to 24 bits, not via
  a double, which would round twice.
"""
import math
import struct

M64 = (1 << 64) - 1
M32 = (1 << 32) - 1

OPS3 = ("fadd64", "fsub64", "fmul64", "fdiv64", "flt64", "fle64", "feq64",
        "fadd32", "fsub32", "fmul32", "fdiv32", "flt32", "fle32", "feq32")
OPS2 = ("cvtid", "cvtud", "cvtis", "cvtus", "cvtdi", "cvtdu", "cvtsd",
        "cvtds", "fsqrt64", "fsqrt32")
OPS = OPS3 + OPS2


def d(x):
    """bits -> double"""
    return struct.unpack("<d", struct.pack("<Q", x & M64))[0]


def bd(f):
    """double -> bits"""
    return struct.unpack("<Q", struct.pack("<d", f))[0]


def s(x):
    """low 32 bits -> float (as a Python double, exactly)"""
    return struct.unpack("<f", struct.pack("<I", x & M32))[0]


def bs(f):
    """double -> float bits, rounded to nearest even; past FLT_MAX is inf"""
    if math.isnan(f):
        return 0x7FC00000 | (0x80000000 if math.copysign(1, f) < 0 else 0)
    try:
        return struct.unpack("<I", struct.pack("<f", f))[0]
    except OverflowError:
        return 0xFF800000 if f < 0 else 0x7F800000


def _div(a, b):
    if b != 0.0:
        return a / b
    if a != a or a == 0.0:
        return math.nan
    neg = (math.copysign(1, a) < 0) != (math.copysign(1, b) < 0)
    return -math.inf if neg else math.inf


def _sqrt(a):
    if a != a or a < 0:
        return math.nan
    return math.sqrt(a)


def _i2f32(v):
    """a Python int, rounded once to a float's 24-bit significand"""
    if abs(v) < (1 << 53):
        return bs(float(v))
    neg, v = v < 0, abs(v)
    sh = v.bit_length() - 24
    q, rem = v >> sh, v & ((1 << sh) - 1)
    half = 1 << (sh - 1)
    if rem > half or (rem == half and q & 1):
        q += 1
    f = float(q << sh)                  # at most 25 significant bits: exact
    return bs(-f if neg else f)


def _f2int(f, unsigned):
    """C's truncation toward zero; the value when it does not fit is
    undefined in C, so this is what AArch64 does: saturate, NaN -> 0."""
    if f != f:
        return 0
    lo, hi = (0, M64) if unsigned else (-(1 << 63), (1 << 63) - 1)
    if f == math.inf or f >= hi + 1:
        return hi & M64
    if f == -math.inf or f <= lo - 1:
        return lo & M64
    return int(f) & M64


def _s64(x):
    return x - (1 << 64) if x >> 63 else x


def op3(op, x, y):
    if op.endswith("64"):
        a, b, pack = d(x), d(y), bd
    else:
        a, b, pack = s(x), s(y), bs
    k = op[:-2]
    if k == "fadd":
        return pack(a + b)
    if k == "fsub":
        return pack(a - b)
    if k == "fmul":
        return pack(a * b)
    if k == "fdiv":
        return pack(_div(a, b))
    if k == "flt":
        return 1 if a < b else 0
    if k == "fle":
        return 1 if a <= b else 0
    return 1 if a == b else 0            # feq: NaN is unequal to itself


def op2(op, x):
    if op == "cvtid":
        return bd(float(_s64(x)))
    if op == "cvtud":
        return bd(float(x & M64))
    if op == "cvtis":
        return _i2f32(_s64(x))
    if op == "cvtus":
        return _i2f32(x & M64)
    if op == "cvtdi":
        return _f2int(d(x), False)
    if op == "cvtdu":
        return _f2int(d(x), True)
    if op == "cvtsd":
        return bd(s(x))
    if op == "cvtds":
        return bs(d(x))
    if op == "fsqrt64":
        return bd(_sqrt(d(x)))
    return bs(_sqrt(s(x)))               # fsqrt32


def dec_to_f32(text):
    """A decimal floating constant, rounded ONCE to binary32 (nearest, ties to
    even).  float(text) and then to float would round twice, and for a
    constant near a halfway point between two floats the two answers
    differ."""
    from fractions import Fraction
    v = Fraction(text)
    if v == 0:
        return 0
    neg, v = v < 0, abs(v)
    # binary exponent e with 2^e <= v < 2^(e+1)
    e = v.numerator.bit_length() - v.denominator.bit_length()
    if Fraction(2) ** e > v:
        e -= 1
    e = max(e, -126)                    # below that the float is subnormal
    q = v / Fraction(2) ** (e - 23)     # the significand, scaled to 24 bits
    n = q.numerator // q.denominator
    rem = q - n
    if rem > Fraction(1, 2) or (rem == Fraction(1, 2) and n & 1):
        n += 1
    f = float(Fraction(n) * Fraction(2) ** (e - 23))   # exact: <= 25 bits
    return bs(-f if neg else f)
