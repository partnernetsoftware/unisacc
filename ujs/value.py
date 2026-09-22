"""UJS values. [V-1]"""
from __future__ import annotations

NULL = ("null", None)
# tags: null bool i64 f64 str list dict fn tup


def tag(v):
    return v[0]


def wrap_null():
    return ("null", None)


def wrap_bool(b):
    return ("bool", bool(b))


def wrap_i64(n):
    return ("i64", int(n))


def wrap_f64(x):
    return ("f64", float(x))


def wrap_str(s):
    return ("str", str(s))


def wrap_list(xs):
    return ("list", list(xs))


def wrap_dict(d):
    return ("dict", dict(d))


def wrap_fn(fn):
    return ("fn", fn)


def wrap_tup(xs):
    return ("tup", tuple(xs))


def keysig_of(d):
    """Map a dict's key set onto the finite KEYSIG vocab. [V-2]"""
    n = len(d)
    if n == 0:
        return "empty"
    if n == 1:
        return "k1"
    if n == 2:
        return "k2"
    if n == 3:
        return "k3"
    if n == 4:
        return "k4"
    return "open"


def ty_of(v):
    return v[0]


def from_py(x):
    if x is None:
        return wrap_null()
    if isinstance(x, bool):
        return wrap_bool(x)
    if isinstance(x, int):
        return wrap_i64(x)
    if isinstance(x, float):
        return wrap_f64(x)
    if isinstance(x, str):
        return wrap_str(x)
    if isinstance(x, list):
        return wrap_list(from_py(i) for i in x)
    if isinstance(x, tuple):
        return wrap_tup(from_py(i) for i in x)
    if isinstance(x, dict):
        return wrap_dict((str(k), from_py(v)) for k, v in x.items())
    raise TypeError("unsupported host value: %r" % (type(x),))


def to_py(v):
    t, x = v
    if t == "null":
        return None
    if t in ("bool", "i64", "f64", "str"):
        return x
    if t == "list":
        return [to_py(i) for i in x]
    if t == "tup":
        return tuple(to_py(i) for i in x)
    if t == "dict":
        return {k: to_py(val) for k, val in x.items()}
    if t == "fn":
        return x  # opaque
    raise TypeError(t)


def truthy(v):
    t, x = v
    if t == "null":
        return False
    if t == "bool":
        return x
    if t in ("i64", "f64"):
        return x != 0
    if t == "str":
        return len(x) > 0
    if t in ("list", "tup"):
        return len(x) > 0
    if t == "dict":
        return len(x) > 0
    if t == "fn":
        return True
    return False
