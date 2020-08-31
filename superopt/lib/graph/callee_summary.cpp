#include "graph/callee_summary.h"
#include "tfg/tfg.h"
#include "expr/memlabel.h"
#include "expr/consts_struct.h"
#include "graph/lr_map.h"
#include "support/timers.h"

namespace eqspace {

pair<memlabel_t, memlabel_t>
callee_summary_t::get_readable_writeable_memlabels(vector<memlabel_t> const &farg_memlabels, memlabel_t const &ml_fcall_bottom) const
{
  int argnum;
  memlabel_t ret_readable, ret_writeable;
  ret_readable.set_to_empty_mem();
  ret_writeable.set_to_empty_mem();
  for (auto ml : m_reads) {
    ASSERT(memlabel_t::memlabel_is_atomic(&ml));
    if ((argnum = ml.memlabel_is_arg()) != -1) {
      memlabel_t ml;
      if (argnum >= 0 && argnum < (int)farg_memlabels.size()) {
        //cout << __func__ << " " << __LINE__ << ": argnum = " << argnum << endl;
        ml = farg_memlabels.at(argnum);
      } else {
        ml = ml_fcall_bottom;
      }
      //cout << __func__ << " " << __LINE__ << ": ml = " << ml.to_string() << endl;
      memlabel_t::memlabels_union(&ret_readable, &ml);
    } else {
      memlabel_t::memlabels_union(&ret_readable, &ml);
    }
  }
  for (auto ml : m_writes) {
    ASSERT(memlabel_t::memlabel_is_atomic(&ml));
    if ((argnum = ml.memlabel_is_arg()) != -1) {
      memlabel_t ml;
      if (argnum >= 0 && argnum < (int)farg_memlabels.size()) {
        //cout << __func__ << " " << __LINE__ << ": argnum = " << argnum << endl;
        ml = farg_memlabels.at(argnum);
      } else {
        ml = ml_fcall_bottom;
      }
      //cout << __func__ << " " << __LINE__ << ": ml = " << ml.to_string() << endl;
      memlabel_t::memlabels_union(&ret_writeable, &ml);
    } else {
      memlabel_t::memlabels_union(&ret_writeable, &ml);
    }
  }
  //ret_writeable.memlabel_eliminate_readonly();
  memlabel_t::memlabels_union(&ret_readable, &ret_writeable);
  //cout << __func__ << " " << __LINE__ << ": ret_readable = " << ret_readable.to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": ret_writeable = " << ret_writeable.to_string() << endl;
  return make_pair(ret_readable, ret_writeable);
}

/*void
callee_summary_t::rename_symbol_ids(map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &from, map<symbol_id_t, tuple<string, size_t, variable_constness_t>> const &to)
{
  //NOT_IMPLEMENTED();
}*/

memlabel_t
callee_summary_t::memlabel_fcall_bottom(consts_struct_t const &cs, set<cs_addr_ref_id_t> const &addr_ref_ids, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map)
{
  memlabel_t ml = memlabel_t::get_memlabel_for_addr_ref_ids(addr_ref_ids, cs, symbol_map, locals_map);
  memlabel_t ml_heap = memlabel_t::memlabel_heap();
  memlabel_t::memlabels_union(&ml, &ml_heap);
  return ml;
}

callee_summary_t
callee_summary_t::callee_summary_bottom(consts_struct_t const &cs, set<cs_addr_ref_id_t> const &addr_ref_ids, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map, size_t num_fargs)
{
  autostop_timer func_timer(__func__);
  //memlabel_t ml = get_memlabel_for_addr_ref_ids(addr_ref_ids, cs, symbol_map, locals_map);
  //memlabel_t ml_heap = memlabel_heap();
  //memlabels_union(&ml, &ml_heap);

  memlabel_t ml = memlabel_fcall_bottom(cs, addr_ref_ids, symbol_map, locals_map);
  callee_summary_t ret;
  //ret.m_is_nop = false;
  for (auto mlar : ml.get_atomic_memlabels()) {
    auto mla = mlar->get_ml();
    if (memlabel_t::memlabel_contains_stack_or_local(&mla)) {
      continue;
    }
    ret.m_reads.insert(mla);
    ret.m_writes.insert(mla);
  }
  for (size_t i = 0; i < num_fargs; i++) {
    ret.m_reads.insert(memlabel_t::memlabel_arg(i));
    ret.m_writes.insert(memlabel_t::memlabel_arg(i));
  }
  return ret;
}

}
