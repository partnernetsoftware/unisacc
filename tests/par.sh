# Sourced by the per-file suites.  Each file's work runs as a background job,
# PAR at a time, writing its results to files; the verdicts are then read back
# serially, in order, by the suite's own unchanged logic.
#
# all.sh already runs suites side by side, so the default is modest: the two
# levels multiply, and this machine has overheated under full load.
#
# A suite whose time is mostly WAITING sets PAR_WAIT before sourcing this: on
# macOS every freshly signed image waits ~0.5s on its first launch for the
# system's scan of new executables, with the CPU idle, so those overlap far
# beyond the core budget.  (Adding the terminal to System Settings > Privacy &
# Security > Developer Tools removes that scan altogether.)
PAR=${PAR:-${PAR_WAIT:-3}}
# bash sees its jobs inside $(...); dash (a Linux guest's sh) does not, and has
# no `jobs -r`, so there the throttle counts launches and waits for the batch
# (0.0.19: unthrottled compiles were OOM-killed on the 4 GiB guest).
_par_n=0
if [ -n "${BASH_VERSION:-}" ]; then
    throttle() { while [ "$(jobs -rp | wc -l)" -ge "$PAR" ]; do sleep 0.1; done; }
else
    throttle() { _par_n=$((_par_n + 1)); if [ "$_par_n" -gt "$PAR" ]; then wait; _par_n=1; fi; }
fi
