#ifndef CSIH_AGENT_RESULT_H
#define CSIH_AGENT_RESULT_H
/* Declaration is distinct from terminal success; ok remains authoritative. */
enum { AGENT_OUTCOME_NONE=0, AGENT_OUTCOME_COMPLETED, AGENT_OUTCOME_PARTIAL,
       AGENT_OUTCOME_FAILED, AGENT_OUTCOME_UNVERIFIED };
enum { AGENT_ACCEPT_NONE=0, AGENT_ACCEPT_ACCEPTED, AGENT_ACCEPT_REJECTED, AGENT_ACCEPT_UNVERIFIED };
enum { AGENT_SCOPE_NONE=0, AGENT_SCOPE_WORK, AGENT_SCOPE_ANSWER_ONLY };
typedef struct {
    int   kind;
    int   outcome;
    int   claims_present; /* parser scratch copied by the single native turn owner */
    int   acceptance, scope, evidence_count;
    char  evidence[16][96];
    char  judgment_reason[320];
    char  cmd[4096];
    char  why[160];   /* one sentence shown for an exec; the command stays collapsed */
    char  op[64];
    char  path[1024];
    char  text[4096];
    char  old[4096];
    char  nw[4096];
    int   line;    /* 1-based start for file read; 0 means line 1 */
    int   nlines;  /* how many lines; 0 means the default window */
    int evidence_read; /* file read of this turn durable observation */
    int offset_set, max_bytes;
    long byte_offset; /* exact seek cursor, bounded before cast */
} agent_step;

typedef struct {
    int ok, stopped, rounds, actions, err, outcome, acceptance, scope;
    char answer[4096];
    char reason[320];
    char last[320];
} agent_result;
#endif
