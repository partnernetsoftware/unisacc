#!/usr/bin/env python3
"""Durable build-environment pin for gatequeue stamp identity (c41 env-pin).

Only variables that affect compilation, linking, or gate tooling enter the pin.
Ephemeral agent-shell noise (CURSOR_*, AGENT_*, etc.) is excluded so resume from
another shell does not void stamps."""
import hashlib, json, os, pathlib, stat

ENV_PIN_VERSION = 1

# Same knobs as gatequeue.execution_settings(); always pinned when set.
SETTINGS_KEYS = frozenset((
    'MODEL_COM', 'UA', 'UA_RUN', 'TOOLS_UA', 'CORPUS_UA', 'CC', 'CFLAGS', 'TARGET', 'DRIVE', 'NETWORK',
    'EXEC_CC', 'PAR', 'STRICT', 'SHARD', 'CHAINKEEP', 'E3KEEP', 'E4STRICT',
    'UNISA_MAXSTEPS', 'UNISA_CONTAINER', 'UNISA_KERNEL', 'UNISACC_FFI_PROVIDER', 'UNISACC_FFI_X86_PROVIDER',
    'SEED_DIR', 'GATE_STATE', 'RELEASE_JOBS', 'REALPROG_CACHE',
    'E2_AUTOINC', 'E2REF', 'E2NOAUTO',
))

_DENY_EXACT = frozenset((
    'OLDPWD', 'TERM_SESSION_ID', '_', 'SHLVL', 'PWD',
    'K2B_FORCE_TREE',   # stripped at gatequeue import; never an identity
))

_DENY_PREFIXES = (
    'CURSOR_', 'AGENT_', 'VSCODE_', 'GIT_ASKPASS', 'SSH_ASKPASS',
)

_ALLOW_EXACT = frozenset((
    'PATH', 'HOME', 'TMPDIR', 'TEMP', 'TMP', 'USER', 'LOGNAME', 'SHELL', 'HOSTNAME',
    'LANG', 'LC_ALL', 'LC_CTYPE', 'LC_MESSAGES', 'LC_NUMERIC', 'LC_COLLATE',
    'CC', 'CXX', 'CFLAGS', 'CXXFLAGS', 'LDFLAGS', 'CPPFLAGS', 'PKG_CONFIG_PATH',
    'LD_LIBRARY_PATH', 'DYLD_LIBRARY_PATH', 'LIBRARY_PATH', 'CPATH', 'C_INCLUDE_PATH',
    'CSMITH', 'CSMITH_INCLUDE', 'LLVM_BIN',
    'PYTHONPATH', 'PYTHONUNBUFFERED', 'PYTHONHOME', 'PYTHONNOUSERSITE',
    'TERM', 'TERM_SH', 'TERM_SH_NOFALLBACK',
    'TZ', 'SSL_CERT_FILE', 'SSL_CERT_DIR',
    'XDG_RUNTIME_DIR', 'XDG_CACHE_HOME',
))

_ALLOW_PREFIXES = (
    'UNISACC_', 'UNISA_', 'E2', 'LIMA_', 'UTM_',
)


def _want_key(key):
    if key in _DENY_EXACT:
        return False
    if any(key.startswith(p) for p in _DENY_PREFIXES):
        return False
    if key in SETTINGS_KEYS or key in _ALLOW_EXACT:
        return True
    return any(key.startswith(p) for p in _ALLOW_PREFIXES)


def collect(environ=None):
    env = environ if environ is not None else os.environ
    return {k: env[k] for k in sorted(env) if _want_key(k)}


def canonical_body(vars_dict):
    return json.dumps({'version': ENV_PIN_VERSION, 'vars': vars_dict}, sort_keys=True) + '\n'


def sha256_of_vars(vars_dict):
    return hashlib.sha256(canonical_body(vars_dict).encode()).hexdigest()


def package(vars_dict):
    sha = sha256_of_vars(vars_dict)
    return {'version': ENV_PIN_VERSION, 'sha256': sha, 'vars': vars_dict}


def write_atomic(path, environ=None):
    """Write env-pin.json next to GATE_STATE; return content sha256."""
    vars_dict = collect(environ)
    obj = package(vars_dict)
    p = pathlib.Path(path)
    tmp = p.with_suffix(p.suffix + '.tmp')
    tmp.write_text(json.dumps(obj, indent=2, sort_keys=True) + '\n')
    os.chmod(tmp, stat.S_IRUSR | stat.S_IWUSR)
    tmp.replace(p)
    return obj['sha256']


def load(path):
    """Load and verify pin; return vars dict."""
    p = pathlib.Path(path)
    try:
        obj = json.loads(p.read_text())
    except (OSError, ValueError) as e:
        raise ValueError('env-pin.json unreadable (%s)' % type(e).__name__)
    if not isinstance(obj, dict) or obj.get('version') != ENV_PIN_VERSION:
        raise ValueError('env-pin.json unsupported version')
    vars_dict = obj.get('vars')
    if not isinstance(vars_dict, dict):
        raise ValueError('env-pin.json vars is not a map')
    if sha256_of_vars(vars_dict) != obj.get('sha256'):
        raise ValueError('env-pin.json sha256 mismatch')
    return vars_dict
