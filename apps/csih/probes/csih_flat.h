/* csih_flat.h — C port of probes/_flat.py (the shared csih file table).
 * Source of truth for probe builds; mirrors csih.sh. Do not hand-edit lists
 * without updating csih.sh. Root is fixed to the known repo path.
 *
 * Use: #include "csih_flat.h" in a probe. Build a sources array by iterating
 * csih_tui / csih_agent under CSIH_APP. Pass csih_global_include_args to unisacc
 * as the -include flags (absolute paths; csih_message_io.h is never -included).
 */
#ifndef CSIH_FLAT_H
#define CSIH_FLAT_H

#define CSIH_ROOT "/Users/wjc/repos/unisacc"
#define CSIH_APP  CSIH_ROOT "/apps/csih"

/* TUI entry (tui.c has main). Same order as csih.sh. */
static const char *csih_tui[] = {
    "tui.c", "render.c", "term.c", "chat.c", "clock.c", "tools.c", "cols.cx", "home.cx", "file.c", "shell.c", "edit.c", "gate.c",
    "json.cx", "session.c", "agent.c", "plugin.c", "net.c", "reload_state.c", "reload_session_decode.c",
    "reload_session_encode.c", "reload_io.c", "reload_load.c", "reload_consume.c", "journal_checkpoint.c",
    "reload_owner.c", "csih_message.c", "csih_message_io.c", "context_index.c", NULL
};

/* agent CLI (no TUI). home.cx is required: agent calls csih_home_bind. */
static const char *csih_agent[] = {
    "agent.c", "agent_cli.c", "cols.cx", "home.cx", "file.c", "edit.c", "shell.c", "json.cx", "session.c", "net.c", "plugin.c", NULL
};

/* -include flags, absolute paths. Flattened pairs: "-include", path, ... */
static const char *csih_global_include_args[] = {
    "-include", CSIH_APP "/csih_cols.h",
    "-include", CSIH_APP "/csih_home.h",
    "-include", CSIH_APP "/json.h",
    NULL
};

#endif
