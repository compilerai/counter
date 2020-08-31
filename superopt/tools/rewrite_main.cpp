#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>

//#include "regset.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "support/src-defs.h"
#include "rewrite/analyze_stack.h"
#include "expr/z3_solver.h"
#include "support/globals.h"
#include "i386/insn.h"
#include "ppc/insn.h"

#if ARCH_SRC == ARCH_PPC
//#include "cpu.h"

#ifdef __APPLE__
#include <crt_externs.h>
# define environ  (*_NSGetEnviron())
#endif

#include "rewrite/ldst_input.h"
#endif
//#include "cpu.h"

#include "i386/insntypes.h"
#include "rewrite/static_translate.h"
#include "rewrite/jumptable.h"
#include "rewrite/peephole.h"
//#include "imm_map.h"
#include "rewrite/transmap.h"
#include "support/timers.h"
//#include "exec-all.h"
#include "valtag/elf/elf.h"
#include "rewrite/translation_instance.h"
#include "superopt/superoptdb.h"
//#include "ldst_exec.h"

static const char *interp_prefix = ""; //CONFIG_QEMU_PREFIX;
//const char *qemu_uname_release = ""; //CONFIG_UNAME_RELEASE;

char const *tmpdir = "/tmp";
char const *udsorted_exec = NULL;

static char as1[4096];

void src_init();
void dst_free();
//void g_ctx_init();

/* XXX: on x86 MAP_GROWSDOWN only works if ESP <= address + 32, so
   we allocate a bigger stack. Need a better solution, for example
   by remapping the process stack directly at the right place */
unsigned long x86_stack_size = 512 * 1024;

#if 0 //ARCH_SRC == ARCH_PPC
static inline uint64_t cpu_ppc_get_tb (CPUPPCState *env)
{
    /* TO FIX */
    return 0;
}
  
uint32_t cpu_ppc_load_tbl (CPUPPCState *env)
{
    return cpu_ppc_get_tb(env) & 0xFFFFFFFF;
}
  
uint32_t cpu_ppc_load_tbu (CPUPPCState *env)
{
    return cpu_ppc_get_tb(env) >> 32;
}
  
static void cpu_ppc_store_tb (CPUPPCState *env, uint64_t value)
{
    /* TO FIX */
}

void cpu_ppc_store_tbu (CPUPPCState *env, uint32_t value)
{
    cpu_ppc_store_tb(env, ((uint64_t)value << 32) | cpu_ppc_load_tbl(env));
}
 
void cpu_ppc_store_tbl (CPUPPCState *env, uint32_t value)
{
    cpu_ppc_store_tb(env, ((uint64_t)cpu_ppc_load_tbl(env) << 32) | value);
}
  
uint32_t cpu_ppc_load_decr (CPUPPCState *env)
{
    /* TO FIX */
    return -1;
}
 
void cpu_ppc_store_decr (CPUPPCState *env, uint32_t value)
{
    /* TO FIX */
}
#endif

static void
usage(void)
{
  printf("rewrite:\n"
      "usage: rewrite [-h] [-d opts] [-prune rowsize] [-f] [-ff] "
      "[-xmm] [-o outfile] [-slice <slice-intervals>] "
      "[-l logfile] [-T] [-t table] [-m] [-p] [-no-aggressive] [-core] "
      "[-M] [-L path] [-s size] [-func function] program [arguments...]\n"
      "Linux CPU emulator (compiled for %s emulation)\n"
      "\n"
      "-server  specify superoptdb server (default superoptdb.cse.iitd.ernet.in)\n"
      "-core	 	dump core\n"
      "-d options   	activate log (options=[tmaps,dom])\n"
      "-f			fast mode\n"
      "-ff			faster mode\n"
      "-debug 	instrument generated executable with debug data\n"
      "-fix-transmaps <rewrite.log> fix the transmaps to those in rewrite.log\n"
      "-full		dump full machine state\n"
      "-func		function needed to be optimized\n"
      "-h           	print this help\n"
      "-l		 	log file\n"
      "-L path      	set the elf interpreter prefix (default=%s)\n"
      "-m			mark peephole translations\n"
      "-M		 	specify track_memlocs\n"
      "-no-aggressive	don't make aggressive assumptions about flags\n"
      "-indir-link 	use indirect branch linking\n"
      "-o		 	output file\n"
      //"-p pagesize  	set the host page size to 'pagesize'\n",
      "-p		 	preserve instruction order (no usedef sort)\n"
      "-profile <name>	profile name (<name>.edges)\n"
      "-prune rowsize	prune the rowsize to rowsize\n"
      "-s size      	set the stack size in bytes (default=%ld)\n"
      "-splice <intervals>  use peep rules only in specified prog-slices\n"
      "-static		generate a statically linked executable\n"
      "-t table     	specify peephole table(\"none\" for no table)\n"
      "-tmpdir <dirname>   directory used to store temporary files\n"
      "-T		 	track time\n"
      "-udsorted_exec <fn> The udsorted exec location\n"
      "-xmm	 	use xmm registers\n"
      "-mode 	[subsetting|stack_analysis|superoptimize|all]\n"
      "-print-stacklocs      print stack locations (stack analysis)\n"
      "-stacklocs <list>     regalloc only these stacklocs\n"
      "\n",
    "ppc",
    interp_prefix, 
    x86_stack_size);
  _exit(1);
}

static translation_mode_t
string_to_translation_mode(char const *string)
{
  int i;
  char buf[64];

  for (i = (int)TRANSLATION_MODE0 + 1; i < END; i++) {
    if (!strcmp(string, translation_mode_to_string((translation_mode_t)i, buf, sizeof buf))) {
      return (translation_mode_t)i;
    }
  }
  NOT_REACHED();
}

int
main(int argc, char **argv)
{
  char const *superoptdb_path = SUPEROPTDB_PATH_DEFAULT;
  char const *server = SUPEROPTDB_SERVER_DEFAULT;
  char const *log_filename = DEFAULT_LOGFILE;
  const char *r, *output_file, *filename;
  char const *peep_trans_table = NULL;
  long min_stack_size_required;
  //bool no_superopt = false;
  translation_mode_t mode = TRANSLATION_MODE0;
  bool use_mode = false;
  int optind, i, ret;
  struct rlimit r1;
  int dump_core = 0;

  min_stack_size_required = 64L * 1024L * 1024L;   // 64MB
  output_file = "a.out";
  init_timers();

  src_init();
  dst_init();
  usedef_init();
  g_ctx_init();

  //equiv_init();

  if (argc <= 1) {
    usage();
  }
  optind = 1;

  for(;;) {
    if (optind >= argc)
      break;
    r = argv[optind];
    if (r[0] != '-') {
      break;
    }
    optind++;
    r++;
    if (!strcmp(r, "-")) {
      break;
    } else if (!strcmp(r, "t")) {
      r = argv[optind++];
      /*if (!strcmp(r, "empty"))
        peep_trans_table = NULL;
      else*/
      peep_trans_table = r;
    /*} else if (!strcmp(r, "fallback")) {
      use_fallback_translation_only = 1;
    } else if (!strcmp(r, "f")) {
      use_fast_mode = 1;
    } else if (!strcmp(r, "ff")) {
      use_fast_mode = 2;*/
    } else if (!strcmp(r, "debug")) {
      dyn_debug = true;
    } else if (!strcmp(r, "fix-transmaps")) {
      r = argv[optind++];
      fix_transmaps_logfile = r;
    } else if (!strcmp(r, "func")) {
      speed_functions[num_speed_functions++] = argv[optind++];
    } else if (!strcmp(r, "full")) {
      dump_full_state = 1;
    } else if (!strcmp(r, "T")) {
      track_time = 1;
    } else if (!strcmp(r, "o")) {
      output_file = argv[optind++];
    } else if (!strcmp(r, "p")) {
      preserve_instruction_order = 1;
    } else if (!strcmp(r, "prune")) {
      prune_row_size = atoi(argv[optind++]);
      ASSERT(prune_row_size > 0);
    } else if (!strcmp(r, "profile")) {
      profile_name = argv[optind++];
    } else if (!strcmp(r, "static")) {
      static_link_flag = 1;
    } else if (!strcmp(r, "tmpdir")) {
      tmpdir = argv[optind++];
    } else if (!strcmp(r, "udsorted_exec")) {
      udsorted_exec = argv[optind++];
    } else if (!strcmp(r, "core")) {
      dump_core = 1;
    } else if (!strcmp(r, "l")) {
      log_filename = argv[optind++];
    } else if (!strcmp(r, "m")) {
      mark_peep_translations = 1;
    } else if (!strcmp(r, "M")) {
      ASSERT(dyn_debug);
      if (optind >= argc) {
        break;
      }
      r = argv[optind++];
      read_track_memlocs(r);
      dbg ("num_track_memlocs=%d\n", num_track_memlocs);
    } else if (!strcmp(r, "no-aggressive")) {
      aggressive_assumptions_about_flags = 0;
    } else if (!strcmp(r, "indir-link")) {
      indir_link_flag = 1;
    } else if (!strcmp(r, "no-calling-conventions")) {
      use_calling_conventions = false;
    } else if (!strcmp(r, "progress")) {
      progress_flag = 1;
    /*} else if (!strcmp(r, "xmm")) {
      use_xmm_regs = 1;*/
    } else if (!strcmp(r, "s")) {
      r = argv[optind++];
      x86_stack_size = strtol(r, (char **)&r, 0);
      if (x86_stack_size <= 0)
        usage();
      if (*r == 'M') {
        x86_stack_size *= 1024 * 1024;
      } else if (*r == 'k' || *r == 'K') {
        x86_stack_size *= 1024;
      }
    } else if (!strcmp(r, "splice")) {
      r = argv[optind++];
      parse_splice_string(r);
    } else if (!strcmp(r, "L")) {
      interp_prefix = argv[optind++];
    /*} else if (!strcmp(r, "r")) {
      qemu_uname_release = argv[optind++];*/
    /*} else if (!strcmp(r, "i386_base_reg")) {
      i386_base_reg = atoi(argv[optind++]);
      ASSERT (i386_base_reg == -1
          || (i386_base_reg >= 0 && i386_base_reg < I386_NUM_GPRS));

      if (i386_base_reg != -1) {
        int i;
        char used[I386_NUM_GPRS];

        memset(used, 0x0, sizeof used);
        for (i = 0; i < i386_num_available_regs; i++) {
          ASSERT(   i386_available_regs[i] >= 0
                 && i386_available_regs[i] < i386_num_available_regs);
          used[i386_available_regs[i]] = 1;
        }
        if (used[i386_base_reg]) {
          int i;
          int unused_reg = -1;
          for (i = 0; i < I386_NUM_GPRS; i++)
            if (!used[i]) unused_reg = i;
          if (unused_reg == -1) {
            err("Cannot set i386_base_reg to %d when all registers have "
                "been used up. x86_num_available_Regs=%d\n",
                i386_base_reg, i386_num_available_regs);
            ASSERT(0);
          }
          for (i = 0; i < i386_num_available_regs; i++)
            if (i386_available_regs[i]==i386_base_reg)
              i386_available_regs[i] = unused_reg;
        }
      }
    } else if (!strcmp(r, "i386_available_regs")) {
      int i;
      int r = atoi(argv[optind++]);
      ASSERT (r <= i386_num_available_regs);
      i386_num_available_regs = 0;
      for (i = 0; i < r; i++) {
        if (i == i386_base_reg) continue;
        i386_available_regs[i386_num_available_regs++] = i;
      }
      if (i386_num_available_regs != r) {
        err("Could not find %d distinct x86 available regs. "
            "i386_base_reg = %d\n", r, i386_base_reg);
        ASSERT(0);
      }*/
    } else if (!strcmp(r, "mode")) {
      char const *modestr = argv[optind++];
      mode = string_to_translation_mode(modestr);
      use_mode = true;
    /*} else if (!strcmp(r, "no-superopt")) {
      no_superopt = true;*/
    /*} else if (!strcmp(r, "print-stacklocs")) {
      print_stacklocs = true;
    } else if (!strcmp(r, "stacklocs")) {
      r = argv[optind++];
      parse_stacklocs_string(r);*/
    } else if (!strcmp(r, "superoptdb")) {
      server = argv[optind++];
    } else if (!strcmp(r, "superoptdb_path")) {
      superoptdb_path = argv[optind++];
    } else {
      usage();
    }
  }
  if (optind >= argc) {
    usage();
  }
  filename = argv[optind];

  if (!peep_trans_table && !superoptdb_init(server, superoptdb_path)) {
    exit(1);
  }

  /*if (use_fallback_translation_only) {
    use_fast_mode = 2;
  }*/

  if (dump_core) {
    struct rlimit core_size = { RLIM_INFINITY, RLIM_INFINITY };
    setrlimit (RLIMIT_CORE, &core_size);
  }

  ret = getrlimit(RLIMIT_STACK, &r1);
  ASSERT(ret == 0);
  if (r1.rlim_cur < min_stack_size_required) {
    printf("Increasing stack limit from %ld to %ld\n", (long)r1.rlim_cur,
        (long)min_stack_size_required);
    r1.rlim_cur = min_stack_size_required;
    ret = setrlimit(RLIMIT_STACK, &r1);
    ASSERT(ret == 0);
  }

  char out_filename[strlen(filename)+128];
  out_filename[0] = '\0';
  if (!udsorted_exec) {
    snprintf(out_filename, sizeof out_filename, "%s/%s.out", tmpdir,
        strip_path(filename));
  } else {
    snprintf (out_filename, sizeof out_filename, "%s", udsorted_exec);
  }
  ASSERT(out_filename[0] != '\0');

  /*static char command[1024];
  snprintf (command, sizeof command, "cp -f %s %s", filename, out_filename);
  if (system(command) == -1) {
		printf("system(%s) failed: %s\n", command, strerror(errno));
	}

  snprintf (command, sizeof command, "chmod 755 %s", out_filename);
  if (system(command) == -1) {
		printf("system(%s) failed: %s\n", command, strerror(errno));
	}*/
  raw_copy(out_filename, filename);
  chmod(out_filename, 0755);

#define MAX_MODES 256
  translation_mode_t modes[MAX_MODES];
  translation_instance_t ti(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity());
  int iters[MAX_MODES];
  int num_modes;

  translation_instance_init(&ti, SUPEROPTIMIZE);
  translation_instance_free(&ti);

  num_modes = 0;
  if (use_mode) {
    iters[num_modes] = 1;
    modes[num_modes++] = mode;
  } else {
#if ARCH_SRC == ARCH_I386
    //iters[num_modes] = 1;
    //modes[num_modes++] = SUBSETTING;
    iters[num_modes] = 1;
    modes[num_modes++] = SUPEROPTIMIZE;
    /*iters[num_modes] = 1;
    modes[num_modes++] = UNROLL_LOOPS;
    if (!no_superopt) {
      iters[num_modes] = 3;
      modes[num_modes++] = SUPEROPTIMIZE;
    }*/
#else
    iters[num_modes] = 1;
    modes[num_modes++] = SUPEROPTIMIZE;
#endif
  }

  char out[strlen(output_file) + 256];
  char in[strlen(out_filename) + strlen(output_file) + 256];
  char mode_string[256];
  int m;
  snprintf(in, sizeof in, "%s", out_filename);
  strncpy(out, in, sizeof out);
  for (m = 0; m < num_modes; m++) {
    translation_mode_t mode;
    int i;

    mode = modes[m];
    for (i = 0; i < iters[m]; i++) {
      snprintf(mode_string, sizeof mode_string, "%s%d.%d",
          translation_mode_to_string(mode, as1, sizeof as1), m, i);
      char log_mode_filename[strlen(log_filename) + strlen(mode_string) + 64];
      snprintf(log_mode_filename, sizeof log_mode_filename, "%s.%s", log_filename,
          mode_string);
      cpu_set_log_filename(log_mode_filename);

      printf("Performing: %s [%d] . . .\n", mode_string, i);
      translation_instance_init(&ti, mode);

      if (peep_trans_table) {
        read_peep_trans_table(&ti, &ti.peep_table, peep_trans_table, NULL);
      } else {
        PROGRESS("%s", "Loading peeptab");
        peep_table_load(&ti);
      }
      snprintf(out, sizeof out, "%s.%s", output_file, mode_string);
      static_translate(&ti, in, out);
      snprintf(in, sizeof in, "%s", out);
      translation_instance_free(&ti);
      raw_copy(log_filename, log_mode_filename);
    }
  }
  ASSERT(out[0] != '\0');
  raw_copy(output_file, out);
  chmod(output_file, 0755);

  dst_free();
  /*snprintf(command, sizeof command, "rm -f %s\n", output_file);
  system(command);
  snprintf(command, sizeof command, "cp -f %s.%s %s\n", output_file,
      translation_mode_to_string(mode - 1, as1, sizeof as1), output_file);
  system(command);
  snprintf(command, sizeof command, "chmod +x %s\n", output_file);
  system(command); */
  /* snprintf (command, sizeof command, "objdump.ppc -d %s > %s.as",
      out_filename, out_filename);
     xsystem(command); */
  solver_kill();
  call_on_exit_function();
  return 0;
}
