/* E4 summary regression input.
 *
 * These C paths exercise read/write order, call plus continuation,
 * conditional branches, labels, and cyclic control flow.  The e4 checker
 * compares their complete optimized tape with the independent classic C
 * implementation, including functions not called by main.
 *
 * C lowering chooses physical registers and block order: this fixture
 * does not assert exact RMM/WMM masks, unknown direct jump targets, or the
 * solver's exact 63/64-round boundary.  Those need separate tape probes.
 */

int summary_write_first(int x)
{
    int v;
    v = x + 17;
    v = 23;
    return v;
}

int summary_read_before_write(int x)
{
    int v;
    v = x + 17;
    v = v + 23;
    return v;
}

int summary_no_arguments(void)
{
    return 7;
}

int summary_six_arguments(int a, int b, int c, int d, int e, int f)
{
    return a + b + c + d + e + f;
}

/* The callee uses argument registers, before a dead continuation. */
int summary_call_target_reads(int x)
{
    int v;
    v = summary_six_arguments(x, 2, 3, 4, 5, 6);
    return v + 1;
}

/* A no-argument callee and a continuation that still uses the old value. */
int summary_call_continuation_reads(int x)
{
    return (x + 11) + summary_no_arguments() + (x + 13);
}

int summary_call_then_write(int x)
{
    int v;
    v = (x + 11) + summary_no_arguments();
    v = 31;
    return v;
}

/* The target is a function-pointer parameter, not a named tape label.
 * A real definition is provided so every translation unit remains linkable.
 */
int summary_indirect_target(int x)
{
    return x + 9;
}

int summary_unknown_call(int (*callee)(int), int x)
{
    return (x + 3) + callee(x) + (x + 5);
}

/* Read in the branch condition, and write on either successor. */
int summary_condition_reads(int x)
{
    int v;
    v = x + 3;
    if (v)
        v = 17;
    else
        v = 19;
    return v;
}

/* One successor reads the prior value, the other overwrites it. */
int summary_target_reads(int x, int choose)
{
    int v;
    v = x + 3;
    if (choose)
        goto read_target;
    v = 29;
    goto done;
read_target:
    v = v + 7;
done:
    return v;
}

int summary_fallthrough_reads(int x, int choose)
{
    int v;
    v = x + 3;
    if (choose)
        goto write_target;
    v = v + 11;
    goto done;
write_target:
    v = 37;
done:
    return v;
}

int summary_both_paths_write(int x, int choose)
{
    int v;
    v = x + 3;
    if (choose)
        v = 41;
    else
        v = 43;
    return v;
}

/* The jump bypasses a read; consecutive labels share a destination. */
int summary_jump_skips_read(int x)
{
    int v;
    v = x + 3;
    goto write_target;
skipped_read:
    v = v + 101;
    goto done;
write_target:
alias_target:
    v = 47;
done:
    return v;
}

int summary_consecutive_labels(int x, int choose)
{
    int v;
    v = x + 3;
    if (choose)
        goto first;
    goto second;
first:
second:
third:
    return v + 5;
}

/* Finite cycles: one repeatedly overwrites, one reads before writing. */
int summary_loop_writes(int x)
{
    int n;
    int v;
    n = 3;
    v = x + 3;
again:
    v = 53;
    n = n - 1;
    if (n)
        goto again;
    return v;
}

int summary_loop_reads(int x)
{
    int n;
    int v;
    n = 3;
    v = x;
again:
    v = v + n;
    n = n - 1;
    if (n)
        goto again;
    return v;
}

int summary_recursive_read(int n)
{
    if (n <= 0)
        return 1;
    return n + summary_recursive_read(n - 1);
}

int summary_recursive_write(int n)
{
    int v;
    if (n <= 0)
        return 59;
    v = summary_recursive_write(n - 1);
    v = 61;
    return v;
}

int main(void)
{
    if (summary_write_first(2) != 23) return 1;
    if (summary_read_before_write(2) != 42) return 2;
    if (summary_call_target_reads(1) != 22) return 3;
    if (summary_call_continuation_reads(2) != 35) return 4;
    if (summary_call_then_write(2) != 31) return 5;
    if (summary_unknown_call(summary_indirect_target, 2) != 23) return 6;
    if (summary_condition_reads(-3) != 19) return 7;
    if (summary_condition_reads(1) != 17) return 8;
    if (summary_target_reads(2, 1) != 12) return 9;
    if (summary_target_reads(2, 0) != 29) return 10;
    if (summary_fallthrough_reads(2, 0) != 16) return 11;
    if (summary_fallthrough_reads(2, 1) != 37) return 12;
    if (summary_both_paths_write(2, 1) != 41) return 13;
    if (summary_both_paths_write(2, 0) != 43) return 14;
    if (summary_jump_skips_read(2) != 47) return 15;
    if (summary_consecutive_labels(2, 0) != 10) return 16;
    if (summary_consecutive_labels(2, 1) != 10) return 17;
    if (summary_loop_writes(2) != 53) return 18;
    if (summary_loop_reads(2) != 8) return 19;
    if (summary_recursive_read(3) != 7) return 20;
    if (summary_recursive_write(3) != 61) return 21;
    return 0;
}

