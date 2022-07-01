include Make.conf

export SUPEROPT_PROJECT_DIR ?= $(PWD)
export SUPEROPT_INSTALL_DIR ?= $(SUPEROPT_PROJECT_DIR)/usr/local
SUPEROPT_INSTALL_FILES_DIR ?= $(SUPEROPT_INSTALL_DIR)
SUPEROPT_PROJECT_BUILD = $(SUPEROPT_PROJECT_DIR)/build
# PARALLEL_LOAD_PERCENT ?= 100  # parallel will start new jobs until number of processes fall below this value

SHELL := /bin/bash
export SUPEROPT_TARS_DIR ?= $(SUPEROPT_PROJECT_DIR)/tars
#Z3=z3-4.8.10
Z3=z3-4.8.14
#Z3_PKGNAME=$(Z3)-x64-ubuntu-18.04
Z3_PKGNAME=$(Z3)-x64-glibc-2.31
#Z3_PATH=$(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3/$(Z3_PKGNAME)
Z3_DIR=$(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3
Z3_BINPATH=$(Z3_DIR)/${Z3_PKGNAME}
#Z3_LIB_PATH=$(Z3_PATH)/bin
Z3_LIB_PATH=$(Z3_BINPATH)/bin

Z3v487=z3-4.8.7
Z3v487_DIR=$(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/z3v487
Z3v487_BINPATH=$(Z3v487_DIR)/usr

MAJOR_VERSION=0
MINOR_VERSION=1
PACKAGE_REVISION=0
PACKAGE_NAME=qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)

build::
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR)/binlibs
	cd $(SUPEROPT_PROJECT_DIR)/superopt && ./configure && cd -
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR)/superopt libs
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR)/llvm-project install
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR)/llvm-project
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR)/superoptdbs
	# build qcc, ooelala, clang12
	#$(MAKE) -C $(SUPEROPT_PROJECT_DIR) $(SUPEROPT_PROJECT_DIR)/build/qcc $(SUPEROPT_PROJECT_DIR)/build/ooelala $(SUPEROPT_PROJECT_DIR)/build/clang12
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR) $(SUPEROPT_PROJECT_DIR)/build/clang12
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR) cleaninstall
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR) linkinstall
	#cd $(SUPEROPT_PROJECT_DIR)/superopt-tests && ./configure && make && cd -

docker-build::
	docker build -t static-persist .



install::
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR) cleaninstall
	$(MAKE) -C $(SUPEROPT_PROJECT_DIR) linkinstall

add_compilerai_server_user::
	sudo $(SUPEROPT_PROJECT_DIR)/compiler.ai-scripts/add-user-script.sh compilerai-server compiler.ai123

install_compilerai_server::
	sudo bash compiler.ai-scripts/afterInstall.sh

start_compilerai_server::
	sudo bash compiler.ai-scripts/startApp.sh

stop_compilerai_server::
	sudo bash compiler.ai-scripts/stopApp.sh

compiler_explorer_preload_files:: # called from afterInstall.sh
	mkdir -p compiler.ai-scripts/compiler-explorer/lib/storage/data/eqcheck_preload
	cp superopt-tests/build/TSVC_prior_work/*.xml superopt-tests/build/TSVC_new/*.xml compiler.ai-scripts/compiler-explorer/lib/storage/data/eqcheck_preload

$(SUPEROPT_PROJECT_BUILD)/qcc: Make.conf Makefile
	mkdir -p $(SUPEROPT_PROJECT_BUILD)
	echo "$(SUPEROPT_INSTALL_DIR)/bin/clang-qcc $(CLANG_I386_EQCHECKER_FLAGS)" '$$*' > $@
	chmod +x $@

$(SUPEROPT_PROJECT_BUILD)/ooelala: Make.conf Makefile
	mkdir -p $(SUPEROPT_PROJECT_BUILD)
	echo "$(SUPEROPT_INSTALL_DIR)/bin/clang --dyn_debug=disableSemanticAA -Xclang -load -Xclang $(SUPEROPT_INSTALL_DIR)/lib/UnsequencedAliasVisitor.so -Xclang -add-plugin -Xclang unseq " '$$*' > $@
	chmod +x $@

$(SUPEROPT_PROJECT_BUILD)/clang12: Make.conf Makefile
	mkdir -p $(SUPEROPT_PROJECT_BUILD)
	echo "$(SUPEROPT_INSTALL_DIR)/bin/clang --dyn_debug=disableSemanticAA " '$$*' > $@
	chmod +x $@

linkinstall::
	mkdir -p $(SUPEROPT_INSTALL_DIR)/bin
	mkdir -p $(SUPEROPT_INSTALL_DIR)/include
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-link $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-dis $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-as $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/opt $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llc $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/binutils-2.21-install/bin/ld $(SUPEROPT_INSTALL_DIR)/bin/qcc-ld
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/binutils-2.21-install/bin/as $(SUPEROPT_INSTALL_DIR)/bin/qcc-as
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eq $(SUPEROPT_INSTALL_DIR)/bin/eq32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eqgen $(SUPEROPT_INSTALL_DIR)/bin/eqgen32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/qcc-codegen $(SUPEROPT_INSTALL_DIR)/bin/qcc-codegen32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/codegen $(SUPEROPT_INSTALL_DIR)/bin/codegen32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/debug_gen $(SUPEROPT_INSTALL_DIR)/bin/debug_gen32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/ctypecheck $(SUPEROPT_INSTALL_DIR)/bin/ctypecheck32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/i386_i386/harvest $(SUPEROPT_INSTALL_DIR)/bin/harvest32
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/smt_helper_process $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/qd_helper_process $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/eq $(SUPEROPT_INSTALL_DIR)/bin/eq
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/eqgen $(SUPEROPT_INSTALL_DIR)/bin/eqgen
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/qcc-codegen $(SUPEROPT_INSTALL_DIR)/bin/qcc-codegen
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/codegen $(SUPEROPT_INSTALL_DIR)/bin/codegen
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/debug_gen $(SUPEROPT_INSTALL_DIR)/bin/debug_gen
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/x64_x64/harvest $(SUPEROPT_INSTALL_DIR)/bin/harvest
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/ctypecheck $(SUPEROPT_INSTALL_DIR)/bin/ctypecheck
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm2tfg $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang $(SUPEROPT_INSTALL_DIR)/bin/clang-qcc
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang $(SUPEROPT_INSTALL_DIR)/bin/clang
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang++ $(SUPEROPT_INSTALL_DIR)/bin/clang++
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/harvest-dwarf $(SUPEROPT_INSTALL_DIR)/bin/harvest-dwarf
	ln -sf $(SUPEROPT_PROJECT_DIR)/llvm-project/build/lib $(SUPEROPT_INSTALL_DIR)
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libLockstepDbg.a $(SUPEROPT_INSTALL_DIR)/lib/libLockstepDbg32.a
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/libLockstepDbg.a $(SUPEROPT_INSTALL_DIR)/lib/libLockstepDbg.a
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libmymalloc.a $(SUPEROPT_INSTALL_DIR)/lib
	ln -sf $(SUPEROPT_PROJECT_DIR)/superoptdbs $(SUPEROPT_INSTALL_DIR)
	ln -sf $(Z3_BINPATH)/bin/z3 $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(Z3_LIB_PATH)/libz3.so* $(SUPEROPT_INSTALL_DIR)/lib
	ln -sf $(Z3_BINPATH)/include/z3_*.h $(SUPEROPT_INSTALL_DIR)/include
	ln -sf $(Z3v487_BINPATH)/bin/z3 $(SUPEROPT_INSTALL_DIR)/bin/z3v487
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/yices_smt2 $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/cvc4 $(SUPEROPT_INSTALL_DIR)/bin
	#ln -sf $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/boolector $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/build/qcc $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/build/ooelala $(SUPEROPT_INSTALL_DIR)/bin
	ln -sf $(SUPEROPT_PROJECT_DIR)/build/clang12 $(SUPEROPT_INSTALL_DIR)/bin

cleaninstall::
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm-link
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm-dis
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm-as
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/opt
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/llc
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc-ld
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc-as
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/eq
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/eqgen
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc-codegen
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/codegen
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/debug_gen
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/smt_helper_process
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/qd_helper_process
	rm -f $(SUPEROPT_INSTALL_DIR)/lib/libLockstepDbg.a
	rm -f $(SUPEROPT_INSTALL_DIR)/lib/libmymalloc.a
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/harvest
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/llvm2tfg
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/clang-qcc
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/clang
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/clang++
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/harvest-dwarf
	#rm -f $(SUPEROPT_INSTALL_DIR)/bin/harvest-dwarf32
	rm -rf $(SUPEROPT_INSTALL_DIR)/lib
	rm -rf $(SUPEROPT_INSTALL_DIR)/superoptdbs
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/yices_smt2
	#rm -f $(SUPEROPT_INSTALL_DIR)/bin/boolector
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/cvc4
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/qcc
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/ooelala
	rm -f $(SUPEROPT_INSTALL_DIR)/bin/clang12
	#rm -f $(SUPEROPT_PROJECT_BUILD)/qcc
	#rm -f $(SUPEROPT_PROJECT_BUILD)/ooelala
	#rm -f $(SUPEROPT_PROJECT_BUILD)/clang12

release::
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/bin
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/lib
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/superoptdbs/etfg_i386
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/superoptdbs/i386_i386
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/superoptdbs/etfg_x64
	mkdir -p $(SUPEROPT_INSTALL_FILES_DIR)/superoptdbs/i386_x64
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-link $(SUPEROPT_INSTALL_FILES_DIR)/bin/llvm-link
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-dis $(SUPEROPT_INSTALL_FILES_DIR)/bin/llvm-dis
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm-as $(SUPEROPT_INSTALL_FILES_DIR)/bin/llvm-as
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/opt $(SUPEROPT_INSTALL_FILES_DIR)/bin/opt
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llc $(SUPEROPT_INSTALL_FILES_DIR)/bin/llc
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/binutils-2.21-install/bin/ld $(SUPEROPT_INSTALL_FILES_DIR)/bin/qcc-ld
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/binutils-2.21-install/bin/as $(SUPEROPT_INSTALL_FILES_DIR)/bin/qcc-as
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eq $(SUPEROPT_INSTALL_FILES_DIR)/bin/eq32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/eqgen $(SUPEROPT_INSTALL_FILES_DIR)/bin/eqgen32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/qcc-codegen $(SUPEROPT_INSTALL_FILES_DIR)/bin/qcc-codegen32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/codegen $(SUPEROPT_INSTALL_FILES_DIR)/bin/codegen32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/debug_gen $(SUPEROPT_INSTALL_FILES_DIR)/bin/debug_gen32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/ctypecheck $(SUPEROPT_INSTALL_FILES_DIR)/bin/ctypecheck32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/eq $(SUPEROPT_INSTALL_FILES_DIR)/bin/eq
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/eqgen $(SUPEROPT_INSTALL_FILES_DIR)/bin/eqgen
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/qcc-codegen $(SUPEROPT_INSTALL_FILES_DIR)/bin/qcc-codegen
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/codegen $(SUPEROPT_INSTALL_FILES_DIR)/bin/codegen
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/debug_gen $(SUPEROPT_INSTALL_FILES_DIR)/bin/debug_gen
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/ctypecheck $(SUPEROPT_INSTALL_FILES_DIR)/bin/ctypecheck
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libLockstepDbg.a $(SUPEROPT_INSTALL_FILES_DIR)/lib/libLockstepDbg32.a
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/libLockstepDbg.a $(SUPEROPT_INSTALL_FILES_DIR)/lib/libLockstepDbg.a
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_i386/libmymalloc.a $(SUPEROPT_INSTALL_FILES_DIR)/lib
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/i386_i386/harvest $(SUPEROPT_INSTALL_FILES_DIR)/bin/harvest32
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/smt_helper_process $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/etfg_x64/qd_helper_process $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/x64_x64/harvest $(SUPEROPT_INSTALL_FILES_DIR)/bin/harvest
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/llvm2tfg $(SUPEROPT_INSTALL_FILES_DIR)/bin/llvm2tfg
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang $(SUPEROPT_INSTALL_FILES_DIR)/bin/clang-qcc
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang $(SUPEROPT_INSTALL_FILES_DIR)/bin/clang
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/clang++ $(SUPEROPT_INSTALL_FILES_DIR)/bin/clang++
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/bin/harvest-dwarf $(SUPEROPT_INSTALL_FILES_DIR)/bin/harvest-dwarf
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/llvm-project/build/lib $(SUPEROPT_INSTALL_FILES_DIR)/
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superoptdbs $(SUPEROPT_INSTALL_FILES_DIR)
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/yices_smt2 $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/superopt/build/third_party/cvc4 $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/build/qcc $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/build/ooelala $(SUPEROPT_INSTALL_FILES_DIR)/bin
	rsync -Lrtv $(SUPEROPT_PROJECT_DIR)/build/clang12 $(SUPEROPT_INSTALL_FILES_DIR)/bin
	cd /tmp && tar xf $(SUPEROPT_TARS_DIR)/$(Z3_PKGNAME).zip && rsync -Lrtv usr/ $(SUPEROPT_INSTALL_FILES_DIR) && cd -
	rsync -Lrtv $(SUPEROPT_INSTALL_FILES_DIR)/* $(SUPEROPT_INSTALL_DIR)

ci::
	$(MAKE) ci_install
	$(MAKE) ci_test

test::
	$(MAKE) testinit
	$(MAKE) gentest
	$(MAKE) eqtest

ci::
	$(MAKE) ci_install
	$(MAKE) test

oldbuild::
	# unzip dbs
	$(MAKE) -C superoptdbs
	# build superopt
	pushd superopt && ./configure --use-ninja && popd;
	$(MAKE) -C superopt solvers
	cmake --build superopt/build/etfg_i386 --target eq
	cmake --build superopt/build/etfg_i386 --target smt_helper_process
	cmake --build superopt/build/etfg_i386 --target qd_helper_process
	cmake --build superopt/build/etfg_i386 --target eqgen
	cmake --build superopt/build/etfg_i386 --target qcc-codegen
	cmake --build superopt/build/etfg_i386 --target codegen
	cmake --build superopt/build/etfg_i386 --target debug_gen
	cmake --build superopt/build/i386_i386 --target harvest
	cmake --build superopt/build/etfg_i386 --target prove_using_local_sprel_expr_guesses
	cmake --build superopt/build/etfg_i386 --target update_invariant_state_over_edge
	cmake --build superopt/build/etfg_x64 --target eq
	cmake --build superopt/build/etfg_x64 --target smt_helper_process
	cmake --build superopt/build/etfg_x64 --target qd_helper_process
	cmake --build superopt/build/etfg_x64 --target eqgen
	cmake --build superopt/build/etfg_x64 --target qcc-codegen
	cmake --build superopt/build/etfg_x64 --target codegen
	cmake --build superopt/build/etfg_x64 --target debug_gen
	cmake --build superopt/build/x64_x64 --target harvest
	cmake --build superopt/build/etfg_x64 --target prove_using_local_sprel_expr_guesses
	cmake --build superopt/build/etfg_x64 --target update_invariant_state_over_edge
	# build our llvm fork and custom llvm-based libs and utils
	pushd llvm-project && $(MAKE) install install && $(MAKE) all && popd
	# build qcc
	$(MAKE) $(SUPEROPT_PROJECT_BUILD)/qcc
	# build ooelala
	$(MAKE) $(SUPEROPT_PROJECT_BUILD)/ooelala
	# build clang12
	$(MAKE) $(SUPEROPT_PROJECT_BUILD)/clang12

ci_install::
	$(MAKE) oldbuild
	$(MAKE) release

ci_test::
	$(MAKE) testinit
	$(MAKE) gentest
	$(MAKE) eqtest

# multiple steps for jenkins pipeline view
testinit::
	#pushd superopt-tests && ./configure && (make clean; true) && make && popd
	pushd superopt-tests && ./configure && $(MAKE) && popd

gentest::
	$(MAKE) -C superopt-tests gentest

eqtest::
	$(MAKE) -C superopt-tests runtest

oopsla_test::
	$(MAKE) gen_oopsla_test
	$(MAKE) eq_oopsla_test

gen_oopsla_test::
	$(MAKE) -C superopt-tests gen_oopsla_test

eq_oopsla_test::
	$(MAKE) -C superopt-tests run_oopsla_test

typecheck_test::
	$(MAKE) -C superopt-tests typecheck_test

codegen_test::
	$(MAKE) -C superopt-tests codegen_test

#install::
#	$(MAKE) oldbuild
#	$(MAKE) linkinstall

debian::
	$(info Checking if SUPEROPT_INSTALL_DIR is equal to /usr/local)
	@if [ "$(SUPEROPT_INSTALL_DIR)" = "/usr/local" ]; then\
		echo "yes";\
		rm -rf qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION);\
		mkdir -p qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/DEBIAN;\
		cp DEBIAN.control qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/DEBIAN/control;\
		mkdir -p qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/usr/local;\
		cp -r $(SUPEROPT_INSTALL_FILES_DIR)/* qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/usr/local;\
		strip qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/usr/local/bin/*;\
		strip qcc_$(MAJOR_VERSION).$(MINOR_VERSION)-$(PACKAGE_REVISION)/usr/local/lib/*;\
		dpkg-deb --build $(PACKAGE_NAME);\
		echo "$(PACKAGE_NAME) created successfully. Use 'sudo apt install $(PACKAGE_NAME).deb' to install";\
	else\
		echo "Rebuild with SUPEROPT_INSTALL_DIR=/usr/local to create a debian package";\
	fi

printpaths:
	@echo "SUPEROPT_PROJECT_DIR = $(SUPEROPT_PROJECT_DIR)"
	@echo "SUPEROPT_INSTALL_DIR = $(SUPEROPT_INSTALL_DIR)"
	@echo "SUPEROPT_INSTALL_FILES_DIR = $(SUPEROPT_INSTALL_FILES_DIR)"
	@echo "SUPEROPT_PROJECT_BUILD = $(SUPEROPT_PROJECT_BUILD)"
	@echo "SUPEROPT_TARS_DIR = $(SUPEROPT_TARS_DIR)"
	@echo "ICC = $(ICC)"

pushdebian::
	scp $(PACKAGE_NAME).deb sbansal@xorav.com:

clean::
	rm -rf $(SUPEROPT_PROJECT_DIR)/superopt/build
	rm -rf $(SUPEROPT_INSTALL_DIR)
	rm -rf $(SUPEROPT_PROJECT_DIR)/llvm-project/build

.PHONY: all build ci install ci_install testinit gentest eqtest printpaths
