#!/bin/bash

set -eu # fail on error

################################

declare -A g_eqflags

################################

source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_runtest.sh

# 5 min smt-timeout, 5 hour global timeout
g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log"

##************* BFS ***************##
> chaperon_commands_bfs

##************* SEMALIGN NONVEC***************##
binary=tsvc
compiler=gcc    gen_commands_from_file  gcc_tsvc_funcs_nonvec_semalign    " --unroll-factor 2"   "BFS"    >> chaperon_commands_bfs
compiler=clang  gen_commands_from_file  clang_tsvc_funcs_nonvec_semalign  " --unroll-factor 2"   "BFS"    >> chaperon_commands_bfs
compiler=icc    gen_commands_from_file  icc_tsvc_funcs_nonvec_semalign    " --unroll-factor 2"   "BFS"    >> chaperon_commands_bfs

##************* SEMALIGN VEC***************##
binary=tsvc
compiler=gcc    gen_commands_from_file  gcc_tsvc_funcs_semalign    " --unroll-factor 4"   "BFS"    >> chaperon_commands_bfs
compiler=clang  gen_commands_from_file  clang_tsvc_funcs_semalign  " --unroll-factor 8"   "BFS"    >> chaperon_commands_bfs
compiler=clang  gen_commands_from_file  clang_tsvc_funcs_semalign_nolu  " --disable_residual_loop_unroll --unroll-factor 8"                               "BFS"    >> chaperon_commands_bfs
compiler=icc    gen_commands_from_file  icc_tsvc_funcs_semalign    " --unroll-factor 4"   "BFS"    >> chaperon_commands_bfs

> chaperon_commands_dfs

##************* DFS ***************##
 g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics "
 
 ##************* SEMALIGN NONVEC***************##
 binary=tsvc
 compiler=gcc    gen_commands_from_file  gcc_tsvc_funcs_nonvec_semalign    " --unroll-factor 2"   "DFS"    >> chaperon_commands_dfs
 compiler=clang  gen_commands_from_file  clang_tsvc_funcs_nonvec_semalign  " --unroll-factor 2"   "DFS"    >> chaperon_commands_dfs
 compiler=icc    gen_commands_from_file  icc_tsvc_funcs_nonvec_semalign    " --unroll-factor 2"   "DFS"    >> chaperon_commands_dfs
 
##************* SEMALIGN VEC***************##
binary=tsvc
compiler=gcc    gen_commands_from_file  gcc_tsvc_funcs_semalign    " --unroll-factor 4"   "DFS"    >> chaperon_commands_dfs
compiler=clang  gen_commands_from_file  clang_tsvc_funcs_semalign  " --unroll-factor 8"   "DFS"    >> chaperon_commands_dfs
compiler=clang  gen_commands_from_file  clang_tsvc_funcs_semalign_nolu  " --disable_residual_loop_unroll --unroll-factor 8"                               "DFS"    >> chaperon_commands_dfs
compiler=icc    gen_commands_from_file  icc_tsvc_funcs_semalign    " --unroll-factor 4"   "DFS"    >> chaperon_commands_dfs


#parallel --load "${PARALLEL_LOAD_PERCENT:-100}%" < chaperon_commands
