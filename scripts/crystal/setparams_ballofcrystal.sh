#!/bin/bash
# Parameters for ballofcrystal.sh. Override any of them in the environment.

# Solver binaries: a STATS=ON build and a FINAL_PREDICTOR=ON build
# (see scripts/build_scripts/build_stats.sh, build_final_predictor.sh)
export STATS_BIN="${STATS_BIN:-$(pwd)/../../build_stats/cryptominisat5}"
export PRED_BIN="${PRED_BIN:-$(pwd)/../../build_pred/cryptominisat5}"
# Optional: the normal build, run at the end for the comparison ("" to skip)
export NORMAL_BIN="${NORMAL_BIN-$(pwd)/../../build/cryptominisat5}"

# Optional: frat-xor + cake_xlrup to check the proof. Set either to "" to skip.
export FRAT_XOR="${FRAT_XOR-$(pwd)/../fuzz/frat-rs}"
export CAKE_XLRUP="${CAKE_XLRUP-$(pwd)/../fuzz/cake_xlrup}"

# Data gathering
# fraction of learnt clauses tracked. auto: TRACKED clauses over the run
# (by the normal build's conflicts, or CONFL if given), at most MAXDUMPRATIO
export DUMPRATIO="${DUMPRATIO:-auto}"
export TRACKED="${TRACKED:-10000}"
export MAXDUMPRATIO="${MAXDUMPRATIO:-0.03}"
# fraction of tracked clauses never deleted. 1: every row is what the
# clause does if kept, none is cut short by the glue policy
export CLLOCK="${CLLOCK:-1.0}"
export EVERYPRED="${EVERYPRED:-10000}" # conflicts between data dumps

# The label: the future uses discounted, halving every HALFLIFE reduces,
# over two half-lives
export HALFLIFE="${HALFLIFE:-4}"

# Sampling and learning
export FIXED="${FIXED:-3000}"          # max rows per strata per table
export EVAL_REDUCES="${EVAL_REDUCES:-10}"       # the fair frame: this many reduces,
export EVAL_PER_REDUCE="${EVAL_PER_REDUCE:-3000}" # at most this many clauses of each
export cut1="${cut1:-3.0}"
export cut2="${cut2:-25.0}"
export bestf="${bestf:-$(pwd)/best_features.txt}"
export XGB_EST="${XGB_EST:-40}"
export XGB_DEPTH="${XGB_DEPTH:-5}"
export XGB_MINCHILD="${XGB_MINCHILD:-10}"
export XGB_OBJ="${XGB_OBJ:-squarederror}" # squarederror, log, poisson
export XGB_SEED="${XGB_SEED:-0}"           # train/test split and (with XGB_SUBSAMPLE < 1) the trees
export XGB_SUBSAMPLE="${XGB_SUBSAMPLE:-1.0}"
export TARGET="${TARGET:-rel}"           # count, or rel: rank among the clauses of the reduce
export XGB_WEIGHTS="${XGB_WEIGHTS:-family}" # none, strata, instance, family (see cldata_predict.py)
export EXTRA_GEN_PANDAS_OPTS="${EXTRA_GEN_PANDAS_OPTS:-}"

export STATS_OPTS="${STATS_OPTS:-}"       # e.g. "--predloc DIR" with a stats+predictor build

export NOBUF="stdbuf -oL -eL "
