#!/bin/bash

set -eu # fail on error

BC_O0_SFX=${BC_O0_SUFFIX}.ll
source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_gentest.sh

> gentest_chaperon_commands

# generate .etfg and .tfg files 
echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/eqbin.py -n ${PWD}/oopsla_tests.${BC_O0_SFX} ${PWD}/oopsla_tests.${GCC_O3_SUFFIX} ${PWD}/oopsla_tests.${CLANG_O3_SUFFIX}" >> gentest_chaperon_commands

echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/eqbin.py -n ${PWD}/oopsla_tests_8uf.${BC_O0_SFX} ${PWD}/oopsla_tests_8uf.${GCC_O3_SUFFIX}" >> gentest_chaperon_commands

echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/eqbin.py -n ${PWD}/oopsla_tests_icc.${BC_O0_SFX} ${PWD}/oopsla_tests_icc.${ICC_O3_SUFFIX}"   >> gentest_chaperon_commands

for f in ${PROGS_PREFIX}
do
  gen_for_src_dst ${f} >> gentest_chaperon_commands
done
