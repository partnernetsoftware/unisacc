"""Single source of the csih file table for probes (mirrors csih.sh / check.sh).

WHY: probes used to hand-copy source lists (json.c, csih.c, reload_support.inc) that
drifted from csih.sh. Import these instead. Global headers are injected with -include;
no unit textually includes another unit's public header.
"""
import pathlib

APP = pathlib.Path(__file__).resolve().parents[1]

# TUI entry (tui.c has main). Same order as csih.sh.
TUI = ("tui.c render.c term.c chat.c clock.c tools.c cols.cx home.cx file.c shell.c edit.c gate.c "
       "json.cx session.c agent.c plugin.c net.c reload_state.c reload_session_decode.c "
       "reload_session_encode.c reload_io.c reload_load.c reload_consume.c journal_checkpoint.c "
       "reload_owner.c csih_message.c csih_message_io.c context_index.c").split()

# agent CLI (no TUI). home.cx is required: agent calls csih_home_bind.
AGENT = "agent.c agent_cli.c cols.cx home.cx file.c edit.c shell.c json.cx session.c net.c plugin.c".split()

# -include flags, absolute paths (quoted includes inside a -include'd header resolve against the forward file).
GLOBAL_INCLUDES = ["-include", str(APP / "csih_cols.h"), "-include", str(APP / "csih_home.h"),
                   "-include", str(APP / "json.h")]
