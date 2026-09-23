---
deferred_work_file: '{implementation_artifacts}/deferred-work.md'
---

# Step 4: Present and Act

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- When `spec_file` is set, always write findings to the story file before offering action choices.
- `decision-needed` findings must be resolved before handling `patch` findings.

## INSTRUCTIONS

### 0. Unattended autofix branch

If `unattended=true` and `action_policy=autofix`, follow this branch and do
not execute any HALT, numbered choice, or next-step prompt below:

1. Write the normalized findings to the story file and deferred-work file as
   usual, but keep `unresolved` findings unchecked and explicit.
   If `quality_debt_file` is set, write a machine-readable JSON ledger there;
   append rather than overwrite earlier attempts, and mark ordinary findings
   and missing evidence as `blocking=false`.
2. Apply only `patch` findings that triage marked unambiguous and compatible;
   do not edit the spec to make a finding disappear.
3. Run the supplied `test_command` after patches when it exists. An empty
   command, missing command receipt, or non-zero exit is recorded as
   `test_status=missing|failed`; it is quality debt, not a human checkpoint.
4. Compute the terminal outcome:
   - `clean` or `autofixed`: all active layers completed, no quality debt
     remains, tests passed, and the receipt is complete.
   - `degraded`: patches were applied or quality evidence is incomplete; all
     verified dangerous paths were fixed or quarantined, and the receipt lists
     the debt and disabled capabilities.
   - `unverified`: one or more layers or tests could not run; no dangerous
     path was verified as open, and the receipt lists the missing evidence.
   - `review`: implementation or evidence repair is still required; the outer
     orchestrator chooses the next repair stage without asking a user.
   - `blocked`: only an external capability is unavailable or a verified
     catastrophic path cannot be fixed or quarantined.
5. `clean`, `autofixed`, `degraded`, and `unverified` may update the story and
   sprint status to `done` when implementation/build/scope/receipt gates pass.
   Never claim that failed or missing tests passed.
6. Write `{receipt_file}` when provided with `story_key`, `spec_file`,
   `diff_file`, `review_mode`, `review_depth`, `risk_reasons`, active and mandatory layers,
   failed layers, finding counts, patch/defer/rejected/unresolved counts,
   `unresolved_catastrophic`, `quality_state`, `quality_debt`,
   `safety_degraded`, `disabled_capabilities`, test exit status,
   `failure_reason`, `no_code_change`, and the terminal outcome.
7. Return a short structured summary and end the child. Do not start another
   review pass; retry is controlled by `bmad-epic-autopilot`.

### 1. Clean review shortcut

If zero findings remain after triage (all rejected or none raised), this is a
clean result only when `failed_layers` is empty and all mandatory reviewers
completed. Otherwise use `unverified` in unattended mode and record the
missing evidence; do not manufacture a clean result.

### 2. Write findings to the story file

If `{spec_file}` exists and contains a Tasks/Subtasks section, append a `### Review Findings` subsection. Write all findings in this order:

1. **`decision-needed`** findings (unchecked):
   `- [ ] [Review][Decision] <Title> — <Detail>`

2. **`patch`** findings (unchecked):
   `- [ ] [Review][Patch] <Title> [<file>:<line>]`

3. **`defer`** findings (checked off, marked deferred):
   `- [x] [Review][Defer] <Title> [<file>:<line>] — deferred: <pre-existing, or for maybe-false the evidence that would settle it>`

Also append each `defer` finding to `{deferred_work_file}` under a heading `## Deferred from: code review ({date})`. If `spec_file` is set, include its basename in the heading (e.g., `code review of story-3.3 (2026-03-18)`). One bullet per finding with description.

### 3. Present summary

Announce what was written:

> **Code review complete.** <D> `decision-needed`, <P> `patch`, <W> `defer`, <R> rejected.

The findings report ends with a `Rejected` appendix — one line per rejected finding: `false` with its refutation, `low` with why it was not worth fixing — in the story file's `### Review Findings` section when `spec_file` is set, at the tail of the chat listing otherwise.

If `spec_file` is set, add: `Findings written to the review findings section in {spec_file}.`
Otherwise add: `Findings are listed above. No story file was provided, so nothing was persisted.`

### 4. Resolve decision-needed findings

If `decision_needed` findings exist, present each one with its detail and the options available. The user must decide — the correct fix is ambiguous without their input. Walk through each finding (or batch related ones) and get the user's call. Once resolved, each becomes a `patch`, `defer`, or is rejected.

If the user chooses to defer, ask: Quick one-line reason for deferring this item? (helps future reviews): — then append that reason to both the story file bullet and the `{deferred_work_file}` entry.

**HALT** — I am waiting for your numbered choice. Reply with only the number. Do not proceed until you select an option.

### 5. Handle `patch` findings

If `patch` findings exist (including any resolved from step 4), HALT. Ask the user:

If `spec_file` is set, present all three options:

> **How would you like to handle the `<P>` `patch` findings?**
> 1. **Apply every patch** — fix all of them now, no per-finding confirmation. Defer and decision-needed items are not touched.
> 2. **Leave as action items** — they are already in the story file
> 3. **Walk through each patch** — show details for each before deciding

If `spec_file` is **not** set, present only options 1 and 2 (omit "Leave as action items" — findings were not written to a file):

> **How would you like to handle the `<P>` `patch` findings?**
> 1. **Apply every patch** — fix all of them now, no per-finding confirmation. Defer and decision-needed items are not touched.
> 2. **Walk through each patch** — show details for each before deciding

**HALT** — I am waiting for your numbered choice. Reply with only the number. Do not proceed until you select an option.

- **Apply every patch**: Apply every patch finding without per-finding confirmation. Do not modify defer or decision-needed items. After all patches are applied, present a summary of changes made. If `spec_file` is set, check off the patch items in the story file (leave defer items as-is).
- **Leave as action items** (only when `spec_file` is set): Done — findings are already written to the story.
- **Walk through each patch**: Present each finding with full detail, diff context, and suggested fix. After walkthrough, re-offer the applicable options above.

  **HALT** — I am waiting for your numbered choice. Do not proceed until you select an option.

**✅ Code review actions complete**

- Decision-needed resolved: <D>
- Patches handled: <P>
- Deferred: <W>
- Rejected: <R>

### 6. Update story status and sync sprint tracking

Skip this section if `spec_file` is not set.

#### Determine new status based on review outcome

- If implementation/build/scope/receipt gates pass and no verified catastrophic path remains open: set `new_status` = `done`. The outcome may be `clean`, `autofixed`, `degraded`, or `unverified`; update the story Status section to `done` and persist quality debt.
- If implementation or evidence repair is still required: set `new_status` = `review`; the unattended outer orchestrator chooses the next repair stage without human input.
- A failed layer, failed test, ordinary unresolved high/medium finding, or missing verification alone is not a reason to keep the story at `review`.

Save the story file.

#### Sync sprint-status.yaml

If `story_key` is not set, skip this subsection and note that sprint status was not synced because no story key was available.

If `{sprint_status}` file exists:

1. Load the FULL `{sprint_status}` file.
2. Find the `development_status` entry matching `{story_key}`.
3. If found: update `development_status[{story_key}]` to `{new_status}`. Update `last_updated` to current date. Save the file, preserving ALL comments and structure including STATUS DEFINITIONS.
4. If `{story_key}` not found in sprint status: warn the user that the story file was updated but sprint-status sync failed.

If `{sprint_status}` file does not exist, note that story status was updated in the story file only.

#### Completion summary

> **Review Complete!**
>
> **Story Status:** `{new_status}`
> **Issues Fixed:** <fixed_count>
> **Action Items Created:** <action_count>
> **Deferred:** <W>
> **Rejected:** <R>

### 7. Next steps

Present the user with follow-up options:

> **What would you like to do next?**
> 1. **Start the next story** — run `dev-story` to pick up the next `ready-for-dev` story
> 2. **Re-run code review** — address findings and review again
> 3. **Done** — end the workflow

**HALT** — I am waiting for your choice. Do not proceed until the user selects an option.

## On Complete

Run: `uv run {project-root}/_bmad/scripts/resolve_customization.py --skill {skill-root} --project-root {project-root} --key workflow.on_complete`

If the resolved `workflow.on_complete` is non-empty, follow it as the final terminal instruction before exiting.
