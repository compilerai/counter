#pragma once

#include <stdio.h>
//struct context;

extern char const *profile_name;
extern bool indir_link_flag;
extern bool use_calling_conventions;
//extern int src_stack_gpr;
//extern int dst_stack_gpr;
extern int gas_inited;

extern FILE *logfile;
//extern struct context *g_ctx;
extern char const *g_exec_name;
//extern bool running_as_fbgen;
extern bool running_as_peepgen;
//extern bool running_as_eq;
//extern int g_helper_pid;
//extern char const *peeptab_filename;
extern bool g_correl_locals_flag;
extern bool g_correl_locals_try_all_possibilities;
extern bool g_correl_locals_consider_all_local_variables;

namespace eqspace {
extern unsigned g_page_size_for_safety_checks;
}
