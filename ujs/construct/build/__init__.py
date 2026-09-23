"""Asset builders (not needed for normal run/wasm_run)."""
from .browser import emit as emit_browser_tables
from .ic_c import emit as emit_ic_net_c
from .web import web_build

__all__ = ["web_build", "emit_browser_tables", "emit_ic_net_c"]
