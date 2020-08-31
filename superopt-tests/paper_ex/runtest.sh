#!/bin/bash

set -eu # fail on error

################################

declare -A g_eqflags

################################

source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_runtest.sh

# 5 min smt-timeout, 5 hour global timeout
g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log"

##************* BFS ***************##
> chaperon_commands_paper_ex

binary=paper_ex
compiler=gcc     gen_commands_from_file_src_dst  paper_func       " "        "gcc.BFS"  >> chaperon_commands_paper_ex

[[ $# -eq 0 ]] && parallel --load "${PARALLEL_LOAD_PERCENT:-30}%" < chaperon_commands_paper_ex || true
