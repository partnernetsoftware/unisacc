#!/usr/bin/env python3
"""copy-contract (thin wrapper): the copy and mode families of tests/cpparity.py over tests/cpparity/cases.tsv.

Kept as the gate's name for copy/copy-state/rule:MODE C/P parity; the table and the runner are in cpparity.py.
"""
import subprocess, sys, pathlib

HERE = pathlib.Path(__file__).resolve().parent
sys.exit(subprocess.call([sys.executable, str(HERE / 'cpparity.py'), '--family', 'copy,mode']))
