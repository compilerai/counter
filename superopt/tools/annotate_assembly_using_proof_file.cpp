#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include "support/debug.h"
#include "config-host.h"
//#include "cpu.h"
//#include "exec-all.h"
#include "rewrite/jumptable.h"
//#include "imm_map.h"
#include "i386/disas.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/ldst_input.h"
#include "rewrite/cfg.h"
#include "rewrite/tagline.h"
#include "expr/z3_solver.h"
#include "cutils/imap_elem.h"
#include "valtag/elf/elf.h"

#include "i386/opctable.h"
#include "support/c_utils.h"
#include "support/utils.h"
#include "cutils/pc_elem.h"

#include "ppc/regs.h"
#include "i386/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "support/globals.h"
#include "ppc/insn.h"
#include "rewrite/src-insn.h"

#include "rewrite/edge_table.h"
#include "support/timers.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "rewrite/static_translate.h"
//#include "tfg/annotate_locals_using_dwarf.h"
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"
//#include "rewrite/harvest.h"
#include "support/mytimer.h"
#include "tfg/parse_input_eq_file.h"
#include "valtag/nextpc.h"
#include "support/cl.h"
#include "rewrite/assembly_handling.h"
#include "eq/corr_graph.h"
#include "eq/cg_with_asm_annotation.h"
#include "eq/function_cg_map.h"

static char as1[409600];
static char as2[1024];
static char as3[1024];

using namespace eqspace;

int
main(int argc, char **argv)
{
  for (int i = 0; i < argc; i++) {
    cout << " " << argv[i];
  }
  cout << endl;

  cl::cl cmd("annotate_assembly_using_proof_file: annotates assembly code using proof file");
  cl::arg<string> proof_file(cl::implicit_prefix, "", "Name of the proof file");
  cmd.add_arg(&proof_file);
  cl::arg<string> asm_filename(cl::implicit_prefix, "", "Name of the assembly .s file");
  cmd.add_arg(&asm_filename);
  cl::arg<string> output_filename(cl::explicit_prefix, "o", "out.s", "Name of the output annotated assembly .s file");
  cmd.add_arg(&output_filename);
  cmd.parse(argc, argv);

  global_timeout_secs = 2000*60*60;
  init_timers();
  g_query_dir_init();

  src_init();
  dst_init();

  g_ctx_init();
  solver_init();
  context* ctx = g_ctx;

  ifstream in(proof_file.get_value());
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": could not open " << proof_file.get_value() << "." << endl;
    exit(1);
  }
  function_cg_map_t function_cg_map(in, ctx);
  ofstream ofs(output_filename.get_value());
  function_cg_map.remove_marker_calls_and_gen_annotated_assembly(ofs, asm_filename.get_value());
  ofs.close();

  //string line;
  //bool end;
  //end = !getline(in, line);
  //ASSERT(!end);
  //ASSERT(string_has_prefix(line, "=result"));
  //shared_ptr<corr_graph> cg = corr_graph::corr_graph_from_stream(in, ctx);

  //shared_ptr<cg_with_asm_annotation> cg_asm_annot = make_shared<cg_with_asm_annotation>(*cg, asm_filename.get_value());

  //ofstream ofs(output_filename.get_value());
  //cg_asm_annot->cg_remove_marker_calls_and_gen_annotated_assembly(ofs);
  //ofs.close();
  return 0;
}
