#include "eq/function_cg_map.h"
#include "support/debug.h"
#include "eq/cg_with_asm_annotation.h"

using namespace std;

function_cg_map_t::function_cg_map_t(istream& is, context* ctx)
{
  NOT_IMPLEMENTED();
}

void
function_cg_map_t::remove_marker_calls_and_gen_annotated_assembly(ostream& os, string const& asm_filename)
{
  map<asm_insn_id_t, string> insn_id_annotations;
  set<asm_insn_id_t> marker_call_insn_ids;
  map<string, map<string, string>> exit_annotations;

  assembly_file_t assembly_file(asm_filename);

  for (auto const& nc : m_map) {
    cg_with_asm_annotation cg_annot(*nc.second);
    asm_insn_id_t start_asm_insn_id = assembly_file.get_start_asm_insn_id_for_function(nc.first);
    auto t = cg_annot.cg_obtain_marker_call_and_other_annotations(start_asm_insn_id);
    set<asm_insn_id_t> const& f_marker_call_insn_ids = get<0>(t);
    map<asm_insn_id_t, string> const& f_insn_id_annotations = get<1>(t);
    map<string, string> const& f_exit_annotations = get<2>(t);
    set_union(marker_call_insn_ids, f_marker_call_insn_ids);
    map_union(insn_id_annotations, f_insn_id_annotations);
    exit_annotations.insert(make_pair(nc.first, f_exit_annotations));
  }
  assembly_file.asm_file_remove_marker_calls(marker_call_insn_ids);
  assembly_file.asm_file_gen_annotated_assembly(os, insn_id_annotations, exit_annotations);
}
