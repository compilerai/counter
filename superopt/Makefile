SHELL := /bin/bash

T := codegen src_iseq_get_usedef push_dbs pull_dbs binutils_target solvers tfg2dot qcc-codegen expr_parse etfg_split_memory_and_propagate_sprels pred_canonicalizer prove_queries_diff eqgen z3_parse z3_neo_parse boolector_parse yices_parse cvc4_parse parse_edge_composition decide_hoare_triple simplify prove_query_split eq smt_helper_process qd_helper_process check_wfconds_on_edge update_invariant_state_over_edge prove ce_eval_prove update_dst_edge_for_local_alloc_and_dealloc debug_soundness range_tester is_expr_equal_using_lhs_set expr_is_provable infer_invariants_xfer_over_path mybitset_tester graph_propagate_CEs_across_new_edge tfg_ssa_construct_from_non_ssa_tfg
T32 := $(addsuffix 32,$(T))
T64 := $(addsuffix 64,$(T))

.PHONY: $(T) $(T32) $(T64)

grandparent::
	make -C ../..

all: eqcheck32

$(T): %: %32 %64

$(T32): %32:
	[[ -d "build/etfg_i386" ]] && ninja -C build/etfg_i386 -j0 -l`nproc` $* || true

$(T64): %64:
	[[ -d "build/etfg_x64" ]] && ninja -C build/etfg_x64 -j0 -l`nproc` $* || true

tags::
	ctags -R include/ lib/ tools/
	find include/ lib/ tools/ -name "*.h" > cscope.files
	find include/ lib/ tools/ -name "*.c" >> cscope.files
	find include/ lib/ tools/ -name "*.cpp" >> cscope.files
	GTAGSFORCECPP=1 gtags -f cscope.files

.PHONY: eqcheck32 eqcheck64 etfg parsers

eqcheck32 eqcheck64: eqcheck%: eq% eqgen% smt_helper_process% prove% decide_hoare_triple% update_invariant_state_over_edge% qd_helper_process%
etfg: codegen32 eqgen32 eq32
parsers: z3_parse z3_neo_parse boolector_parse yices_parse cvc4_parse

.PHONY: i386 
i386:
	if [ -d "build/i386_i386" ]; then ninja -C build/i386_i386 -j0 -l`nproc` rewrite && ninja -C build/i386_i386 -j0 -l`nproc` harvest; fi

.PHONY: debug
debug:
	if [ -d "build/etfg_i386" ]; then ninja -C build/etfg_i386 -j0 -l`nproc`; fi
	if [ -d "build/i386_i386" ]; then ninja -C build/i386_i386 -j0 -l`nproc`; fi
	if [ -d "build/etfg_x64" ];  then ninja -C build/etfg_x64  -j0 -l`nproc`; fi
	if [ -d "build/x64_x64" ];   then ninja -C build/x64_x64   -j0 -l`nproc`; fi

.PHONY: debug
libs:
	if [ -d "build/etfg_x64" ];  then ninja -C build/etfg_x64  -j0 -l`nproc` identify_durables; fi

.PHONY: clean
clean:
	cmake --build build/i386_i386 --target clean
	cmake --build build/etfg_i386 --target clean
	#make -C build/ppc_i386 clean

.PHONY: clean_dbs
clean_dbs:
	if [ -d "build/i386_i386" ]; then cmake --build build/i386_i386 --target clean_dbs; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target clean_dbs; fi
	#if [ -d "build/ppc_i386" ]; then make -C build/ppc_i386 clean_dbs; fi

.PHONY: test-failing
test-failing:
	python utils/eq_test.py failing

.PHONY: test-passing
test-passing:
	python utils/eq_test.py passing

.PHONY: tidy
tidy:
	run-clang-tidy-9 -p build/etfg_i386 -checks='-*,clang-analyzer-core*,clang-analyzer-cplusplus*,cppcoreguidelines*,modernize-*'
