include Make.conf

export SUPEROPT_PROJECT_DIR ?= $(PWD)
SUPEROPT_INSTALL_DIR ?= $(SUPEROPT_PROJECT_DIR)/usr/local
SUPEROPT_INSTALL_FILES_DIR ?= $(SUPEROPT_INSTALL_DIR)
SUPEROPT_PROJECT_BUILD = $(SUPEROPT_PROJECT_DIR)/build
# check if sudo command is available
ifeq (,$(shell which sudo))
SUDO :=
else ifdef SUDO
SUDO := $(SUDO)
else
SUDO := sudo
endif
# PARALLEL_LOAD_PERCENT ?= 100  # parallel will start new jobs until number of processes fall below this value

SHELL := /bin/bash
export SUPEROPT_TARS_DIR ?= ~/tars
Z3=z3-4.8.7

MAJOR_VERSION=0
MINOR_VERSION=1
PACKAGE_REVISION=0
PACKAGE_NAME=qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)

all:: $(SUPEROPT_PROJECT_BUILD)/qcc
	$(MAKE) -C superopt debug
	$(MAKE) -C llvm-project
	$(MAKE) -C superoptdbs

$(SUPEROPT_PROJECT_BUILD)/qcc: Make.conf Makefile
	mkdir -p $(SUPEROPT_PROJECT_BUILD)
	echo "$(SUPEROPT_INSTALL_DIR)/bin/clang-qcc $(CLANG_I386_EQCHECKER_FLAGS)" '$$*' > $@
	chmod +x $@

linkinstall::
	$(SUDO) mkdir -p $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) mkdir -p $(SUPEROPT_INSTALL_DIR)/include
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-link $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-as $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/opt $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llc $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/binutils-2.21-install/bin/ld $(SUPEROPT_INSTALL_DIR)/bin/qcc-ld
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eq $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eqgen $(SUPEROPT_INSTALL_DIR)/bin
#	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/qcc-codegen $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/codegen $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/debug_gen $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/smt_helper_process $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/i386_i386/harvest $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm2tfg $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang $(SUPEROPT_INSTALL_DIR)/bin/clang-qcc
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/lib $(SUPEROPT_INSTALL_DIR)
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libLockstepDbg.a $(SUPEROPT_INSTALL_DIR)/lib
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libmymalloc.a $(SUPEROPT_INSTALL_DIR)/lib
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superoptdbs $(SUPEROPT_INSTALL_DIR)
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3/usr/bin/z3 $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3/usr/lib/libz3.so* $(SUPEROPT_INSTALL_DIR)/lib
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3/usr/include/z3_*.h $(SUPEROPT_INSTALL_DIR)/include
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/yices_smt2 $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/cvc4 $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/boolector $(SUPEROPT_INSTALL_DIR)/bin
	$(SUDO) ln -sf $(SUPEROPT_PROJECT_DIR)/build/qcc $(SUPEROPT_INSTALL_DIR)/bin

cleaninstall::
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm-link
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm-as
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/opt
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/llc
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc-ld
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/eq
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/eqgen
#	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc-codegen
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/codegen
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/debug_gen
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/smt_helper_process
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/lib/libLockstepDbg.a
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/lib/libmymalloc.a
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/harvest
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm2tfg
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/clang-qcc
	$(SUDO) rm -rf $(SUPEROPT_INSTALL_DIR)/lib
	$(SUDO) rm -rf $(SUPEROPT_INSTALL_DIR)/superoptdbs
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/yices_smt2
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/boolector
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/cvc4
	$(SUDO) rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc
	rm -f $(SUPEROPT_PROJECT_BUILD)/qcc

build::
	# unzip dbs
	$(MAKE) -C superoptdbs
	# unzip binlibs
	$(MAKE) -C binlibs
	# build superopt
	pushd superopt && ./configure --use-ninja && popd;
	$(MAKE) -C superopt solvers
	cmake --build superopt/build/etfg_i386 --target superoptdb_rpc.h
	cmake --build superopt/build/i386_i386 --target superoptdb_rpc.h
	cmake --build superopt/build/etfg_i386 --target eq
	cmake --build superopt/build/etfg_i386 --target smt_helper_process
	cmake --build superopt/build/etfg_i386 --target eqgen
#	cmake --build superopt/build/etfg_i386 --target qcc-codegen
	#cmake --build superopt/build/etfg_i386 --target codegen
	#cmake --build superopt/build/etfg_i386 --target debug_gen
	cmake --build superopt/build/i386_i386 --target harvest
	#cmake --build superopt/build/etfg_i386 --target prove_using_local_sprel_expr_guesses
	#cmake --build superopt/build/etfg_i386 --target update_invariant_state_over_edge
	# build our llvm fork and custom llvm-based libs and utils
	pushd llvm-project && $(MAKE) install && popd
	ninja -C llvm-project/build llvm2tfg clang opt llvm-link llvm-dis
	# build qcc
	#$(MAKE) $(SUPEROPT_PROJECT_BUILD)/qcc

install::
	$(MAKE) build
	$(MAKE) linkinstall

test::
	$(MAKE) testinit
	$(MAKE) gentest
	$(MAKE) eqtest |  tee "$(shell date +"%F_%T").eqruncache"

# multiple steps for jenkins pipeline view
testinit::
	#pushd superopt-tests && ./configure && (make clean; true) && make && popd
	pushd superopt-tests && ./configure && $(MAKE) && popd
	cp -r icc_binaries/* superopt-tests/build

gentest::
	$(MAKE) -C superopt-tests gen_oopsla_test

eqtest::
	$(MAKE) -C superopt-tests run_oopsla_test_bfs
	$(MAKE) -C superopt-tests run_oopsla_test_dfs

oopsla_tsvc_prior_bfs::
	$(MAKE) -C superopt-tests run_oopsla_tsvc_prior_bfs

oopsla_tsvc_prior_dfs::
	$(MAKE) -C superopt-tests run_oopsla_tsvc_prior_dfs

oopsla_tsvc_new_bfs::
	$(MAKE) -C superopt-tests run_oopsla_tsvc_new_bfs

oopsla_tsvc_new_dfs::
	$(MAKE) -C superopt-tests run_oopsla_tsvc_new_dfs

oopsla_lore_mem_write_bfs::
	$(MAKE) -C superopt-tests run_oopsla_lore_mem_bfs

oopsla_lore_mem_write_dfs::
	$(MAKE) -C superopt-tests run_oopsla_lore_mem_dfs

oopsla_lore_no_mem_write_bfs::
	$(MAKE) -C superopt-tests run_oopsla_lore_nomem_bfs

oopsla_lore_no_mem_write_dfs::
	$(MAKE) -C superopt-tests run_oopsla_lore_nomem_dfs

run_dietlibc::
	$(MAKE) testinit
	$(MAKE) -C superopt-tests run_dietlibc

run_paper_ex::
	$(MAKE) testinit
	$(MAKE) -C superopt-tests run_paper_ex

tableresults::
	$(MAKE) install
	$(MAKE) test
	$(MAKE) tablesummary

tablesummary::
	python3 table-gen-csv.py

printpaths:
	@echo "SUPEROPT_PROJECT_DIR = $(SUPEROPT_PROJECT_DIR)"
	@echo "SUPEROPT_INSTALL_DIR = $(SUPEROPT_INSTALL_DIR)"
	@echo "SUPEROPT_INSTALL_FILES_DIR = $(SUPEROPT_INSTALL_FILES_DIR)"
	@echo "SUPEROPT_PROJECT_BUILD = $(SUPEROPT_PROJECT_BUILD)"
	@echo "SUPEROPT_TARS_DIR = $(SUPEROPT_TARS_DIR)"
	@echo "ICC = $(ICC)"

.PHONY: all build linkinstall cleaninstall install test testinit gentest eqtest oopsla_tsvc_prior_bfs oopsla_tsvc_prior_dfs oopsla_tsvc_new_bfs oopsla_tsvc_new_dfs oopsla_lore_mem_write_bfs oopsla_lore_mem_write_dfs oopsla_lore_no_mem_write_bfs oopsla_lore_no_mem_write_dfs run_dietlibc run_paper_ex tableresults tablesummary printpaths 
