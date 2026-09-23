---
failed_layers: '' # set at runtime: comma-separated list of layers that failed or returned empty
---

# Step 2: Review

## RULES

- YOU MUST ALWAYS SPEAK OUTPUT in your Agent communication style with the config `{communication_language}`
- All review subagents must run at the same model capability as the current session.
- Run subagents synchronously: launch them together as blocking calls awaited in this turn — never backgrounded or detached, never ending the turn to await results.

## INSTRUCTIONS

1. The review layers are `{workflow.review_layers}`, resolved during activation.

   If `unattended=true` and `no_code_change=true`, do not launch reviewer
   children. Record `active_layers=[]`, `mandatory_layers=[]`,
   `completed_layers=[]`, `failed_layers=[]`, and a non-blocking
   `quality_debt` entry explaining that the validated story-local diff is
   empty. Continue to triage and presentation so the outer orchestrator can
   close the story as `unverified` when implementation and evidence receipts
   are complete.

   In unattended mode, the runtime depth is authoritative and the active layer
   matrix is:

   | `review_depth` | Active layers | Mandatory layers |
   |---|---|---|
   | `lite` | `edge-case-hunter` | `edge-case-hunter` |
   | `standard` | `edge-case-hunter`, `verification-gap`, plus `acceptance-auditor` when `review_mode=full` | every active layer |
   | `deep` | all configured layers, plus `acceptance-auditor` when `review_mode=full` | every active layer |

   A layer's `when` text is a gate, not a reason to silently run an inactive
   layer. The selected mandatory set must be recorded in the receipt.

2. For each layer in `{workflow.review_layers}`:
   - `instruction` empty or missing → drop the layer silently (an override disabled it).
   - `when` condition present and not satisfied by the current context (`{review_mode}`, `{review_depth}`, `{spec_file}`) → drop the layer and, for interactive runs, tell the user, e.g. "Acceptance Auditor skipped — no spec file provided." In unattended runs record the skip in the receipt without prompting.
   - otherwise → the layer is active.

   If no layer is active, record `reviewer-config-missing` and let the unattended
   caller attempt to restore the current project's customization. Only an
   unavailable skill/config source is terminal; do not wait for a human.

3. Announce skipped layers first, then launch every active layer before handling any layer's result. Try running all active layers simultaneously: expand `{skill-root}` in each layer's `instruction` to this skill's absolute installed directory, then substitute the runtime placeholders (`{diff_file}`, `{claims_file}`, `{spec_file}`). `{diff_file}` is a path: substitute the path itself and let the layer read the file — a launch prompt never carries diff text. For an instruction that launches a reviewer subagent, launch that child with the prompt text after placeholder substitution; do not load the reviewer instruction file yourself. For any other customized instruction, execute it as written. Do not leave `{skill-root}` unresolved in a child prompt, and resolve `{diff_file}` to an absolute path — the child's working directory is not yours. If the host cannot run all active layers at once, unattended mode may run them in bounded batches and then merge all results; it must never ask the user to open another session. If a reviewer is unavailable in a batch, record that layer as failed and continue to the mandatory-quorum gate. The external-session fallback remains available only to interactive runs.

4. **Layer failure handling**: If any active layer fails or times out, append the layer's `name` to `failed_layers` (a JSON list), continue collecting other results, and add a non-blocking `quality_debt` item with the layer, error and retry strategy. The run is `unverified`, not `clean`; the outer orchestrator may continue to `done` after evidence/safety checks. Only a missing review skill/config that cannot be restored is terminal.

5. Collect all findings from the completed layers, keeping track of each finding's originating layer `id`.

## NEXT

Read fully and follow `./step-03-triage.md`
