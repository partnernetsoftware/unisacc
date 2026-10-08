# Stable reload launcher

Start a new foreground controlling-TTY session with:

```
python3 reload_launcher.py run SOURCE_APP COMPILER PRIVATE_CANDIDATE_ROOT PRIVATE_SESSION_DIR SESSION [VERIFIED_INITIAL_DIR]
```

Both private directories must already exist, be owned by the caller and mode0700. The host first opens supervisor.lock with NOFOLLOW/CLOEXEC, verifies an owned regular0600 file, then takes nonblocking POSIX flock exclusive ownership. All hosts sharing the directory conflict regardless of SESSION. It holds the lock through actor cleanup and final termios restoration attempt; descriptors are not inherited by actors/builds. No chmod, unlink or stale-PID cleanup occurs; process exit releases the kernel lock. A denied second host performs no event/TTY/build/spawn activity. This host flock is separate from native owner.lock record locks. Each session uses its own directory. Initial and subsequent actors are actual native binaries verified against the candidate source/compiler/artifact/gate receipt immediately before spawn. Optional initial directory is verified, not trusted by its name. HASH displayed by actors is this receipt binding, not cryptographic provenance authentication.

The supervisor stays alive with the same CTTY foreground process group. Only actors read user terminal input. It saves baseline termios and restores it on final exit with TCSAFLUSH, discarding residual unread keys. A private macOS probe showed TCSANOW alone adds kernel PENDIN on raw-to-canonical transition. /reload-code starts background candidate build and both existing local gates while the current actor continues editing. A successful candidate waits READY, then final idle FREEZE snapshots actual UI, RELEASED transfers ownership, COMMIT restores fixed journal/context, ACK precedes ACTIVATE, and RETIRE waits for old exit. Busy freeze rejects rather than forcing cancellation.

Control frames are bounded, strict JSON with op/session/handoff/hash and exact expected identities; stale ACK is not success. Unknown candidate ACK/failure requires candidate terminate and confirmed wait before RESUME. If candidate exit or old resume cannot be confirmed, BLOCKED is an error, never a success claim. Once new ACTIVATE was sent the launcher reports committed warnings rather than retrying the snapshot. Successful old retirement does not restore terminal state over the new actor. Build subprocess groups are cleaned on timeout (55s outer, existing14s inner); protocol steps8s, terminate/wait3s each. Runtime environment is inherited normally, not serialized into state or receipts; build gates use the existing private dummy-key environment.

This enables only newly launched managed sessions. Existing legacy windows are not automatically protected or upgraded. Native binaries and trusted private single-writer directories are required; this is a local launcher, not a deployment/publication mechanism.

Permanent private CTTY probe has separate success/rollback selectors. Focused host ownership/isolation coverage is `python3 probes/reload_launcher_lock.py RECEIPT.json`: real held/dead host processes, invalid locks, constructor/run cleanup, and an active verified native actor while another host with the same directory is rejected. Fault injection subclasses the test launcher to ignore an actual standby ACK; production has no fault environment or forged success receipt.
