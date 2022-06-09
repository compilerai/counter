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
#include "support/cl.h"

//#include "regset.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "support/src-defs.h"
#include "rewrite/analyze_stack.h"
#include "support/globals.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "eq/parse_input_eq_file.h"

#if ARCH_SRC == ARCH_PPC
//#include "cpu.h"

#ifdef __APPLE__
#include <crt_externs.h>
# define environ  (*_NSGetEnviron())
#endif

#include "valtag/ldst_input.h"
#endif
//#include "cpu.h"

#include "i386/insntypes.h"
#include "rewrite/static_translate.h"
#include "insn/jumptable.h"
#include "rewrite/peephole.h"
//#include "imm_map.h"
#include "valtag/transmap.h"
#include "support/timers.h"
//#include "exec-all.h"
#include "valtag/elf/elf.h"
#include "rewrite/translation_instance.h"
#include "superopt/superoptdb.h"
//#include "ldst_exec.h"

int
main(int argc, char **argv)
{
  cl::arg<string> tmaps_file(cl::implicit_prefix, "", "path to .tmaps file");
  cl::arg<bool> cost_only(cl::explicit_flag, "c", false, "report the transmap translation cost");
  cl::cl cmd("Transmap translation cost and insns");
  cmd.add_arg(&tmaps_file);
  cmd.add_arg(&cost_only);
  cmd.parse(argc, argv);
  DYN_DBG_SET(tmap_trans, 3);
  string in_filename_str = tmaps_file.get_value();
  char const *in_filename = in_filename_str.c_str();
  dst_init();
  state *init_state = NULL;
  ifstream ifs(in_filename);
  string first_line;
  getline(ifs, first_line);
  g_ctx_init();
  if (first_line == "=StartState:") {
    map<string_ref, expr_ref> value_expr_map;
    eqspace::state::read_state(ifs, value_expr_map, g_ctx/*, NULL*/);
    init_state = new state;
    init_state->set_value_expr_map(value_expr_map);
    init_state->populate_reg_counts(/*NULL*/);
  }
  ifs.close();
  FILE *in_file = fopen(in_filename, "r");
  ASSERT(in_file);
//#define OUT_TMAPS_SIZE 1
  transmap_t in_tmap1, in_tmap2;
  //transmap_t out_tmaps1[OUT_TMAPS_SIZE];
  //transmap_t out_tmaps2[OUT_TMAPS_SIZE];
  long num_out_tmaps1, num_out_tmaps2;
  static char line[1024];
  do {
    if (fscanf(in_file, "%[^\n]\n", line) == EOF) {
      NOT_REACHED();
    }
  } while (strcmp(line, "=="));
  fscanf_transmaps(in_file, &in_tmap1, NULL/*out_tmaps1*/, 0/*OUT_TMAPS_SIZE*/, NULL/*&num_out_tmaps1*/);
  fscanf_transmaps(in_file, &in_tmap2, NULL/*out_tmaps2*/, 0/*OUT_TMAPS_SIZE*/, NULL/*&num_out_tmaps2*/);
  //ASSERT(num_out_tmaps1 == 0);
  //ASSERT(num_out_tmaps2 == 0);
  fclose(in_file);
  if (!cost_only.get_value()) {
    dst_insn_t ibuf[128];
    long num_insns = transmap_translation_insns(&in_tmap1, &in_tmap2, init_state, ibuf, sizeof ibuf, R_ESP, fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), I386_AS_MODE_32);
    char as1[4096];
    printf("init_state:\n%s\n", init_state ? init_state->state_to_string_for_eq().c_str() : "(null)");
    printf("tmap1:\n%s\n", transmap_to_string(&in_tmap1, as1, sizeof as1));
    printf("tmap2:\n%s\n", transmap_to_string(&in_tmap2, as1, sizeof as1));
    printf("insns:\n%s\n", dst_iseq_to_string(ibuf, num_insns, as1, sizeof as1));
  }
  cout << "cost: " << transmap_translation_cost(&in_tmap1, &in_tmap2, true) << endl;
}
