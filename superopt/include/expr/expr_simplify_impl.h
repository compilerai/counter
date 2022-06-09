#include "expr/expr_simplify.h"

#include "gsupport/precond.h"
#include "gsupport/predicate.h"

namespace eqspace {

template<typename T_PRED, typename T_PRECOND>
class expr_simplify_select_on_store_visitor : public expr_visitor
{
  using T_PRED_REF = shared_ptr<T_PRED const>;
  public:
  expr_simplify_select_on_store_visitor(context* ctx, expr_ref const &mem, expr_ref const& mem_alloc, memlabel_t const &memlabel, expr_ref const &addr, unsigned nbytes, bool bigendian, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<T_PRED_REF> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map) : m_ctx(ctx), m_mem(mem), m_mem_alloc(mem_alloc), m_memlabel(memlabel), m_addr(addr), m_nbytes(nbytes), m_bigendian(bigendian), /*m_comment(comment), */m_semantic_simplification(semantic_simplification), m_lhs_expr(lhs_expr), m_relevant_memlabels(relevant_memlabels), m_lhs_set(lhs_set), m_precond(precond), m_sprel_map_pair(sprel_map_pair), m_symbol_map(symbol_map), m_locals_map(locals_map), m_cs(ctx->get_consts_struct())
  {
    //m_visit_each_expr_only_once = true;
    visit_recursive(m_mem);
  }
  expr_ref get_result() const {
    //expr_ref ret = m_mem_map.at(m_mem->get_id());
    expr_ref new_data = m_data_map.at(m_mem->get_id());
    //ASSERT(ret.get_ptr());
    ASSERT(new_data);
    return new_data;
    /*if (new_data.get_ptr()) {
      return new_data;
      } else if ((&m_cs != NULL) && ret == m_cs.get_memzero()) {
      return m_ctx->mk_zerobv(m_nbytes * BYTE_LEN);
      } else {
      return m_ctx->mk_select(ret, m_memlabel, m_addr, m_nbytes, m_bigendian, m_comment);
      }*/
  }
  private:
  virtual void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args)
  {
    ASSERT(e->is_array_sort());
    if (e->is_var()) {
      num_interesting_args = 0;
    } else if (e->is_const()) {
      num_interesting_args = 0;
    } else if (e->get_operation_kind() == expr::OP_STORE) {
      interesting_args[0] = OP_STORE_ARGNUM_MEM;
      num_interesting_args = 1;
    } else if (e->get_operation_kind() == expr::OP_STORE_UNINIT) {
      interesting_args[0] = OP_STORE_UNINIT_ARGNUM_MEM;
      num_interesting_args = 1;
    } else if (e->get_operation_kind() == expr::OP_MEMMASK) {
      interesting_args[0] = OP_MEMMASK_ARGNUM_MEM;
      num_interesting_args = 1;
      //} else if (e->get_operation_kind() == expr::OP_MEMSPLICE) {
      //  size_t j = 0;
      //  for (size_t i = 0; i < e->get_args().size(); i+=2, j++) {
      //    interesting_args[j] = i + 1;
      //  }
      //  num_interesting_args = j; //e->get_args().size();
      //} else if (e->get_operation_kind() == expr::OP_ALLOCA) {
      //  interesting_args[0] = 0;
      //  num_interesting_args = 1;
  } else if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
    /*if (expr_func_has_array_output(e->get_args()[0])) {
      ASSERT(OP_FUNCTION_CALL_ARGNUM_MEM < e->get_args().size());
      interesting_args[0] = OP_FUNCTION_CALL_ARGNUM_MEM;
      num_interesting_args = 1;
      } else */{
        num_interesting_args = 0;
      }
  } else if (e->get_operation_kind() == expr::OP_ITE) {
    ASSERT(e->get_args().size() == 3);
    interesting_args[0] = 1;
    interesting_args[1] = 2;
    num_interesting_args = 2;
    /*} else if (e->get_operation_kind() == expr::OP_SWITCHMEMLABEL) {
      size_t n = 0;
      for (size_t i = 2; i < e->get_args().size(); i += 2) {
      interesting_args[n] = i;
      n++;
      }
      num_interesting_args = n;
      */} else {
        cout << "e = " << endl << expr_string(e) << endl;
        NOT_REACHED();
      }
  }

  virtual void visit(expr_ref const &mem)
  {
    //cout << __func__ << " " << __LINE__ << ": visiting:" << endl << mem->to_string_table(true) << endl;
    DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": simplifying " << expr_string(mem) << " wrt select memlabel " << m_memlabel.to_string() << endl);
    ASSERT(mem->is_array_sort());

    expr_ref ret;
    if (mem->is_var() || mem->is_const()) {
      m_mem_map_for_other_ops[mem->get_id()] = expr_simplify::get_memmask_from_mem(mem, m_mem_alloc, m_memlabel);
      m_mem_map_for_upcoming_memmask_op[mem->get_id()] = mem;
      //m_data_map[mem->get_id()] = this->get_data_from_mem(m_mem_map_for_other_ops[mem->get_id()]);
      m_data_map[mem->get_id()] = this->get_data_from_mem(m_mem_map_for_upcoming_memmask_op[mem->get_id()]);
      DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": mem = " << expr_string(mem) << endl);
      return;
    }
    expr_ref new_data;
    ASSERT(!mem->is_const());
    ASSERT(mem->get_args().size() > 0);
    if (mem->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      expr_ref const &ml_expr = mem->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_WRITEABLE);
      if (ml_expr->is_const()) {
        memlabel_t ml = ml_expr->get_memlabel_value();
        memlabel_t::memlabels_intersect(&ml, &m_memlabel);
        if (memlabel_t::memlabel_is_empty(&ml)) {
          ret = mem->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEM);
        } else {
          ret = mem;
        }
      } else {
        ret = mem;
      }
      new_data = this->get_data_from_mem(ret);
    } else if (mem->get_operation_kind() == expr::OP_MEMMASK) {
      //expr_ref mem_memmask = new_mem->get_args()[0];
      memlabel_t label_memmask = mem->get_args()[OP_MEMMASK_ARGNUM_MEMLABEL]->get_memlabel_value();
      memlabel_t new_memmask_label = label_memmask;
      memlabel_t::memlabels_intersect(&new_memmask_label, &m_memlabel);
      if (memlabel_t::memlabel_is_empty(&new_memmask_label)) {
        ret = m_cs.get_memzero();
        new_data = m_ctx->mk_zerobv(m_nbytes * BYTE_LEN);
      } else {
        expr_vector new_args;
        expr_ref arg0 = mem->get_args().at(OP_MEMMASK_ARGNUM_MEM);
        if (m_mem_map_for_upcoming_memmask_op.count(arg0->get_id())) {
          ASSERT(arg0->is_array_sort());
          new_args.push_back(m_mem_map_for_upcoming_memmask_op.at(arg0->get_id()));
        } else {
          new_args.push_back(arg0);
        }
        new_args.push_back(m_ctx->mk_memlabel_const(new_memmask_label));

        //ret = m_ctx->mk_app(expr::OP_MEMMASK, new_args);
        ret = new_args.at(OP_MEMMASK_ARGNUM_MEM);
        //ret = new_mem; //mem_memmask;
        //ASSERT(m_data_map.count(mem->get_args().at(0)->get_id()) > 0);
        //new_data = m_data_map.at(mem->get_args().at(0)->get_id());
        new_data = this->get_data_from_mem(ret);
      }
  } else if (mem->get_operation_kind() == expr::OP_STORE) {
    expr_ref store_mem       = mem->get_args()[OP_STORE_ARGNUM_MEM];
    expr_ref mem_write       = m_mem_map_for_upcoming_memmask_op.at(store_mem->get_id());
    expr_ref mem_write_alloc = mem->get_args()[OP_STORE_ARGNUM_MEM_ALLOC];
    memlabel_t label_write   = mem->get_args()[OP_STORE_ARGNUM_MEMLABEL]->get_memlabel_value();
    expr_ref addr_write      = mem->get_args()[OP_STORE_ARGNUM_ADDR];
    expr_ref data_write      = mem->get_args()[OP_STORE_ARGNUM_DATA];
    int nbytes_write         = mem->get_args()[OP_STORE_ARGNUM_COUNT]->get_int_value();
    expr_ref new_mem = m_ctx->mk_store(mem_write, mem_write_alloc, label_write, addr_write, data_write, nbytes_write, mem->get_args()[OP_STORE_ARGNUM_ENDIANNESS]->get_bool_value());

    expr_ref nbytes_expr = m_ctx->mk_bv_const(m_addr->get_sort()->get_size(), static_cast<int>(m_nbytes));
    expr_ref nbytes_write_expr = m_ctx->mk_bv_const(m_addr->get_sort()->get_size(), static_cast<int>(nbytes_write));
    //expr_ref addr_write_no_locals = addr_write; //addr_write->substitute_locals_with_input_stack_pointer_const(m_cs);
    //expr_ref addr_no_locals = m_addr; //m_addr->substitute_locals_with_input_stack_pointer_const(m_cs);

    int start_byte, stop_byte;
    bool is_symbol_local_loc;
    //DYN_DEBUG(expr_simplify_select, cout << __func__ << " " << __LINE__ << ": addr_no_locals = " << expr_string(addr_no_locals) << ", nbytes = " << m_nbytes << ", addr_write_no_locals = " << expr_string(addr_write_no_locals) << ", nbytes_write = " << nbytes_write << ", m_addr = " << expr_string(m_addr) << ", addr_write = " << expr_string(addr_write) << ", is_expr_equal_syntactic(no_locals) = " << is_expr_equal_syntactic(addr_no_locals, addr_write_no_locals) << ", is_contained_in() = " << is_contained_in(m_ctx, m_memlabel, addr_no_locals, m_nbytes, label_write, addr_write_no_locals, nbytes_write, start_byte, stop_byte) << ", is_overlapping() = " << is_overlapping(m_ctx, m_memlabel, addr_no_locals, m_nbytes, label_write, addr_write_no_locals, nbytes_write, m_lhs_set, m_cs) << endl);
    if (expr_simplify::is_contained_in_using_lhs_set_and_precond<T_PRED, T_PRECOND>(m_ctx, m_memlabel, m_addr, m_nbytes, label_write, addr_write, nbytes_write, is_symbol_local_loc, start_byte, stop_byte, m_lhs_set, m_precond, m_sprel_map_pair, m_symbol_map, m_locals_map)) {
      ret = new_mem; //XXX: this can actually be a dummy array variable, because it really does not matter what its contents are, at this point.
      if (is_symbol_local_loc || (stop_byte - start_byte == nbytes_write)) {
        new_data = data_write;
      } else {
        new_data = m_ctx->mk_bvextract(data_write, stop_byte * BYTE_LEN - 1, start_byte * BYTE_LEN);
      }
      DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": new_data = " << expr_string(new_data) << endl);
    } else if (   m_memlabel.memlabel_contains_only_readonly_symbols()
               || memlabel_t::memlabels_are_independent(&label_write, &m_memlabel)
               || !expr_simplify::is_overlapping_using_lhs_set_and_precond<T_PRED, T_PRECOND>(m_ctx, m_addr, m_nbytes, addr_write, nbytes_write, m_lhs_set, m_precond, m_sprel_map_pair, m_cs)) {
      ret = mem_write;
      DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl);
      ASSERT(m_data_map.count(store_mem->get_id()) > 0);
      new_data = m_data_map.at(store_mem->get_id());
    } else if (   m_semantic_simplification    // semantic simplification should be attempted after syntactic simplification has failed
               && m_nbytes == nbytes_write
               && m_ctx->expr_is_provable_using_lhs_expr_with_solver(m_lhs_expr, m_ctx->mk_eq(m_addr, addr_write), string(__func__)+".select-over-store-read-write-addrs-are-equal", m_relevant_memlabels)) {
      ret = new_mem; //XXX: this can actually be a dummy array variable, because it really does not matter what its contents are, at this point.
      new_data = data_write;
    } else if (   m_semantic_simplification
               && !m_ctx->check_is_overlapping_using_lhs_expr_with_solver(m_lhs_expr, m_addr, nbytes_expr, addr_write, nbytes_write_expr, m_relevant_memlabels)) {
      ret = mem_write;
      ASSERT(m_data_map.count(store_mem->get_id()) > 0);
      new_data = m_data_map.at(store_mem->get_id());
    } else {
      ret = new_mem;
      DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl);
      new_data = this->get_data_from_mem(ret);
    }
  } else if (mem->get_operation_kind() == expr::OP_STORE_UNINIT) {
    expr_ref store_mem       = mem->get_args()[OP_STORE_UNINIT_ARGNUM_MEM];
    expr_ref mem_write       = m_mem_map_for_upcoming_memmask_op.at(store_mem->get_id());
    expr_ref mem_write_alloc = mem->get_args()[OP_STORE_UNINIT_ARGNUM_MEM_ALLOC];
    memlabel_t label_write   = mem->get_args()[OP_STORE_UNINIT_ARGNUM_MEMLABEL]->get_memlabel_value();
    expr_ref addr_write      = mem->get_args()[OP_STORE_UNINIT_ARGNUM_ADDR];
    expr_ref nbytes_write     = mem->get_args()[OP_STORE_UNINIT_ARGNUM_COUNT];
    expr_ref local_alloc_count = mem->get_args()[OP_STORE_UNINIT_ARGNUM_LOCAL_ALLOC_COUNT];
    expr_ref new_mem = m_ctx->mk_store_uninit(mem_write, mem_write_alloc, label_write, addr_write, nbytes_write, local_alloc_count);

    expr_ref nbytes_expr = m_ctx->mk_bv_const(m_addr->get_sort()->get_size(), static_cast<int>(m_nbytes));

    //if (memlabel_t::memlabels_are_independent(&label_write, &m_memlabel)) {
    if (   memlabel_t::memlabels_are_independent(&label_write, &m_memlabel)
        || (m_semantic_simplification && !m_ctx->check_is_overlapping_using_lhs_expr_with_solver(m_lhs_expr, m_addr, nbytes_expr, addr_write, nbytes_write, m_relevant_memlabels))) {
      ret = mem_write;
      DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl);
      ASSERT(m_data_map.count(store_mem->get_id()) > 0);
      new_data = m_data_map.at(store_mem->get_id());
    } else {
      ret = new_mem;
      new_data = this->get_data_from_mem(new_mem);
    }
  } else if (mem->get_operation_kind() == expr::OP_ITE) {
    expr_ref arg1 = mem->get_args()[1];
    expr_ref arg2 = mem->get_args()[2];
    expr_ref data_write1 = m_data_map.at(arg1->get_id());
    expr_ref data_write2 = m_data_map.at(arg2->get_id());
    expr_ref mem_write1 = m_mem_map_for_upcoming_memmask_op.at(mem->get_args()[1]->get_id());
    mem_write1 = expr_simplify::prune_obviously_false_branches_using_assume_clause(mem->get_args()[0], mem_write1);
    expr_ref mem_write2 = m_mem_map_for_upcoming_memmask_op.at(mem->get_args()[2]->get_id());
    mem_write2 = expr_simplify::prune_obviously_false_branches_using_assume_clause(m_ctx->mk_not(mem->get_args()[0]), mem_write2);
    ASSERT(mem_write1->get_sort() == mem_write2->get_sort());
    ASSERT(data_write1->get_sort() == data_write2->get_sort());
    if (expr_simplify::is_expr_equal_syntactic(mem_write1, mem_write2)) {
      ret = mem_write1;
    } else {
      ret = m_ctx->mk_ite(mem->get_args()[0], mem_write1, mem_write2);
      ret = expr_simplify::simplify_ite(ret);
    }
    data_write1 = expr_simplify::prune_obviously_false_branches_using_assume_clause(mem->get_args()[0], data_write1);
    data_write2 = expr_simplify::prune_obviously_false_branches_using_assume_clause(m_ctx->mk_not(mem->get_args()[0]), data_write2);
    if (expr_simplify::is_expr_equal_syntactic(data_write1, data_write2)) {
      new_data = data_write1;
    } else if (   data_write1 == this->get_data_from_mem(mem_write1)
        && data_write2 == this->get_data_from_mem(mem_write2)) {
      new_data = this->get_data_from_mem(ret);
    } else {
      new_data = m_ctx->mk_ite(mem->get_args()[0], data_write1, data_write2);
      new_data = expr_simplify::simplify_ite(new_data);
    }
    /*} else if (mem->get_operation_kind() == expr::OP_SWITCHMEMLABEL) {
      expr_vector new_args;

      for (size_t i = 0; i < mem->get_args().size(); i++) {
      expr_ref arg = mem->get_args().at(i);
      expr_ref arg_i;
      if (m_mem_map_for_other_ops.count(arg->get_id())) {
      arg_i = m_mem_map_for_other_ops.at(arg->get_id());
      } else {
      arg_i = arg;
      }
      new_args.push_back(arg_i);
      }

      ret = m_ctx->mk_app(expr::OP_SWITCHMEMLABEL, new_args);
      expr_vector new_data_args;

      for (size_t i = 0; i < mem->get_args().size(); i++) {
      expr_ref arg = mem->get_args().at(i);
      if (m_data_map.count(arg->get_id())) {
      ASSERT(arg->is_array_sort());
      new_data_args.push_back(m_data_map.at(arg->get_id()));
      } else {
    //ASSERT(!arg->is_array_sort());
    new_data_args.push_back(arg);
    }
    }
    new_data = m_ctx->mk_app(expr::OP_SWITCHMEMLABEL, new_data_args);
    */} else NOT_REACHED();

    ASSERT(ret);
    ASSERT(new_data);
    DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": m = " << expr_string(mem) << ", ret = " << expr_string(ret) << ", new_data = " << (!new_data ? "NULL" : expr_string(new_data) ) << endl);
    m_mem_map_for_other_ops[mem->get_id()] = ret;
    m_mem_map_for_upcoming_memmask_op[mem->get_id()] = ret;
    m_data_map[mem->get_id()] = new_data;
    DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": mem = " << expr_string(ret) << endl);
    //cout << __func__ << " " << __LINE__ << ": returning:" << endl << ret->to_string_table(true) << endl;
  }

  expr_ref get_data_from_mem(expr_ref const &mem) const
  {
    if (mem == m_cs.get_memzero()) {
      return m_ctx->mk_zerobv(m_nbytes * BYTE_LEN);
    } else {
      return m_ctx->mk_select(mem, m_mem_alloc, m_memlabel, m_addr, m_nbytes, m_bigendian/*, m_comment*/);
    }
  }

  /*expr_ref get_memmask_from_mem(expr_ref mem) const
    {
    if ((m_cs != NULL) && mem == m_cs->get_memzero()) {
    return mem;
    } else if (memlabel_is_top(&m_memlabel)) {
    return mem;
    } else {
    return m_ctx->mk_memmask(mem, m_memlabel);
    }
    }*/

  context *m_ctx;
  expr_ref const &m_mem;
  expr_ref const &m_mem_alloc;
  memlabel_t const &m_memlabel;
  expr_ref const &m_addr;
  unsigned m_nbytes;
  bool m_bigendian;
  //comment_t const &m_comment;
  bool m_semantic_simplification;
  expr_ref const& m_lhs_expr;
  relevant_memlabels_t const& m_relevant_memlabels;
  list<shared_ptr<T_PRED const>> const &m_lhs_set;
  //edge_guard_t const &m_guard;
  T_PRECOND const &m_precond;
  sprel_map_pair_t const &m_sprel_map_pair;
  //tfg_suffixpath_t const &m_src_suffixpath;
  //pred_avail_exprs_t const &m_src_pred_avail_exprs;
  graph_symbol_map_t const& m_symbol_map;
  graph_locals_map_t const& m_locals_map;
  consts_struct_t const &m_cs;
  //bool m_solve;
  map<unsigned, expr_ref> m_mem_map_for_upcoming_memmask_op;
  map<unsigned, expr_ref> m_mem_map_for_other_ops;
  map<unsigned, expr_ref> m_data_map;
};

template<typename T_PRED, typename T_PRECOND>
expr_ref
expr_simplify::expr_simplify_select_on_store(context* ctx, expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const& memlabel, expr_ref const& addr, unsigned nbytes, bool bigendian/*, comment_t comment*/, bool semantic_simplification, expr_ref const& lhs_expr, relevant_memlabels_t const& relevant_memlabels, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
{
  //autostop_timer func_timer(__func__);
  DYN_DEBUG(expr_simplify_select_dbg, cout << _FNLN_ << ": entry\n");
  consts_struct_t const& cs = ctx->get_consts_struct();
  expr_ref mem_masked = mem;
  if (    mem_masked->get_operation_kind() != expr::OP_MEMMASK
      && !memlabel_t::memlabel_is_top(&memlabel)) {
    mem_masked = mem; //ctx->mk_memmask_composite(mem, memlabel); //this is necessary as it brings all memsplice to the top-level of mem.
    if (mem_masked->get_operation_kind() == expr::OP_MEMMASK) {
      mem_masked = expr_simplify::simplify_memmask(mem_masked);
    }
  }
  //Important invariant: all memsplice (if any) are at the top-level of mem_masked at this point
  expr_simplify_select_on_store_visitor<T_PRED, T_PRECOND> visitor(ctx, mem_masked, mem_alloc, memlabel, addr, nbytes, bigendian, semantic_simplification, lhs_expr, relevant_memlabels, lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
  expr_ref ret = visitor.get_result();

  if (ret->get_operation_kind() == expr::OP_SELECT && ret->get_args().at(OP_SELECT_ARGNUM_MEM_ALLOC) == mem_alloc && ret->get_args().at(OP_SELECT_ARGNUM_MEMLABEL)->get_memlabel_value() == memlabel && !memlabel.memlabel_contains_stack_or_local()) {
    //expr_simplify_memalloc_using_memlabel_visitor visitor2(mem_alloc, memlabel);
    //expr_ref new_mem_alloc = visitor2.get_result();

    expr_ref new_mem_alloc = expr_simplify::expr_simplify_memalloc_using_memlabel(mem_alloc, memlabel);

    ret = ctx->mk_app(expr::OP_SELECT, {ret->get_args().at(OP_SELECT_ARGNUM_MEM), new_mem_alloc, ret->get_args().at(OP_SELECT_ARGNUM_MEMLABEL), ret->get_args().at(OP_SELECT_ARGNUM_ADDR), ret->get_args().at(OP_SELECT_ARGNUM_COUNT), ret->get_args().at(OP_SELECT_ARGNUM_ENDIANNESS)});
  }

  DYN_DEBUG(expr_simplify_select_dbg, cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << ": mem = " << expr_string(mem) << ", ret = " << expr_string(ret) << endl);
  //cout << __func__ << " " << __LINE__ << ": mem = " << expr_string(mem) << ", ret = " << expr_string(ret) << endl;
  return ret;
}



template<typename T_PRED, typename T_PRECOND>
class expr_simplify_store_on_store_visitor : public expr_visitor
{
  using T_PRED_REF = shared_ptr<T_PRED const>;
  public:
  expr_simplify_store_on_store_visitor(context* ctx, expr_ref const& mem, expr_ref const& mem_alloc, memlabel_t const &memlabel, expr_ref addr, expr_ref data, unsigned nbytes, bool bigendian, list<T_PRED_REF> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map) : m_ctx(ctx), m_mem(mem), m_mem_alloc(mem_alloc), m_memlabel(memlabel), m_addr(addr), m_data(data), m_nbytes(nbytes), m_bigendian(bigendian), /*m_comment(comment), */m_lhs_set(lhs_set), m_precond(precond), m_sprel_map_pair(sprel_map_pair), m_symbol_map(symbol_map), m_locals_map(locals_map), m_cs(ctx->get_consts_struct())
  {
    //m_visit_each_expr_only_once = true;
    visit_recursive(m_mem);
  }
  expr_ref get_result() const {
    return m_ctx->mk_store(m_mem_map.at(m_mem->get_id()), m_mem_alloc, m_memlabel, m_addr, m_data, m_nbytes, m_bigendian/*, m_comment*/);
  }
  private:
  virtual void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args)
  {
    ASSERT(e->is_array_sort());
    if (e->is_var()) {
      num_interesting_args = 0;
    } else if (e->is_const()) {
      num_interesting_args = 0;
    } else if (e->get_operation_kind() == expr::OP_STORE) {
      interesting_args[0] = OP_STORE_ARGNUM_MEM;
      num_interesting_args = 1;
    } else if (e->get_operation_kind() == expr::OP_STORE_UNINIT) {
      interesting_args[0] = OP_STORE_UNINIT_ARGNUM_MEM;
      num_interesting_args = 1;
    } else if (e->get_operation_kind() == expr::OP_MEMMASK) {
      interesting_args[0] = OP_MEMMASK_ARGNUM_MEM;
      num_interesting_args = 1;
      //} else if (e->get_operation_kind() == expr::OP_MEMSPLICE) {
      //  size_t j = 0;
      //  for (size_t i = 0; i < e->get_args().size(); i+=2, j++) {
      //    interesting_args[j] = i + 1;
      //  }
      //  num_interesting_args = j; //e->get_args().size();
      //} else if (e->get_operation_kind() == expr::OP_ALLOCA) {
      //  interesting_args[0] = 0;
      //  num_interesting_args = 1;
  } else if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
    /*if (expr_func_has_array_output(e->get_args()[0])) {
      ASSERT(OP_FUNCTION_CALL_ARGNUM_MEM < e->get_args().size());
      interesting_args[0] = OP_FUNCTION_CALL_ARGNUM_MEM;
      num_interesting_args = 1;
      } else */{
        num_interesting_args = 0;
      }
  } else if (e->get_operation_kind() == expr::OP_ALLOCA) {
    num_interesting_args = 0;
  } else if (e->get_operation_kind() == expr::OP_DEALLOC) {
    num_interesting_args = 0;
  } else if (e->get_operation_kind() == expr::OP_ITE) {
    ASSERT(e->get_args().size() == 3);
    interesting_args[0] = 1;
    interesting_args[1] = 2;
    num_interesting_args = 2;
    /*} else if (e->get_operation_kind() == expr::OP_SWITCHMEMLABEL) {
      size_t n = 0;
      for (size_t i = 2; i < e->get_args().size(); i += 2) {
      interesting_args[n] = i;
      n++;
      }
      num_interesting_args = n;
      */} else {
        cout << "e = " << endl << expr_string(e) << endl;
        NOT_REACHED();
      }
  }

  virtual void visit(expr_ref const &e)
  {
    context *ctx = e->get_context();
    if (e->is_var() || e->is_const()) {
      m_mem_map[e->get_id()] = e;
      return;
    }
    if (e->get_operation_kind() == expr::OP_STORE) {
      memlabel_t e_ml = e->get_args().at(OP_STORE_ARGNUM_MEMLABEL)->get_memlabel_value();
      expr_ref e_addr = e->get_args().at(OP_STORE_ARGNUM_ADDR);
      unsigned e_nbytes = (unsigned)e->get_args().at(OP_STORE_ARGNUM_COUNT)->get_int_value();
      int start_byte, stop_byte;
      bool is_symbol_local_loc;
      //cout << __func__ << " " << __LINE__ << ": e_addr = " << expr_string(e_addr) << endl;
      //cout << __func__ << " " << __LINE__ << ": m_addr = " << expr_string(m_addr) << endl;
      if (expr_simplify::is_contained_in_using_lhs_set_and_precond(ctx, e_ml, e_addr, e_nbytes, m_memlabel, m_addr, m_nbytes,is_symbol_local_loc, start_byte, stop_byte, m_lhs_set, m_precond, m_sprel_map_pair, m_symbol_map, m_locals_map)) {
        //cout << __func__ << " " << __LINE__ << ": is_contained_in returned true\n";
        /*&& is_expr_equal_syntactic(e->get_args().at(2), m_addr)
          && (int)m_nbytes == e->get_args().at(4)->get_int_value())*/
        //ignore this store
        m_mem_map[e->get_id()] = m_mem_map[e->get_args().at(OP_STORE_ARGNUM_MEM)->get_id()];
        return;
      }
      //cout << __func__ << " " << __LINE__ << ": is_contained_in returned false\n";
    }
    if (e->get_operation_kind() == expr::OP_STORE_UNINIT) {
      // no-op for now
    }
    expr_vector new_args;
    for (auto arg : e->get_args()) {
      if (m_mem_map.count(arg->get_id())) {
        new_args.push_back(m_mem_map.at(arg->get_id()));
      } else {
        new_args.push_back(arg);
      }
    }
    m_mem_map[e->get_id()] = m_ctx->mk_app(e->get_operation_kind(), new_args);
  }

  context *m_ctx;
  expr_ref m_mem;
  expr_ref m_mem_alloc;
  memlabel_t const &m_memlabel;
  expr_ref m_addr;
  expr_ref m_data;
  unsigned m_nbytes;
  bool m_bigendian;
  //comment_t const &m_comment;
  list<T_PRED_REF> const &m_lhs_set;
  //edge_guard_t const &m_guard;
  T_PRECOND const &m_precond;
  sprel_map_pair_t const &m_sprel_map_pair;
  graph_symbol_map_t const& m_symbol_map;
  graph_locals_map_t const& m_locals_map;
  consts_struct_t const &m_cs;
  map<expr_id_t, expr_ref> m_mem_map;
};

template<typename T_PRED, typename T_PRECOND>
expr_ref
expr_simplify::simplify_store(expr_ref e, list<shared_ptr<T_PRED const>> const &lhs_set, T_PRECOND const &precond, sprel_map_pair_t const &sprel_map_pair, graph_symbol_map_t const& symbol_map, graph_locals_map_t const& locals_map)
{
  context* ctx = e->get_context();
  consts_struct_t const& cs = ctx->get_consts_struct();
  //autostop_timer func_timer(__func__);
  //stringstream ss;
  //ss << __func__ << ".expr_size" << expr_size(e);
  //autostop_timer func_expr_size_timer(ss.str());
  ASSERT(e->get_operation_kind() == expr::OP_STORE);
  expr_ref ml_expr = e->get_args()[OP_STORE_ARGNUM_MEMLABEL];
  if (ml_expr->is_var()) {
    return e;
  }
  ASSERT(ml_expr->is_const());
  expr_ref mem       = e->get_args()[OP_STORE_ARGNUM_MEM];
  expr_ref mem_alloc = e->get_args()[OP_STORE_ARGNUM_MEM_ALLOC];
  memlabel_t ml      = ml_expr->get_memlabel_value();
  expr_ref addr      = e->get_args()[OP_STORE_ARGNUM_ADDR];
  expr_ref data      = e->get_args()[OP_STORE_ARGNUM_DATA];
  unsigned nbytes    = e->get_args()[OP_STORE_ARGNUM_COUNT]->get_int_value();
  bool bigendian     = e->get_args()[OP_STORE_ARGNUM_ENDIANNESS]->get_bool_value();
  if(addr->get_operation_kind() != expr::OP_ITE && mem->get_operation_kind() != expr::OP_ITE) {
    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": simplifying store_on_store: addr = " << expr_string(addr) << endl);
    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": mem = " << expr_string(mem) << endl);
    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": ml = " << ml.to_string() << endl);

    //memlabel_intersect_using_lhs_set_and_addr(ml, lhs_set, precond, sprel_map_pair/*, src_suffixpath, src_pred_avail_exprs*/, addr, nbytes, cs);
    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": after intersect, ml = " << ml.to_string() << endl);
    expr_simplify_store_on_store_visitor<T_PRED, T_PRECOND> visitor(ctx, mem, mem_alloc, ml, addr, data, nbytes, bigendian, lhs_set, precond, sprel_map_pair, symbol_map, locals_map);
    expr_ref ret = visitor.get_result();

    if (ret->get_operation_kind() == expr::OP_STORE && ret->get_args().at(OP_STORE_ARGNUM_MEM_ALLOC) == mem_alloc && ret->get_args().at(OP_STORE_ARGNUM_MEMLABEL)->get_memlabel_value() == ml && !ml.memlabel_contains_stack_or_local()) {
      //expr_simplify_memalloc_using_memlabel_visitor visitor2(mem_alloc, ml);
      //expr_ref new_mem_alloc = visitor2.get_result();
      expr_ref new_mem_alloc = expr_simplify::expr_simplify_memalloc_using_memlabel(mem_alloc, ml);
      ret = ctx->mk_app(expr::OP_STORE, {ret->get_args().at(OP_STORE_ARGNUM_MEM), new_mem_alloc, ret->get_args().at(OP_STORE_ARGNUM_MEMLABEL), ret->get_args().at(OP_STORE_ARGNUM_ADDR), ret->get_args().at(OP_STORE_ARGNUM_DATA), ret->get_args().at(OP_STORE_ARGNUM_COUNT), ret->get_args().at(OP_STORE_ARGNUM_ENDIANNESS)});
    }

    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << endl);
    return ret;
  }
  return e;

  //  std::function<expr_ref (expr_ref const &)> simplify_store_addr_no_ite_function =
  //    [ctx, &mem, &ml, &data, nbytes, bigendian, /*&comment, */&lhs_set, &precond, &sprel_map_pair, &src_suffixpath, &src_pred_avail_exprs, &cs](expr_ref const &addr)
  //  {
  //    std::function<expr_ref (expr_ref const &mem)> simplify_store_addr_no_ite_mem_no_ite_function =
  //      [ctx, &addr, &ml, &data, nbytes, bigendian, /*&comment, */&lhs_set, &precond, &sprel_map_pair, &src_suffixpath, &src_pred_avail_exprs, &cs](expr_ref const &mem)
  //    {
  //      CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": simplifying store_on_store: addr = " << expr_string(addr) << endl);
  //      CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": mem = " << expr_string(mem) << endl);
  //      CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": ml = " << memlabel_to_string(&ml, as1, sizeof as1) << endl);
  //
  //      memlabel_intersect_using_lhs_set_and_addr(ml, lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, addr, nbytes, cs);
  //      CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": after intersect, ml = " << memlabel_to_string(&ml, as1, sizeof as1) << endl);
  //      expr_simplify_store_on_store_visitor visitor(ctx, mem, ml, addr, data, nbytes, bigendian, /*comment, */lhs_set, precond, sprel_map_pair, src_suffixpath, src_pred_avail_exprs, cs);
  //      expr_ref ret = visitor.get_result();
  //      CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << endl);
  //      return ret;
  //    };
  //
  //    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << endl);
  //    expr_ref ret = expr_fold_ite<expr_ref>(mem, simplify_store_addr_no_ite_mem_no_ite_function, fold_ite_and_simplify_expr);
  //    CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl);
  //    return ret;
  //  };
  //
  //  CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": addr = " << expr_string(addr) << ", mem = " << expr_string(mem) << endl);
  //  expr_ref ret;
  //  ret = expr_fold_ite<expr_ref>(addr, simplify_store_addr_no_ite_function, fold_ite_and_simplify_expr);
  //  CPP_DBG_EXEC(EXPR_SIMPLIFY, cout << __func__ << " " << __LINE__ << ": returning = " << expr_string(ret) << endl);
  //
  //  return ret;
}

}
