#!/usr/bin/env python3
"""Hand-written recursive-descent postfix translator: the reference for the toy.
Same contract as exec: accept -> postfix on stdout, exit 0;
reject k -> 'reject k at i' on stderr, exit 1.
Codes: 1 = no F alternative, 2 = ')' expected, 3 = trailing input."""
import sys
sys.setrecursionlimit(100000)


class Reject(Exception):
    pass


def translate(x):
    i = 0
    out = []

    def b():
        return x[i] if i < len(x) else None

    def E():
        T(); Ep()

    def Ep():
        nonlocal i
        while b() == ord('+'):
            i += 1; T(); out.append('+')

    def T():
        F(); Tp()

    def Tp():
        nonlocal i
        while b() == ord('*'):
            i += 1; F(); out.append('*')

    def F():
        nonlocal i
        c = b()
        if c == ord('('):
            i += 1
            E()
            if b() != ord(')'):
                raise Reject(2, i)
            i += 1
        elif c is not None and ord('0') <= c <= ord('9'):
            out.append(chr(c)); i += 1
        else:
            raise Reject(1, i)

    E()
    if i != len(x):
        raise Reject(3, i)
    return ''.join(out)


if __name__ == '__main__':
    data = open(sys.argv[1], 'rb').read()
    try:
        sys.stdout.write(translate(data))
    except Reject as e:
        sys.stderr.write('reject %d at %d\n' % e.args)
        sys.exit(1)
