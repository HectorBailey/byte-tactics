#!/usr/bin/env bash
# Make a separate working copy for one issue, so several agents can work at
# once without touching each other's files:
#
#   tools/worktree.sh 12      # creates .worktrees/issue-12 on branch issue-12
#
# The compiler, the original exe and the Ghidra export are not in git, so they
# are linked in from the main checkout.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
N="${1:?usage: tools/worktree.sh <issue number>}"
N="${N#\#}"
DIR="$ROOT/.worktrees/issue-$N"

# Branch from this repository's main, whichever remote holds it: an outside
# contributor's clone can have their fork as origin and this repository (which
# the pull request is opened against) as upstream.
REMOTE="$(git -C "$ROOT" remote -v | awk '$3 == "(fetch)" && $2 ~ /HectorBailey\/byte-tactics/ { print $1; exit }')"
REMOTE="${REMOTE:-origin}"

if [ ! -d "$DIR" ]; then
    git -C "$ROOT" fetch -q "$REMOTE" main
    git -C "$ROOT" worktree add -q -b "issue-$N" "$DIR" "$REMOTE/main"
fi
ln -sfn "$ROOT/toolchain" "$DIR/toolchain"
mkdir -p "$DIR/orig" "$DIR/build"
ln -sf "$ROOT/orig/TotalA.exe" "$DIR/orig/TotalA.exe"
[ -d "$ROOT/build/ghidra" ] && ln -sfn "$ROOT/build/ghidra" "$DIR/build/ghidra"
echo "$DIR"
