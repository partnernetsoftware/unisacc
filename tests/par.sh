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
throttle() { while [ "$(jobs -rp | wc -l)" -ge "$PAR" ]; do sleep 0.1; done; }
