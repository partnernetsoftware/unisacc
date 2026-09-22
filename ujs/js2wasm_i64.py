"""Legacy i64-only js2wasm (WAT). Prefer full js2wasm."""
from .emit_web import encode_i64_program
# Reuse previous module functions by importing from a saved copy — stub:
raise SystemExit("use full js2wasm (default); i64-only temporarily unavailable")
