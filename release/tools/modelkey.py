#!/usr/bin/env python3
"""modelkey ROOT TARGET -- the host model cache's real key and whether that cache is valid (0.0.38).

Uses exec/pipeline/models.py's own identity() and valid() read-only (the same key prepare() builds
under: checkout, target, network, compiler executable and the full construction closure), so a warm
marker can be bound to it instead of a weaker path or git-status signature.  Prints `KEY valid` or
`KEY invalid`; never builds."""
import os, pathlib, sys, tempfile
root, target = pathlib.Path(sys.argv[1]).resolve(), sys.argv[2]
sys.path.insert(0, str(root / 'exec/pipeline'))
import models
network, compiler = os.environ.get('NETWORK', '1'), os.environ.get('EXEC_CC', 'cc')
key = models.identity(target, network, compiler)
base = pathlib.Path(os.environ.get('UNISACC_MODEL_CACHE', tempfile.gettempdir() + '/unisacc-model-cache'))
print(key, 'valid' if models.valid(base / key, network) else 'invalid')
