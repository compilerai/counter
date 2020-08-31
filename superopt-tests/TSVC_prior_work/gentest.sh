#!/bin/bash

set -eu # fail on error

BC_O0_SUFFIX=${BC_O0_SUFFIX}.ll

> gentest_chaperon_commands
# generate .etfg and .tfg files 
echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/eqbin.py -n ${PWD}/tsvc.${BC_O0_SUFFIX} ${PWD}/tsvc.${GCC_O3_SUFFIX} ${PWD}/tsvc.${CLANG_O3_SUFFIX}  ${PWD}/tsvc.${ICC_O3_SUFFIX}" >> gentest_chaperon_commands
