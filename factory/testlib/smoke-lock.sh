# One host smoke at a time, including nested make/script invocations.
# FD 9 survives fork/exec so an orphan emulator retains the lock until exit.
# A private path argument is for the host regression only; production callers
# use the same per-user path from every worktree. Queueing is outside guest
# deadlines. Never unlink a lock file: waiters must share the same inode.
smoke_lock() {
 smoke_lock_path=${1:-/tmp/initech-smoke-$(id -u).lock}
 if [ "$(readlink /proc/$$/fd/9 2>/dev/null || :)" = "$smoke_lock_path" ]; then
  return 0
 fi
 exec 9<>"$smoke_lock_path"
 if ! flock -n 9; then
  echo "smoke: waiting for host lock $smoke_lock_path (pid=$$, worktree=$PWD)" >&2
  flock -w "${SMOKE_LOCK_WAIT_S:-1200}" 9 || {
   echo "FAIL smoke: host lock unavailable: $smoke_lock_path" >&2
   return 1
  }
 fi
 echo "smoke: acquired host lock $smoke_lock_path (pid=$$, worktree=$PWD)" >&2
}
