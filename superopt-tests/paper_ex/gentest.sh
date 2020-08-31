#!/bin/bash

set -eu # fail on error

BC_O0_SFX=${BC_O0_SUFFIX}.ll
source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_gentest.sh

> gentest_chaperon_commands
# generate .etfg and .tfg files 
# echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/eqbin.py -n ${PWD}/paper_ex1.${BC_O0_SFX} ${PWD}/paper_ex1.${GCC_O3_SUFFIX} ${PWD}/paper_ex1.${CLANG_O3_SUFFIX}  ${PWD}/paper_ex1.${ICC_O3_SUFFIX}" >> gentest_chaperon_commands

for f in ${PROGS_PREFIX}
do
  gen_for_src_dst ${f} >> gentest_chaperon_commands
done

[[ $# -eq 0 ]] && parallel --load "${PARALLEL_LOAD_PERCENT:-30}%" < gentest_chaperon_commands || true
