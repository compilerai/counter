#include "graph/lr_status.h"
#include "gsupport/pc.h"
//#include "graph/tfg.h"
#include "expr/consts_struct.h"
#include "graph/expr_loc_visitor.h"

#include "support/timers.h"

namespace eqspace {

lr_status_ref
mk_lr_status(lr_status_type_t typ, set<cs_addr_ref_id_t> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set)
{
  lr_status_t ret(typ, cs_addr_ref_ids, bottom_set);
  return lr_status_t::get_lr_status_manager()->register_object(ret);
}

lr_status_t::~lr_status_t()
{
  if (m_is_managed) {
    this->get_lr_status_manager()->deregister_object(*this);
  }
}

manager<lr_status_t> *
lr_status_t::get_lr_status_manager()
{
  static manager<lr_status_t> *ret = NULL;
  if (!ret) {
    ret = new manager<lr_status_t>;
  }
  return ret;
}



set<cs_addr_ref_id_t>
lr_status_t::subtract_memlocs_from_addr_ref_ids(set<cs_addr_ref_id_t> const &m_relevant_addr_ref_ids, graph_symbol_map_t const &symbol_map, context *ctx)
{
  //autostop_timer func_timer(__func__);
  consts_struct_t const &cs = ctx->get_consts_struct();
  set<cs_addr_ref_id_t> ret;
  for (auto i : m_relevant_addr_ref_ids) {
    string s = cs.get_addr_ref(i).first;
    bool ignore = false;
    if (string_has_prefix(s, g_symbol_keyword + ".")) {
      symbol_id_t symbol_id = stol(s.substr(g_symbol_keyword.size() + 1));
      if (!symbol_map.count(symbol_id)) {
        cout << __func__ << " " << __LINE__ << ": s = " << s << ", symbol_id = " << symbol_id << endl;
      }
      ASSERT(symbol_map.count(symbol_id));
      if (string_has_prefix(symbol_map.at(symbol_id).get_name()->get_str(), PEEPTAB_MEM_PREFIX)) {
        ignore = true;
      }
    }
    if (!ignore) {
      ret.insert(i);
    }
  }
  return ret;
}

lr_status_ref
lr_status_t::start_pc_value(expr_ref const &e, set<cs_addr_ref_id_t> const &relevant_addr_refs, context *ctx, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map)
{
  //lr_status_t ret;
  //ret.m_lr_status = LR_STATUS_BOTTOM;
  set<cs_addr_ref_id_t> ret_cs_addr_ref_ids;
  set<memlabel_t> ret_bottom_set;
  set<memlabel_t> bottom_set;
  set<string> arg_ids;
  set<cs_addr_ref_id_t> relevant_addr_refs_minus_memlocs = subtract_memlocs_from_addr_ref_ids(relevant_addr_refs, symbol_map, ctx);

  arg_ids = ctx->expr_contains_tfg_argument(e, argument_regs);
  consts_struct_t const &cs = ctx->get_consts_struct();
  for (const auto &arg_id : arg_ids) {
    int argnum = context::argname_get_argnum(arg_id);
    if (argnum != -1) {
      bottom_set.insert(memlabel_t::memlabel_arg(argnum));
    }
  }

  for (auto i : relevant_addr_refs_minus_memlocs) {
    string addr_ref_str = cs.get_addr_ref(i).first;
    //cout << __func__ << " " << __LINE__ << ": addr_ref_str = " << addr_ref_str << endl;
    //cout << __func__ << " " << __LINE__ << ": g_stack_symbol = " << g_stack_symbol << endl;
    if (   addr_ref_str != g_stack_symbol
        && !string_has_prefix(addr_ref_str, g_local_keyword)) {
      ret_cs_addr_ref_ids.insert(i);
    }
  }
  ret_bottom_set = bottom_set;
  ret_bottom_set.insert(memlabel_t::memlabel_heap());
  //cout << __func__ << " " << __LINE__ << ": for e : " << expr_string(e) << ", returning " << ret.lr_status_to_string(cs) << endl;
  //return lr_status_t(LR_STATUS_BOTTOM, ret_cs_addr_ref_ids, ret_bottom_set);
  return mk_lr_status(LR_STATUS_BOTTOM, ret_cs_addr_ref_ids, ret_bottom_set);
}

memlabel_t
lr_status_t::to_memlabel(consts_struct_t const &cs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map) const
{
  if (this->is_top()) {
    memlabel_t ret;
    memlabel_t::keyword_to_memlabel(&ret, G_MEMLABEL_TOP_SYMBOL, MEMSIZE_MAX);
    return ret;
  } else {
    memlabel_t ret = memlabel_t::get_memlabel_for_addr_ref_ids(m_cs_addr_ref_ids, cs, symbol_map, locals_map);
    if (this->is_bottom()) {
      for (auto b : m_bottom_set) {
        memlabel_t::memlabels_union(&ret, &b);
      }
      //ret.m_alias_set.insert(make_tuple(memlabel_t::alias_type_heap, -1, (variable_size_t)-1, false));
    }
    return ret;
  }
}

}
