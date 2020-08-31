#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <algorithm>
#include <limits.h>
#include "support/cl.h"
//#include "support/c_utils.h"
#include "support/src-defs.h"
#include "support/debug.h"
//#include "rewrite/harvest.h"
#include "codegen/etfg_insn.h"
#include "i386/insntypes.h"
#include "rewrite/dst-insn.h"
#include "rewrite/insn_list.h"
//#include "config.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/jumptable.h"
//#include "strtab.h"
//#include "disas.h"
#include "expr/z3_solver.h"
#include "valtag/elf/elf.h"
#include "graph/prove.h"

#include "rewrite/static_translate.h"
#include "superopt/typestate.h"
//#include "dbg_functions.h"

#include "i386/opctable.h"
#include "valtag/regset.h"
//#include "temporaries.h"
#include "support/utils.h"
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"

#include "i386/insn.h"
#include "ppc/insn.h"
#include "rewrite/src-insn.h"

//#include "live_ranges.h"
#include "ppc/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"

#include "rewrite/edge_table.h"
#include "rewrite/peep_entry_list.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"

#include "equiv/equiv.h"
#include "superopt/superopt.h"
#include "support/timers.h"
#include "ppc/execute.h"
#include "i386/execute.h"
#include "superopt/superoptdb.h"
#include "equiv/bequiv.h"
#include "rewrite/function_pointers.h"
#include "support/globals.h"
#include "equiv/jtable.h"
#include "support/cmd_helper.h"

//#include <caml/mlvalues.h>
//#include <caml/alloc.h>
//#include <caml/memory.h>
//#include <caml/fail.h>
//#include <caml/callback.h>
//#include <caml/custom.h>
//#include <caml/intext.h>

static char as1[40960000], as2[40960], as3[40960];

#define MAX_NUM_TYPESTATES 1024

int
main(int argc, char **argv)
{
  init_timers();
  cmd_helper_init();

  g_ctx_init(); //this should be first, as it involves a fork which can be expensive if done after usedef_init()
  solver_init(); //this also involves a fork and should be as early as possible
  g_query_dir_init();
  src_init();
  dst_init();
  g_se_init();
  //types_init();
  usedef_init();
  types_init();

  cpu_set_log_filename(DEFAULT_LOGFILE);

  equiv_init();

  cl::arg<string> itables_filename(cl::implicit_prefix, "", "filename with itables");
  cl::arg<string> types_filename(cl::implicit_prefix, "", "filename with typestates");
  cl::cl cmd("itables_prune_using_types");
  cmd.add_arg(&itables_filename);
  cmd.add_arg(&types_filename);
  cmd.parse(argc, argv);

  vector<itable_position_t> ipositions;
  vector<itable_t> itables;
  read_itables_from_file(itables_filename.get_value(), ipositions, itables);
  typestate_t *typestate_initial, *typestate_final;
  //ts_init = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_initial = new typestate_t[MAX_NUM_TYPESTATES];
  ASSERT(typestate_initial);
  for (size_t i = 0; i < MAX_NUM_TYPESTATES; i++) {
    typestate_init(&typestate_initial[i]);
  }
  //ts_fin = (typestate_t *)malloc(NUM_FP_STATES * sizeof(typestate_t));
  typestate_final = new typestate_t[MAX_NUM_TYPESTATES];
  ASSERT(typestate_final);
  for (size_t i = 0; i < MAX_NUM_TYPESTATES; i++) {
    typestate_init(&typestate_final[i]);
  }
  cout << __func__ << " " << __LINE__ << ": calling typestate_from_file()\n";
  size_t num_states = typestate_from_file(typestate_initial, typestate_final, MAX_NUM_TYPESTATES, types_filename.get_value().c_str());

  cout << __func__ << " " << __LINE__ << ": calling typestate_canonicalize() in loop\n";
  regmap_t pregmap;
  for (int i = 0; i < num_states; i++) {
    typestate_canonicalize(typestate_initial, typestate_final, &pregmap);
    DBG(TYPESTATE, "pregmap:\n%s\n", regmap_to_string(&pregmap, as1, sizeof as1));
  }
  cout << __func__ << " " << __LINE__ << ": calling itable truncate using typestate in loop\n";
  for (int i = 0; i < itables.size(); i++) {
    ASSERT(num_states > 0);
    cout << __func__ << " " << __LINE__ << ": before itable num entries: " << itables.at(i).get_num_entries() << "\n";
    itable_truncate_using_typestate(&itables[i], &typestate_initial[0],
        &typestate_final[0]);
    itable_typestates_add_integer_temp_if_needed(&itables[i],
        typestate_initial, typestate_final, num_states);
    cout << __func__ << " " << __LINE__ << ": after itable num entries: " << itables.at(i).get_num_entries() << "\n";
  }
  delete [] typestate_initial;
  delete [] typestate_final;
  itables_print(stdout, itables, false);
  solver_kill();
  call_on_exit_function();
  return 0;
}
