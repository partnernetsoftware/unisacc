#!/usr/bin/env python3
"""Regression: CURSOR_* drift in the agent shell must not change gatequeue stamp identity when env-pin is loaded."""
import hashlib, json, os, pathlib, sys, tempfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tests'))
import env_pin
import importlib.util

def load_gatequeue():
    spec = importlib.util.spec_from_file_location('gq', ROOT / 'tests/gatequeue.py')
    gq = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gq)
    return gq

def main():
    base = {'PATH': '/usr/bin', 'HOME': '/tmp/home', 'LANG': 'C.UTF-8'}
    pin_a = env_pin.collect(base)
    pin_b = env_pin.collect({**base, 'CURSOR_AGENT': '1', 'CURSOR_BC_ID': 'deadbeef', 'AGENT_MODE': 'cloud'})
    sha_a = env_pin.sha256_of_vars(pin_a)
    sha_b = env_pin.sha256_of_vars(pin_b)
    if sha_a != sha_b:
        print('envpin: filtered pin changed when only CURSOR_/AGENT_ vars differ')
        return 1

    gq = load_gatequeue()
    with tempfile.TemporaryDirectory() as td:
        state = pathlib.Path(td) / 'q'
        state.mkdir()
        env_pin.write_atomic(state / 'env-pin.json', base)
        gq.init_env_pin(state)
        jobs = {'probe': ['true']}
        stamp_pin = gq.fingerprint(jobs)

        gq._ENV_PIN = None
        os.environ['CURSOR_PIN_TEST'] = 'noise'
        gq.init_env_pin(state)
        if gq.fingerprint(jobs) != stamp_pin:
            print('envpin: stamp changed after CURSOR_* in live shell with same env-pin')
            return 1

    print('envpin  same pin sha with CURSOR_/AGENT_ drift; loaded pin keeps fingerprint across agent shell noise')
    return 0

if __name__ == '__main__':
    sys.exit(main())
