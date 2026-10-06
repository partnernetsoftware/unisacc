/*
 * term_api.h — term.c's public surface, declared once so other modules can use
 * it WITHOUT `#include "term.c"`. See render_api.h for the mistake that made
 * this necessary (including a .c pulled in a second `main`).
 *
 * NOTE the platform constraint recorded here rather than in prose: unisacc has
 * `signal()` but NOT `struct sigaction` (probes/tui-q5-sigaction.c stays red to
 * prove it). So the exit handler takes a plain function pointer and callers
 * must not assume sigaction semantics.
 */
#ifndef CDSH_TERM_API_H
#define CDSH_TERM_API_H

#include <stddef.h>
#include <termios.h>   /* struct termios appears in no signature here, but the
                        * probe questions are about it; kept out to avoid
                        * forcing every includer to pull it in — see .c file. */

#define TERM_FALLBACK_COLS 80
#define TERM_FALLBACK_ROWS 24
#define TERM_MIN_COLS      20
#define TERM_MIN_ROWS       4

typedef struct {
    int cols;
    int rows;
    int source;   /* 1 = from the tty, 0 = fallback */
} term_size_t;

typedef enum {
    TERM_KEY_NONE = 0,
    TERM_KEY_CTRL_C,
    TERM_KEY_CTRL_D,
    TERM_KEY_PASTE_ON,       /* bracketed paste start: ESC [ 200 ~ */
    TERM_KEY_PASTE_OFF,      /* bracketed paste end:   ESC [ 201 ~ */
    TERM_KEY_ENTER,
    TERM_KEY_BACKSPACE,
    TERM_KEY_UP,
    TERM_KEY_DOWN,
    TERM_KEY_RIGHT,
    TERM_KEY_LEFT,
    TERM_KEY_UNKNOWN,
    TERM_KEY_CHAR,
    TERM_KEY_MOUSE,         /* left press; x and y are 1-based cells */
    TERM_KEY_WHEEL          /* button 64 up (ch=2) or 65 down (ch=1) */
} term_key_kind;

typedef struct {
    term_key_kind kind;
    char          ch;       /* CHAR, or WHEEL direction: 2 up, 1 down */
    char          raw[16];  /* the bytes as read, always NUL-terminated */
    int           nraw;
    int           x, y;     /* valid when kind == TERM_KEY_MOUSE or WHEEL */
} term_key_t;

int         term_raw_enter(void);
int         term_raw_leave(void);
int         term_install_exit_handler(void (*fn)(int));
int         term_interrupted(void);
void        term_on_signal(int sig);
term_size_t term_size(void);
int         term_wait_readable(int ms);
term_key_t  term_parse_key(const char *buf, int got);
term_key_t  term_read_key(void);
int         term_read_keys(term_key_t *out, int cap);
int         term_run_selftest(void);

#endif
