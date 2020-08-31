all: cur #etfg i386
#all: eq
#i386

cur::
	make -C ../..

tags::
	ctags -R include/ lib/ tools/
	find include/ lib/ tools/ -name "*.h" > cscope.files
	find include/ lib/ tools/ -name "*.c" >> cscope.files
	find include/ lib/ tools/ -name "*.cpp" >> cscope.files
	GTAGSFORCECPP=1 gtags -f cscope.files

.PHONY: codegen src_iseq_get_usedef push_dbs pull_dbs binutils_target solvers tfg2dot etfg2dot qcc-codegen expr_parse etfg_split_memory_and_propagate_sprels pred_canonicalizer prove_queries_diff eqgen z3_parse boolector_parse yices_parse cvc4_parse parse_edge_composition decide_hoare_triple
codegen src_iseq_get_usedef binutils_target tfg2dot etfg2dot qcc-codegen expr_parse etfg_split_memory_and_propagate_sprels pred_canonicalizer prove_queries_diff eqgen z3_parse boolector_parse yices_parse cvc4_parse push_dbs pull_dbs solvers parse_edge_composition decide_hoare_triple:
	cmake --build build/etfg_i386 --target $@

.PHONY: etfg
etfg:
	#if [ -d "build/etfg_i386" ]; then make -j 8 -C build/etfg_i386 codegen && make -j8 -C build/etfg_i386 enumerate_for_src_iseq_using_itable src_iseq_compute_fingerprint enum_candidate_make_itables src_iseq_compute_typestate src_iseq_gen_rand_states peeptab_verify typecheck codegen gen_enum_candidates_from_harvest_out db_create_schema dbclear db_pick_itable_for_enum_candidate db_populate_enum_candidates db_populate_itables_for_enum_candidate db_pick_enum_candidate db_update_itable_enum_state_for_enum_candidate db_add_peep_entry db_download_peeptab; fi
	#if [ -d "build/etfg_i386" ]; then make -j 8 -C build/etfg_i386 codegen && make -j 8 -C build/etfg_i386 codegen eqgen eq smt_helper_process peep_enumerate_transmaps prove_using_local_sprel_expr_guesses prove simplify ; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target codegen && cmake --build build/etfg_i386 --target eqgen && cmake --build build/etfg_i386 --target eq ; fi

.PHONY: i386 
i386:
	if [ -d "build/i386_i386" ]; then cmake --build build/i386_i386 --target rewrite && cmake --build build/i386_i386 --target harvest; fi

.PHONY: eq
eq:
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target eq; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target smt_helper_process; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target prove_using_local_sprel_expr_guesses; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target prove; fi
	if [ -d "build/etfg_i386" ]; then cmake --build build/etfg_i386 --target update_invariant_state_over_edge; fi

.PHONY: debug
debug:
	if [ -d "build/etfg_i386" ]; then ninja -C build/etfg_i386 -j `nproc`; fi
	if [ -d "build/i386_i386" ]; then ninja -C build/i386_i386 -j `nproc`; fi

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

.PHONY: parsers
parsers: z3_parse boolector_parse yices_parse cvc4_parse

.PHONY: tidy
tidy:
	run-clang-tidy-9 -p build/etfg_i386 -checks='-*,clang-analyzer-core*,clang-analyzer-cplusplus*,cppcoreguidelines*,modernize-*'; fi

