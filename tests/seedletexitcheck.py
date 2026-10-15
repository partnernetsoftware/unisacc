#!/usr/bin/env python3
"""seedgen-let-exit (thin wrapper): the letexit family of tests/cpparity.py over tests/cpparity/cases.tsv.

Kept as the gate's name for the generic constructor exit before write (hit/miss, NUL repr); the table is in cpparity.py.
"""
import subprocess, sys, pathlib

HERE = pathlib.Path(__file__).resolve().parent
sys.exit(subprocess.call([sys.executable, str(HERE / 'cpparity.py'), '--family', 'letexit']))
