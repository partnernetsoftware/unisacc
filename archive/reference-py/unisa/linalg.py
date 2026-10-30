"""The kernel primitives.  [K-1] [D-4]

Flat row-major list[float].  gemv accumulates in ascending index order and
nothing here may reorder it -- that is what makes results bit-stable. [D-4]
No softmax at inference: argmax only, ties to the lowest index. [D-2]
"""
import math


def gemv(x, W, b, n_in, n_out):
    """y[j] = b[j] + sum_i x[i]*W[i*n_out+j], i ascending. [D-4]"""
    y = b[:]
    for i in range(n_in):
        xi = x[i]
        base = i * n_out
        for j in range(n_out):
            y[j] += xi * W[base + j]
    return y


def gemv_bwd(x, W, dy, n_in, n_out, dW, db):
    """Accumulate dW/db, return dx."""
    for j in range(n_out):
        db[j] += dy[j]
    dx = [0.0] * n_in
    for i in range(n_in):
        xi = x[i]
        base = i * n_out
        s = 0.0
        for j in range(n_out):
            dyj = dy[j]
            dW[base + j] += xi * dyj
            s += W[base + j] * dyj
        dx[i] = s
    return dx


def relu(v):
    return [a if a > 0.0 else 0.0 for a in v]


def relu_bwd(dv, pre):
    return [dv[i] if pre[i] > 0.0 else 0.0 for i in range(len(dv))]


def argmax(v):
    """Ties break to the lowest class index. [D-2]"""
    bi = 0
    bv = v[0]
    for i in range(1, len(v)):
        if v[i] > bv:
            bv = v[i]
            bi = i
    return bi


def margin(v):
    """Top1 - Top2. Predicts the quantization ladder floor. [Q-9]"""
    a = b = -1e308
    for x in v:
        if x > a:
            b = a
            a = x
        elif x > b:
            b = x
    return a - b


def softmax_ce(logits, target):
    """Returns (loss, dlogits). Training only -- deploy has no softmax. [K-2]"""
    n = len(logits)
    m = logits[0]
    for i in range(1, n):
        if logits[i] > m:
            m = logits[i]
    ex = [0.0] * n
    s = 0.0
    for i in range(n):
        e = math.exp(logits[i] - m)
        ex[i] = e
        s += e
    inv = 1.0 / s
    loss = -math.log(ex[target] * inv + 1e-300)
    d = [0.0] * n
    for i in range(n):
        d[i] = ex[i] * inv
    d[target] -= 1.0
    return loss, d


class Adam:
    """beta1=0.9 beta2=0.999 eps=1e-8.  Fixed parameter order. [N/TR]"""
    __slots__ = ("b1", "b2", "eps", "t")

    def __init__(self, b1=0.9, b2=0.999, eps=1e-8):
        self.b1, self.b2, self.eps, self.t = b1, b2, eps, 0

    def step(self, tensors, lr):
        self.t += 1
        b1, b2, eps, t = self.b1, self.b2, self.eps, self.t
        c1 = 1.0 - b1 ** t
        c2 = 1.0 - b2 ** t
        for T in tensors:
            w, g, m, v = T.w, T.g, T.m, T.v
            for i in range(len(w)):
                gi = g[i]
                if gi == 0.0 and m[i] == 0.0 and v[i] == 0.0:
                    continue
                mi = m[i] = b1 * m[i] + (1.0 - b1) * gi
                vi = v[i] = b2 * v[i] + (1.0 - b2) * gi * gi
                w[i] -= lr * (mi / c1) / (math.sqrt(vi / c2) + eps)
                g[i] = 0.0
