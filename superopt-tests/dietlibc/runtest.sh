#!/bin/bash

set -eu # fail on error

###########################

# function specific eqflags
declare -A g_eqflags
g_global_eqflags="--global-timeout 3600" # these tests are supposed to fail; 3600s should be enough to test soundness

###########################

source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_runtest.sh
> chaperon_commands

for f in ${PROGS_PREFIX:-}
do
  gen_for_src_dst ${f} >> chaperon_commands
done

for f in ${PROGS:-}
do
  gen_for_all ${f} >> chaperon_commands
done

for f in ${LL_ASM_PROGS:-}
do
  gen_for_ll_as ${f} >> chaperon_commands
done

[[ $# -eq 0 ]] && parallel --load "${PARALLEL_LOAD_PERCENT:-30}%" < chaperon_commands || true
