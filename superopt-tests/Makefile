SHELL := /bin/bash

include config-host.mak      # BUILDDIR

# add new dirs' targets here
TSVC_PRIOR_TARGETS := TSVC_prior_work
TSVC_NEW_TARGETS := TSVC_new
LORE_MEM_TARGETS := LORE_mem_write
LORE_NOMEM_TARGETS := LORE_no_mem_write
DIETLIBC_TARGET := dietlibc
PAPER_EX_TARGET := paper_ex
OOPSLA_TARGETS := $(TSVC_PRIOR_TARGETS) $(TSVC_NEW_TARGETS) $(LORE_MEM_TARGETS) $(LORE_NOMEM_TARGETS)
TARGETS := $(OOPSLA_TARGETS) $(DIETLIBC_TARGET) $(PAPER_EX_TARGET)

PARALLEL_LOAD_PERCENT ?= 33
PARALLEL_LOAD_PERCENT_DFS ?= 30

# rules

all: $(BUILDDIR) $(TARGETS)

clean:
	$(foreach t,$(TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) clean || true;)

distclean: clean
	rm -rf config-host.mak $(BUILDDIR)

$(BUILDDIR):
	mkdir -p $@

$(TARGETS):
	mkdir -p $(BUILDDIR)/$@
	cp $@/Makefile -t $(BUILDDIR)/$@
	$(MAKE) -C $(BUILDDIR)/$@

gen_oopsla_test:
	$(foreach t,$(OOPSLA_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) RUN=0 gentest || exit;)
	true > $(BUILDDIR)/all_gentest_chaperon_commands
	$(foreach t,$(OOPSLA_TARGETS), [[ -f $(BUILDDIR)/$(t)/gentest_chaperon_commands ]] && cat $(BUILDDIR)/$(t)/gentest_chaperon_commands >> $(BUILDDIR)/all_gentest_chaperon_commands || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_gentest_chaperon_commands

run_oopsla_test_bfs:
	$(foreach t,$(OOPSLA_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_oopsla
	$(foreach t,$(OOPSLA_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_bfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_bfs >> $(BUILDDIR)/all_chaperon_commands_oopsla || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_chaperon_commands_oopsla

run_oopsla_test_dfs:
	$(foreach t,$(OOPSLA_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_oopsla
	$(foreach t,$(OOPSLA_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_dfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_dfs >> $(BUILDDIR)/all_chaperon_commands_oopsla || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT_DFS)%" < $(BUILDDIR)/all_chaperon_commands_oopsla

run_oopsla_tsvc_prior_dfs:
	$(foreach t,$(TSVC_PRIOR_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_tsvc_prior
	$(foreach t,$(TSVC_PRIOR_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_dfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_dfs >> $(BUILDDIR)/all_chaperon_commands_tsvc_prior || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT_DFS)%" < $(BUILDDIR)/all_chaperon_commands_tsvc_prior

run_oopsla_tsvc_prior_bfs:
	$(foreach t,$(TSVC_PRIOR_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_tsvc_prior
	$(foreach t,$(TSVC_PRIOR_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_bfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_bfs >> $(BUILDDIR)/all_chaperon_commands_tsvc_prior || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_chaperon_commands_tsvc_prior

run_oopsla_tsvc_new_bfs:
	$(foreach t,$(TSVC_NEW_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_tsvc_new
	$(foreach t,$(TSVC_NEW_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_bfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_bfs >> $(BUILDDIR)/all_chaperon_commands_tsvc_new || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_chaperon_commands_tsvc_new

run_oopsla_tsvc_new_dfs:
	$(foreach t,$(TSVC_NEW_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_tsvc_new
	$(foreach t,$(TSVC_NEW_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_dfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_dfs >> $(BUILDDIR)/all_chaperon_commands_tsvc_new || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT_DFS)%" < $(BUILDDIR)/all_chaperon_commands_tsvc_new

run_oopsla_lore_mem_bfs:
	$(foreach t,$(LORE_MEM_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_lore_mem
	$(foreach t,$(LORE_MEM_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_bfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_bfs >> $(BUILDDIR)/all_chaperon_commands_lore_mem || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_chaperon_commands_lore_mem

run_oopsla_lore_mem_dfs:
	$(foreach t,$(LORE_MEM_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_lore_mem
	$(foreach t,$(LORE_MEM_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_dfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_dfs >> $(BUILDDIR)/all_chaperon_commands_lore_mem || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT_DFS)%" < $(BUILDDIR)/all_chaperon_commands_lore_mem

run_oopsla_lore_nomem_bfs:
	$(foreach t,$(LORE_NOMEM_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_lore_nomem
	$(foreach t,$(LORE_NOMEM_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_bfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_bfs >> $(BUILDDIR)/all_chaperon_commands_lore_nomem || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT)%" < $(BUILDDIR)/all_chaperon_commands_lore_nomem

run_oopsla_lore_nomem_dfs:
	$(foreach t,$(LORE_NOMEM_TARGETS),$(MAKE) -C $(BUILDDIR)/$(t) runtest || exit;)
	true > $(BUILDDIR)/all_chaperon_commands_lore_nomem
	$(foreach t,$(LORE_NOMEM_TARGETS), [[ -f $(BUILDDIR)/$(t)/chaperon_commands_dfs ]] && cat $(BUILDDIR)/$(t)/chaperon_commands_dfs >> $(BUILDDIR)/all_chaperon_commands_lore_nomem || exit;)
	parallel --load "$(PARALLEL_LOAD_PERCENT_DFS)%" < $(BUILDDIR)/all_chaperon_commands_lore_nomem

run_dietlibc:
	$(MAKE) -C $(BUILDDIR)/$(DIETLIBC_TARGET) gentest
	$(MAKE) -C $(BUILDDIR)/$(DIETLIBC_TARGET) runtest

run_paper_ex:
	$(MAKE) -C $(BUILDDIR)/$(PAPER_EX_TARGET) gentest
	$(MAKE) -C $(BUILDDIR)/$(PAPER_EX_TARGET) runtest

.PHONY: all clean distclean $(TARGETS) gen_oopsla_test run_oopsla_test_bfs run_oopsla_test_dfs run_oopsla_tsvc_prior_dfs run_oopsla_tsvc_prior_bfs run_oopsla_tsvc_new_bfs run_oopsla_tsvc_new_dfs run_oopsla_lore_mem_bfs run_oopsla_lore_mem_dfs run_oopsla_lore_nomem_bfs run_oopsla_lore_nomem_dfs run_dietlibc run_paper_ex
