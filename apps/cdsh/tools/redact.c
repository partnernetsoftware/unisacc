/* redact.c — filter stdin, same shapes as the old tools/redact.sh.
 *   some-command 2>&1 | unisacc tools/redact.c
 * Matches values, not names: an s-k dash token, AKIA, gh*_ , xox*-, eyJ JWTs,
 * and assignments whose name ENDS with KEY/TOKEN/SECRET/PASSWORD/PASSWD/CREDENTIAL.
 */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int is_sk(int c) { return isalnum(c) || c == '_' || c == '-'; }
static int is_git(int c) { return isalnum(c); }
static int is_xox(int c) { return isalnum(c) || c == '-'; }
static int is_jwt(int c) { return isalnum(c) || c == '_' || c == '-'; }
static int is_val(int c) {
    return c && !isspace(c) && c != '"' && c != '\'';
}

static int ends_secret(const char *s, int n) {
    static const char *w[] = {
        "API_KEY", "APIKEY", "_KEY", "KEY", "TOKEN", "SECRET",
        "PASSWORD", "PASSWD", "CREDENTIAL", 0
    };
    int i;
    /* "KEY" must not fire on TOKENIZERS or on a longer word's middle.
     * The name must END with the word. "_KEY" covers MY_KEY; bare "KEY"
     * covers KEY itself. Prefer the longer suffix. */
    for (i = 0; w[i]; i++) {
        int L = (int)strlen(w[i]);
        if (n >= L && memcmp(s + n - L, w[i], (size_t)L) == 0) {
            if (strcmp(w[i], "KEY") == 0 && n > 3 && s[n - 4] != '_' && s[n - 4] != ' ')
                continue; /* TOKENIZERS has no KEY suffix; MYKEY is APIKEY */
            if (strcmp(w[i], "KEY") == 0) {
                if (n == 3) return 1;
                if (s[n - 4] == '_' || s[n - 4] == '.') return 1;
                continue;
            }
            return 1;
        }
    }
    return 0;
}

static void copy_span(char *dst, int *j, int cap, const char *s, int n) {
    int i;
    for (i = 0; i < n && *j + 1 < cap; i++) dst[(*j)++] = s[i];
}

static void redact_line(const char *in, char *out, int cap) {
    int i = 0, j = 0, n = (int)strlen(in);
    while (i < n && j + 1 < cap) {
        /* s, k, dash, then 8+ token chars */
        if (i + 11 < n && in[i] == 's' && in[i + 1] == 'k' && in[i + 2] == '-') {
            int k = i + 3, c = 0;
            while (k < n && is_sk(in[k])) { c++; k++; }
            if (c >= 8) {
                copy_span(out, &j, cap, "s" "k" "-" "<REDACTED>", 13);
                i = k; continue;
            }
        }
        /* AKIA + 12 uppercase/digits */
        if (i + 16 <= n && memcmp(in + i, "AKIA", 4) == 0) {
            int k = i + 4, c = 0;
            while (k < n && (isupper((unsigned char)in[k]) || isdigit((unsigned char)in[k]))) {
                c++; k++;
            }
            if (c >= 12) {
                copy_span(out, &j, cap, "AKIA<REDACTED>", 14);
                i = k; continue;
            }
        }
        /* gh[pousr]_ + 20 */
        if (i + 24 <= n && in[i] == 'g' && in[i + 1] == 'h' &&
            (in[i + 2] == 'p' || in[i + 2] == 'o' || in[i + 2] == 'u' ||
             in[i + 2] == 's' || in[i + 2] == 'r') && in[i + 3] == '_') {
            int k = i + 4, c = 0;
            while (k < n && is_git(in[k])) { c++; k++; }
            if (c >= 20) {
                copy_span(out, &j, cap, "gh<REDACTED>", 12);
                i = k; continue;
            }
        }
        /* xox[abposr]- */
        if (i + 15 <= n && in[i] == 'x' && in[i + 1] == 'o' && in[i + 2] == 'x' &&
            (in[i + 3] == 'a' || in[i + 3] == 'b' || in[i + 3] == 'p' ||
             in[i + 3] == 'o' || in[i + 3] == 's' || in[i + 3] == 'r') &&
            in[i + 4] == '-') {
            int k = i + 5, c = 0;
            while (k < n && is_xox(in[k])) { c++; k++; }
            if (c >= 10) {
                copy_span(out, &j, cap, "xox<REDACTED>", 13);
                i = k; continue;
            }
        }
        /* eyJ.... . .... jwt */
        if (i + 24 <= n && in[i] == 'e' && in[i + 1] == 'y' && in[i + 2] == 'J') {
            int k = i + 3, c = 0, dots = 0;
            while (k < n && (is_jwt(in[k]) || in[k] == '.')) {
                if (in[k] == '.') dots++;
                else c++;
                k++;
            }
            if (c >= 20 && dots >= 1) {
                copy_span(out, &j, cap, "eyJ<REDACTED-JWT>", 17);
                i = k; continue;
            }
        }
        /* NAME=value where name ends with a secret word */
        {
            int s = i;
            if (isalpha((unsigned char)in[i]) || in[i] == '_') {
                int k = i;
                while (k < n && (isalnum((unsigned char)in[k]) || in[k] == '_')) k++;
                if (k < n && in[k] == '=' && ends_secret(in + i, k - i)) {
                    int v = k + 1;
                    copy_span(out, &j, cap, in + i, k - i + 1);
                    while (v < n && is_val(in[v])) v++;
                    if (v > k + 1) {
                        copy_span(out, &j, cap, "<REDACTED>", 10);
                        i = v; continue;
                    }
                    j -= (k - i + 1); /* roll back if empty value; fall through */
                    if (j < 0) j = 0;
                }
                (void)s;
            }
        }
        out[j++] = in[i++];
    }
    out[j] = 0;
}

int main(void) {
    char in[8192], out[8192];
    while (fgets(in, sizeof in, stdin)) {
        redact_line(in, out, (int)sizeof out);
        fputs(out, stdout);
    }
    return 0;
}
