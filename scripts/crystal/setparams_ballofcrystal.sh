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
export DUMPRATIO="${DUMPRATIO:-0.1}"   # fraction of learnt clauses tracked
export CLLOCK="${CLLOCK:-0.3}"         # fraction of tracked clauses never deleted
export EVERYPRED="${EVERYPRED:-10000}" # conflicts between data dumps

# Labels: 'used_later' is counted over the next SHORT/LONG/FOREVER conflicts
export SHORT="${SHORT:-10000}"
export LONG="${LONG:-30000}"
export FOREVER="${FOREVER:-120000}"

# Sampling and learning
export FIXED="${FIXED:-3000}"          # max rows per strata per tier/table
export cut1="${cut1:-3.0}"
export cut2="${cut2:-25.0}"
export bestf="${bestf:-$(pwd)/best_features.txt}"
export XGB_EST="${XGB_EST:-10}"
export XGB_DEPTH="${XGB_DEPTH:-4}"
export XGB_MINCHILD="${XGB_MINCHILD:-10}"
export EXTRA_GEN_PANDAS_OPTS="${EXTRA_GEN_PANDAS_OPTS:-}"

export NOBUF="stdbuf -oL -eL "
