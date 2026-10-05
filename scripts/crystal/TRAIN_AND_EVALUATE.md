# CrystalBall: the cluster run (todo)

What to do once there is a cluster: gather on many UNSAT instances, train,
validate. `CLAUDE.md` says how the pipeline runs and what is known.

Why a plan at all: on this box a comparison is 9 hold-out instances, and
differences under ~10% are noise. The defaults (feature list, `--predkeep
2`, the label) were chosen on six of those nine, so they are not a
hold-out. The big run has to be able to see a 5-point difference, on
instances nothing was tuned on.

Where it starts from: the embedded model is 110% of the normal build's
conflicts and 118% of its time on those nine, and +2.5 points over the
glue sort offline.

On the cluster: `eval_corpus.sh` runs one job after the other, so write
`<cnf>.<normal|pred0>.s<seed>` files with one job per run and call
`eval_summary.py` on the dir. Write a `FAMILIES` file (`family filename`
per line): the family is otherwise guessed from the file name.

## 1. Instances

- [ ] Survey: the normal build with `--xor 0` on all of satcomp
      2020-2023 (or whatever is on the cluster), timeout 5000 s. Keep
      result, conflicts, time, family.
- [ ] UNSAT only for gathering. Conflict range: 100k up to what the
      scratch disk holds (the proof is ~2 GB per million conflicts;
      node-local scratch, the proof is deleted after labelling).
- [ ] Three-way split BY FAMILY, fixed once and written to a file:
      - train: gathered and learnt from
      - dev: every decision below is taken on these
      - test: run once, at the very end, with the final configuration
- [ ] None of the 30 instances used so far (`cb_test/general/cnf`,
      `cb_test/sr19/cnf`) in dev or test; their families go to train.
- [ ] SAT instances: validation only, never gathered. A SAT set per
      split, same families rule.

## 2. Gather

- [ ] One job per instance: `ballofcrystal.sh --gather-only` on a COPY
      of the CNF in scratch (the run dir lands next to the CNF).
- [ ] Keep per instance: both frames, `data-min.db`, the `*.out-stage`
      files, `cms-stats-run.out`. Not the proof, not `data.db`.
- [ ] All gates must pass (`check_rawdb.py`, `check_data_quality.py`,
      `check_frames.py`); a table of which instances failed which gate.
- [ ] Same solver commit for all (`learn.sh` warns via `gathered_by`).
- [ ] Look at the per-instance table before learning: conflicts, rows,
      reduces, never-used share, share of the proof uses in the last
      10% of the run (ps_200_301_70 has 99%: such a run says little at
      its early reduces).
- [ ] Optional: 2-3 solver seeds per training instance. More
      trajectories of the same instance, cheap on a cluster.

## 3. The noise floor, first

- [ ] Normal build vs itself on dev: N seeds, `eval_summary.py`.
      The spread of the geometric mean is what every later difference
      is compared to. Decide N from it (so that 5 points are visible).
- [ ] Same for the current embedded model, as the starting point.

## 4. Train, decide on dev

One change at a time, each against the best so far, each with the seeds
of section 3. Offline (the fair frame) first, solver A/B for what
survives.

- [ ] Baseline: the 24-feature list, `disc`, `rel`, 40 trees depth 5.
- [ ] Does the fair offline metric agree with the solver now? If it
      does, the rest of this list gets cheap. If not, write down where.
      On this box it did not: +2.5 points over the glue sort offline,
      110% of the normal build's conflicts. First find out why (what
      the normal reduce does that a glue/size sort does not).
- [ ] `XGB_WEIGHTS`: none / strata / instance / family. On this box
      (30 instances, offline) `none` was the best and `family` the
      worst at 25%, intervals overlapping.
- [ ] `HALFLIFE`: 2 / 4 / 8 reduces (needs relabelling: `KEEP_FRAT=1`
      and `--skip-solve`, or keep `data.db`).
- [ ] Trees: early stopping on dev families; depth 4-8, min child,
      learning rate. Never tuned.
- [ ] Ranking objective: `rank:pairwise` / `rank:ndcg`, one group per
      (instance, reduce), on the fair frames.
- [ ] Watch `pred feats outside training range` on the long runs.
- [ ] Open question: plain or ancestor label (`--predanc 0` vs `1`).
      Delete the loser from the pipeline.
- [ ] Round 2: regather train under the learnt policy
      (`build_stats_pred`), train on both rounds. Not settled on this
      box.
- [ ] `--predcands 0` vs `1`, `--predkeep 0` vs `2`, with the final
      model. `--predcands 1` shrinks the DB: compare against the normal
      build with `--reducekeepused 0` too.
- [ ] Feature importance / ablation only if something above moved:
      importance-picked lists lost three times.

## 5. Validate, once

- [ ] Freeze: feature list, label, objective, tree parameters, solver
      options. Retrain on train + dev. Commit the model to
      `src/predict/` (never push).
- [ ] Test set, UNSAT and SAT, default options (XOR reasoning on, not
      `--xor 0`), timeout 5000 s, the seeds of section 3: solved count,
      PAR2, geometric mean of time and of conflicts with intervals, per
      family.
- [ ] `pred feats outside training range` on every run; list the
      instances above ~2%.
- [ ] The prediction overhead: share of the run time, per instance.
- [ ] Write the result into `CLAUDE.md`, whichever way it went.

## Rules

- Nothing is decided on the test set. One run of it.
- A difference inside the normal-vs-normal spread is not a result.
- Time/PAR2 and solved count are the verdict; conflicts explain it.
- UNSAT only for training, as always.
