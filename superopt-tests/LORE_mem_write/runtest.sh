#!/bin/bash

set -eu # fail on error

################################

declare -A g_eqflags

g_eqflags["ex109.gcc"]=" --unroll-factor 4"
g_eqflags["ex1010.gcc"]=" --unroll-factor 4"
g_eqflags["ex1011.gcc"]=" --unroll-factor 4"

g_eqflags["ex109_8.gcc"]=" --unroll-factor 8"

g_eqflags["ex1010.clang"]=" --unroll-factor 8 --global-timeout 36000"
g_eqflags["ex1011.clang"]=" --unroll-factor 8 --global-timeout 18000"
################################

source ${SUPEROPT_PROJECT_DIR}/superopt-tests/scripts/eqchecker_runtest.sh
##************* BFS ***************##

> chaperon_commands_bfs
# 5 min smt-timeout, 10 hour global timeout
g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log"

##************* 4 unroll factor ***************##

binary=oopsla_tests
compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec           " --unroll-factor 4 --disable_residual_loop_unroll"                               "4uf.BFS"  >> chaperon_commands_bfs

binary=corr_mutations
g_global_eqflags=""
compiler=gcc     gen_commands_from_file_src_dst  gcc_oopsla_funcs_hand       "--debug=oopsla_log --global-timeout 18000 --smt-query-timeout 300 "        "4uf.BFS"  >> chaperon_commands_bfs

##************* 8 unroll factor ***************##

binary=oopsla_tests
g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log"
compiler=clang  gen_commands_from_file  clang_oopsla_funcs_vec         " --unroll-factor 8 --disable_residual_loop_unroll"                               "8uf.BFS"  >> chaperon_commands_bfs
binary=oopsla_tests_8uf
compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec_8uf       " --unroll-factor 8 --disable_residual_loop_unroll"                               "8uf.BFS"  >> chaperon_commands_bfs
compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec_8uf_nolpr " --disable_loop_path_exprs --unroll-factor 8 --disable_residual_loop_unroll"     "8uf.BFS"  >> chaperon_commands_bfs

binary=corr_mutations
g_global_eqflags=""
compiler=gcc     gen_commands_from_file_src_dst  gcc_oopsla_funcs_hand_8uf       "--debug=oopsla_log --global-timeout 18000 --smt-query-timeout 300 "    "8uf.BFS"  >> chaperon_commands_bfs
compiler=clang   gen_commands_from_file_src_dst  clang_oopsla_funcs_hand     "--debug=oopsla_log --smt-query-timeout 300 "        "8uf.BFS"  >> chaperon_commands_bfs


##************* DFS ***************##
 
 g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics"
> chaperon_commands_dfs

 ##************* 4 unroll factor ***************##

 binary=oopsla_tests
 compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec           " --unroll-factor 4 --disable_residual_loop_unroll"                              "4uf.DFS"  >> chaperon_commands_dfs

binary=corr_mutations
 g_global_eqflags=" "
compiler=gcc     gen_commands_from_file_src_dst  gcc_oopsla_funcs_hand       "--debug=oopsla_log --global-timeout 18000 --smt-query-timeout 300  --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics "        "4uf.DFS"  >> chaperon_commands_dfs

 ##************* 8 unroll factor ***************##

binary=oopsla_tests
 g_global_eqflags="--global-timeout 18000 --smt-query-timeout 300 --debug=oopsla_log --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics"
compiler=clang  gen_commands_from_file  clang_oopsla_funcs_vec         " --unroll-factor 8 --disable_residual_loop_unroll"                               "8uf.DFS"  >> chaperon_commands_dfs
binary=oopsla_tests_8uf
compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec_8uf       " --unroll-factor 8 --disable_residual_loop_unroll"                               "8uf.DFS"  >> chaperon_commands_dfs
compiler=gcc    gen_commands_from_file  gcc_oopsla_funcs_vec_8uf_nolpr " --disable_loop_path_exprs --unroll-factor 8 --disable_residual_loop_unroll"     "8uf.DFS"  >> chaperon_commands_dfs

 binary=corr_mutations
 g_global_eqflags=" "
compiler=gcc     gen_commands_from_file_src_dst  gcc_oopsla_funcs_hand_8uf       "--debug=oopsla_log --global-timeout 18000 --smt-query-timeout 300  --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics "        "8uf.DFS"  >> chaperon_commands_dfs
compiler=clang   gen_commands_from_file_src_dst  clang_oopsla_funcs_hand     "--debug=oopsla_log --smt-query-timeout 300  --disable_dst_bv_rank --disable_src_bv_rank --disable_propagation_based_pruning --disable_all_static_heuristics "        "8uf.DFS"  >> chaperon_commands_dfs

# 
# 
# #parallel --load "${PARALLEL_LOAD_PERCENT:-100}%" < chaperon_commands
