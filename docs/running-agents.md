# Running agents on the project

Work is handed out as GitHub issues labelled `decomp`, each listing a few
functions. Any coding agent can take part: it reads `AGENTS.md` (OpenCode and
Codex both load it automatically), claims an unassigned issue, works in its own
copy of the repository and opens a pull request. The orchestrator re-checks the
pull request, merges it, records which model did what, and opens new issues.

## One-time setup

Everything runs from the main checkout, `~/repos/personal/byte-tactics`, which
must already have the toolchain (`tools/setup_toolchain.sh`), the original exe
in `orig/` and the Ghidra export in `build/ghidra/`. The agent also needs the
GitHub CLI logged in (`gh auth status`), because it claims issues, pushes
branches and opens pull requests with it.

Agents create their working copies in `.worktrees/issue-<N>` (ignored by git),
linked to the main checkout's toolchain, so several can run at once from one
machine.

## OpenCode

```sh
cd ~/repos/personal/byte-tactics
opencode
```

OpenCode runs as a lead plus cheap workers:

- **The lead** is the model you pick with `/models`, ideally GLM-5.3 or
  Grok 4.7. It claims the issue, sets up the worktree, reviews the results,
  retries what the workers left and opens the pull request.
- **The workers** are the `decomp-worker` subagent defined in
  `.opencode/agents/decomp-worker.md`. The file sets no model, so they run on
  whatever model the session was started with (OpenCode gives a subagent
  without a `model:` line its caller's model). They do the first attempt at
  each function, one worker per function, all at once.
- **When workers stop:** a worker keeps going while its best score improves
  and stops when it has not improved in 30 check runs or 60 minutes (the rule
  in `AGENTS.md`). The file's `steps` (400) is only a safety net: a worker
  that runs out of steps while still improving says so, and the session
  starts a fresh one from its file.
- **Free sessions:** start a session on a free model such as Space Bunny Free
  (`opencode/space-bunny-free`, see `opencode models | grep free`) and its
  workers run on it too, so it costs nothing and you can run as many as you
  like. Record its results under its own name (`record.py <issue>
  space-bunny-free ...`) and send its leftovers back to the ordinary queue with
  `--escalate retry`, for DeepSeek to try next.

To run the workers on a different model from the lead, add a `model:` line to
that file. Your OpenCode Go plan limits spending per model: DeepSeek V4.1 Flash and Kimi K3
have lower caps than GLM-5.3 and Grok 4.7, and each cap applies per 5 hours,
per week and per month.

Give the lead this prompt:

> Follow AGENTS.md: pick up the lowest-numbered unassigned `decomp` issue,
> hand its functions to decomp-worker subagents, retry what they leave, and
> open a pull request. Then pick up the next one, until none are left.

Every model may take any issue, including those labelled `hard` (functions
over 1000 bytes). To steer a model, add a size label to the prompt, for
example "only take `size:medium` issues".

For an unattended run, `opencode run` takes the same prompt on the command
line, with `-m opencode-go/glm-5.3` to choose the lead (check
`opencode run --help`). Run several in separate terminals; each claims a
different issue.

OpenCode asks before running shell commands unless you allow them. The agent
needs to run `uv`, `gh`, `git` and the compiler (Wine) freely, so allow those
for this project.

## Codex

```sh
cd ~/repos/personal/byte-tactics
codex
```

Choose a model with `/model` and give it this prompt:

> Follow AGENTS.md: pick up the lowest-numbered unassigned `decomp` issue,
> decompile it and open a pull request. Then pick up the next one, until none
> are left.

To point it at the biggest functions, add "labelled `hard`" to the prompt.

Codex usage goes quickly on long decompilation sessions. To make it last:

- **One Codex session at a time.**
- **Lower reasoning effort.** Set `model_reasoning_effort = "medium"` in
  `~/.codex/config.toml`, or pass `-c model_reasoning_effort="medium"`.
  Raise it again only for a stubborn near-miss.

For an unattended run, `codex exec "<prompt>"`.

Codex runs commands in a sandbox. The agent needs network access (for `gh`
and `git push`) and needs to run Wine. If either is blocked, start Codex with
`--sandbox danger-full-access` (check `codex --help` for the current flag
names). This is your own repository and machine, and the agent only needs the
repository folder.

## Which model

Last night's calibration (`docs/agents.md`) covered Claude models only. Opus
matched 99-100% of functions up to 160 bytes and about 80% at 161-260 bytes.
What's left is mostly the harder, larger functions.

- **Codex, GPT-6 Astra.** OpenAI reports it solves 88% of a
  binary reverse-engineering benchmark first time. That is not the same task
  as matching decompilation, but it makes Astra a strong candidate for the
  `hard` issues (functions over 1000 bytes).
- **OpenCode.** There is no track record here for any of these models. The
  likeliest candidates are the larger, non-Flash ones (GPT-6 Luna, Kimi K3,
  GLM-5.3, Qwen3.8 Max, MiMo-V2.6-Pro, Grok 4.7). Give two or three of them a
  `size:medium` issue each and compare.
- **Measuring.** Every pull request names its model, and `tools/record.py`
  logs the orchestrator's re-check under that name. `docs/agents.md` then
  shows each model's match rate by function size. Use those numbers, not
  reputation, to decide who gets which issues. If a plan is flat-rate rather
  than per token, match rate is the number that matters.

## Grouping retries by family

`tools/record.py --escalate retry` builds its issue from the functions **this
pull request** left unmatched. That is the right set to escalate, but it is the
wrong set to *group* by, because near-copies of the same function are usually
spread over several pull requests.

Two cases from 2026-10-02, both in #4609 to #4751:

- The 18 `vector<int>::insert` copies came out as four separate issues. Two
  workers were given 0x425210 and 0x46e640 at the same time, both at 99.6%, both
  stuck on one swapped SIB byte, and each spent a full pass reaching the same
  conclusion: that a two-register `lea` ignores source operand order, so the
  residual is not reachable by respelling the source. That conclusion is worth
  having once. It was paid for twice.
- The reverse also happens. #4746 grouped 0x438ea0 with 0x4c3e40, which share
  no code and no class (`tools/unitmap.py --at` reports `no unit` for both), so
  the issue gave one worker two unrelated cold starts and put the two functions
  out of reach of anyone who wanted just one of them.

Grouping does not guarantee a win. #4381 is the counterexample: the shape that
took 0x4c1000 from 91.4% to 97.1% made 0x4c0820 *worse*, 93.6% against 99.5%,
because the function-scope `b` already owns the stack slot the fix was trying to
free. Grouping makes that visible sooner, which is the point, but a shared
method name is a hint that shapes resemble, not a promise.

### Finding the family

The method name in `data/symbols.csv` is enough, and it groups cleanly:

```
     19  vector::_Destroy
     18  vector::insert
     13  vector::_Ucopy
     12  vector::size
     10  vector::_Ufill
      8  vector::erase
```

Take the method after `?$vector::`, or the last `::`/`.` component, and keep the
functions that are still `kind == game` in `data/functions.csv`. Where the class
prefix carries the element type (`PAVClass_004c2ea0`, `VClass_00489260`),
separate `vector<int>::insert` from `vector<Unit>::insert`; where it does not,
the method-name group is still better than nothing.

```sh
# after record.py --escalate retry, for anything it just retried
uv run tools/issues.py --addresses <family members still unmatched> \
    --title "Retry: <n> <method> functions" --label near-miss --open \
    --note "Same <method> shape as #<issue>. A fix in one is likely to transfer, \
though not guaranteed: check which local already owns the stack slot before \
copying a sibling's shape. Read the siblings' files under src/unsorted/ first."
```

`tools/issues.py` already refuses to hand out a function that is in an open
issue, listed in `data/attempts.csv` or annotated under `src/`, so the extra
addresses cannot collide with an issue that already exists. The note matters:
agents cannot see the family from inside their own worktree, because
`tools/unitmap.py` reports `no unit` for every one of these unmatched functions.

## The orchestrator's loop

Run from the main checkout, on `main`:

1. **Keep issues open.** Open a handful per size:
   - `uv run tools/issues.py --band medium --count 6`
   - `uv run tools/issues.py --band large --count 6`

   Near-misses to retry:
   - `uv run tools/issues.py --addresses ... --title "Near-misses" --label near-miss --escalation`
2. **Review each pull request.**
   - `tools/review.sh <PR>` checks the pull request out in `.worktrees/pr-<PR>`
     and merges `origin/main` into it. It lists the changed files, rebuilds
     `data/symbols.csv` as the real merge will, re-checks every function in
     the pull request, lists any function that matches on `main` but would
     stop matching (usually two files disagreeing on a callee's name), and
     flags forbidden constructs.
   - Read the files for made-up names, and fix bad matches before merging.
   - Squash-merge with a commit message in the project's format:
     `gh pr merge <PR> --squash --delete-branch --subject "Add: ..." --body "..."`.
   - `tools/review.sh <PR> --clean` removes the worktree.
3. **Merge and record.** Squash-merge, then on `main`:
   - `uv run tools/progress.py` first, so the names record.py checks against
     include the ones this merge added.
   - `uv run tools/record.py <issue> <model> --escalate retry`. Add
     `--model-for <addr>=<model>` for each function another model (such as a
     worker) wrote. `--escalate retry` opens an ordinary `near-miss` issue any
     model can take for everything left unmatched, including what a retry
     missed again. Since 2026-09-29 every model may take any issue, `hard`
     included, so every model's leftovers are escalated this way.
   - `uv run tools/progress.py`
   - `uv run tools/calibration.py`
   - **Group the retry by family.** `--escalate retry` only sees the functions
     this pull request left unmatched, which splits near-copies across issues
     and makes agents re-derive the same conclusion once per function. After
     it has run, look for the family members of anything just retried and, if
     any are unmatched and not already in an open issue, put them in the same
     issue with `tools/issues.py --addresses <the family members> --note "..."`
     (see [Grouping retries by family](#grouping-retries-by-family)).

   Add any suspected original bugs to `docs/bugs.md` and new techniques to
   `docs/agent-guide.md`, commit and push.
4. **Clean up what was left.** Everything left unmatched goes back out as a
   retry that any model may take; since 2026-10-01 no issue is reserved for
   one model (the old `claude` label is gone). Group near-copies together when
   you do, for the reason in step 3.
5. **Fix bad matches too.** A cheap model's file can match and still be wrong
   in other ways: `__fastcall` free functions, hand-stored vtables, invented
   names. The orchestrator fixes those during review, or with a subagent,
   before merging.
6. **Release stale claims.** For a claim with no pull request and no recent
   work (no pushed branch, and nothing changed anywhere in its
   `.worktrees/issue-<N>`, including `build/`, for a couple of hours), ask the
   human whether that session is still running: Codex and OpenCode sessions
   pause on usage limits and resume later. If it has stopped, check whether
   the uncommitted work beats `main`
   (`tools/checkall.py` in both), then unassign the issue and comment
   "Released by the orchestrator: <why>". Agents treat an issue as free when
   its most recent "Claimed by"/"Released" comment is a release, so the
   comment must start with "Released".
