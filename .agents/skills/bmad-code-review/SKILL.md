---
name: bmad-code-review
description: 'Review code changes with several independent reviewers in parallel, then triage and present the findings. Use when the user says "run code review" or "review this code"'
---

# Code Review Workflow

**Goal:** Review code changes adversarially with a caller-selected depth. No noise, no filler, and no automatic completion claim when a mandatory reviewer or verification step failed.

## Automation contract

An unattended caller may provide a complete contract in its invocation:

```text
review_depth: lite | standard | deep
risk_reasons: <JSON/list of classifier reasons>
review_mode: full | no-spec
action_policy: autofix
unattended: true
story_key: <exact sprint-status key>
spec_file: <absolute story path>
diff_file: <absolute story-local unified diff>
receipt_file: <absolute JSON receipt path>
test_command: <non-empty command recorded by dev-story>
sprint_status: <absolute sprint-status path>
```

When the contract is complete, use those paths and values directly. Do not
run the interactive target-selection cascade, ask for a spec, ask for a diff,
wait at a summary checkpoint, or rediscover the working-tree diff. Missing,
unreadable, empty, or oversized inputs are structured failures in unattended
mode. Interactive invocations retain the normal human checkpoints.

Subagents, when the capability is available, are an important part of this workflow. Use them as directed by the workflow steps.
Interactive runs may ask once if the host requires explicit permission to launch them; unattended runs already carry that authorization and must not ask.

## Conventions

- Bare paths (e.g. `checklist.md`) resolve from the skill root.
- `{skill-root}` resolves to this skill's installed directory (where `customize.toml` lives).
- `{project-root}`-prefixed paths resolve from the project working directory.
- `{skill-name}` resolves to the skill directory's basename.

## On Activation

### Step 1: Resolve the Workflow Block

Run: `uv run {project-root}/_bmad/scripts/resolve_customization.py --skill {skill-root} --project-root {project-root} --key workflow`

**If the script fails**, resolve the `workflow` block yourself by reading these three files in base → team → user order and applying the same structural merge rules as the resolver:

1. `{skill-root}/customize.toml` — defaults
2. `{project-root}/_bmad/custom/{skill-name}.toml` — team overrides
3. `{project-root}/_bmad/custom/{skill-name}.user.toml` — personal overrides

Any missing file is skipped. Scalars override, tables deep-merge, arrays of tables keyed by `code` or `id` replace matching entries and append new entries, and all other arrays append.

### Step 2: Execute Prepend Steps

Execute each entry in `{workflow.activation_steps_prepend}` in order before proceeding.

### Step 3: Load Persistent Facts

Treat every entry in `{workflow.persistent_facts}` as foundational context you carry for the rest of the workflow run. Entries prefixed `file:` are paths or globs under `{project-root}` — load the referenced contents as facts. All other entries are facts verbatim.

### Step 4: Load Config

Load config from `{project-root}/_bmad/bmm/config.yaml` and resolve:

- `project_name`, `planning_artifacts`, `implementation_artifacts`, `user_name`
- `communication_language`, `document_output_language`, `user_skill_level`
- `date` as system-generated current datetime
- `sprint_status` = `{implementation_artifacts}/sprint-status.yaml`
- `project_context` = `**/project-context.md` (load if exists)
- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`

### Step 5: Greet the User

Greet `{user_name}`, speaking in `{communication_language}`.

### Step 6: Execute Append Steps

Execute each entry in `{workflow.activation_steps_append}` in order.

Activation is complete. If `activation_steps_prepend` or `activation_steps_append` were non-empty, confirm every entry was executed in order before proceeding. Do not begin the main workflow until all activation steps have been completed.

## WORKFLOW ARCHITECTURE

This uses **step-file architecture** for disciplined execution:

- **Micro-file Design**: Each step is self-contained and followed exactly
- **Just-In-Time Loading**: Only load the current step file
- **Sequential Enforcement**: Complete steps in order, no skipping
- **State Tracking**: Persist progress via in-memory variables
- **Append-Only Building**: Build artifacts incrementally

### Step Processing Rules

1. **READ COMPLETELY**: Read the entire step file before acting
2. **FOLLOW SEQUENCE**: Execute sections in order
3. **WAIT FOR INPUT (interactive only)**: Halt at checkpoints and wait for human input. An explicit `unattended=true` contract skips those checkpoints and records the decision in its receipt.
4. **LOAD NEXT**: When directed, read fully and follow the next step file

### Critical Rules (NO EXCEPTIONS)

- **NEVER** load multiple step files simultaneously
- In unattended mode, a child executes exactly one review/triage/action pass;
  it must not recursively start another review pass. Retry ownership belongs to
  the outer orchestrator.
- **ALWAYS** read entire step file before execution
- **NEVER** skip steps or optimize the sequence
- **ALWAYS** follow the exact instructions in the step file
- **ALWAYS** halt at checkpoints and wait for human input in interactive mode; unattended mode must never halt for input.

## FIRST STEP

Read fully and follow: `./steps/step-01-gather-context.md`
