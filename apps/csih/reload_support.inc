/* Private codec aggregation; no CLI and no new argv source slot. */
#include "reload_state.c"
#include "reload_session_decode.c"
#include "reload_session_encode.c"
#include "reload_io.c"
#include "reload_load.c"
#include "reload_consume.c"
/* These two libraries each have a private set_why. */
#define set_why checkpoint_set_why
#include "journal_checkpoint.c"
#undef set_why
#include "reload_owner.c"
#include "csih_message.c"
#include "csih_message_io.c"
