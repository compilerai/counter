#pragma once

#include "expr/expr_utils.h"

#include "graph/graph_with_locs.h"

namespace eqspace {
using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED, typename T_VAL>
class expr_loc_visitor
{
public:
  expr_loc_visitor(expr_ref const& e, map<expr_id_t, graph_loc_id_t> const& expr_id_to_loc_id_map, string const& fname) : m_ctx(e->get_context()), m_expr(e), m_expr_id_to_loc_id_map(expr_id_to_loc_id_map), m_fname(fname)
  {
    m_expr = m_ctx->expr_eliminate_allocas(m_expr);
    m_expr = m_ctx->expr_sink_memmasks_to_bottom(m_expr);
    m_expr = m_ctx->expr_do_simplify(m_expr);
  }

  T_VAL run() { return visit_recursive(m_expr, nullptr, nullptr); }

private:
  T_VAL visit_recursive(expr_ref const& e, expr_ref const& mem_alloc, expr_ref const& mask_memlabel)
  {
    context* ctx = e->get_context();
    //cout << __func__ << " " << __LINE__ << ": e =\n" << ctx->expr_to_string_table(e) << endl;
    pair<bool, map<int, pair<expr_ref,expr_ref>>> pval = previsit(e, mem_alloc, mask_memlabel);
    vector<T_VAL> argvals;
    if (!pval.first) { //visit the children only if this is not a loc itself
      map<int, pair<expr_ref,expr_ref>> const &arg_masks = pval.second;
      for (size_t i = 0; i < e->get_args().size(); i++) {
        expr_ref const &arg = e->get_args().at(i);
        expr_ref ml = arg_masks.count(i) ? arg_masks.at(i).first  : nullptr;
        expr_ref mml = arg_masks.count(i) ? arg_masks.at(i).second : nullptr;
        //cout << "ml =\n" << (ml ? ctx->expr_to_string_table(ml) : "(nullptr))") << endl;
        argvals.push_back(visit_recursive(arg, ml, mml));
      }
    }
    return identify_locs_and_visit(e, argvals, mem_alloc, mask_memlabel);
  }

  expr_ref mask_memlabel_intersect(expr_ref const &a, expr_ref const &b)
  {
    if (!a) {
      return b;
    }
    if (!b) {
      return a;
    }
    if (a->is_var() || b->is_var()) {
      NOT_IMPLEMENTED();
    }
    memlabel_t const &a_ml = a->get_memlabel_value();
    memlabel_t const &b_ml = b->get_memlabel_value();
    if (a_ml.memlabel_is_top() || b_ml.memlabel_is_top()) {
      return m_ctx->mk_memlabel_const(memlabel_t::memlabel_top());
    } else {
      memlabel_t ml = a_ml;
      memlabel_t::memlabels_intersect(&ml, &b_ml);
      return m_ctx->mk_memlabel_const(ml);
    }
  }

  //BOOL retval indicates if this is a loc
  //map<int, expr_ref> is a map from the argnum of e->get_args() TO the corresponding mask_memlabel
  pair<bool, map<int, pair<expr_ref,expr_ref>>> previsit(expr_ref const &e, expr_ref const& mem_alloc, expr_ref const &mask_memlabel_e)
  {
    //cout << __func__ << " " << __LINE__ << ": entry, e = " << expr_string(e) << endl;
    graph_loc_id_t loc_id;
    loc_id = get_loc_id_for_expr(e);
    if (loc_id != GRAPH_LOC_ID_INVALID) {
      //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
      return make_pair(true, map<int, pair<expr_ref,expr_ref>>());
    }
    map<int, pair<expr_ref,expr_ref>> mask_memlabel;
    if (e->get_operation_kind() == expr::OP_SELECT) {
      mask_memlabel.insert(make_pair(OP_SELECT_ARGNUM_MEM, make_pair(e->get_args().at(OP_SELECT_ARGNUM_MEM_ALLOC), e->get_args().at(OP_SELECT_ARGNUM_MEMLABEL))));
    //} else if (e->get_operation_kind() == expr::OP_ALLOCA_PTR) {
    //  mask_memlabel.insert(make_pair(OP_ALLOCA_PTR_ARGNUM_MEM, /*make_pair(e->get_args().at(OP_ALLOCA_PTR_ARGNUM_MEM_ALLOC), */e->get_args().at(OP_ALLOCA_PTR_ARGNUM_MEMLABEL)))/*)*/;
    } else if (e->get_operation_kind() == expr::OP_MEMMASK) {
      expr_ref new_mask = mask_memlabel_intersect(mask_memlabel_e, e->get_args().at(OP_MEMMASK_ARGNUM_MEMLABEL));
      mask_memlabel.insert(make_pair(OP_MEMMASK_ARGNUM_MEM, make_pair(e->get_args().at(OP_MEMMASK_ARGNUM_MEM_ALLOC), new_mask)));
    } else if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      // ignore mask_mem since fcall is reading from these memlabels
      expr_ref new_mask = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
      expr_ref new_mem_alloc = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEM_ALLOC);
      //if (e->is_array_sort()) {
      //  ASSERT(mask_memlabel_e);
      //}
      mask_memlabel.insert(make_pair(OP_FUNCTION_CALL_ARGNUM_MEM, make_pair(new_mem_alloc, new_mask)));
    } else if (e->get_operation_kind() == expr::OP_STORE) {
      //m_mask_memlabel.insert(make_pair(e->get_args().at(0)->get_id(), m_mask_memlabel.at(e->get_id())));
      ASSERT(mem_alloc);
      ASSERT(mask_memlabel_e);
      mask_memlabel.insert(make_pair(OP_STORE_ARGNUM_MEM, make_pair(mem_alloc, mask_memlabel_e)));
    } else if (e->get_operation_kind() == expr::OP_STORE_UNINIT) {
      ASSERT(mem_alloc);
      ASSERT(mask_memlabel_e);
      mask_memlabel.insert(make_pair(OP_STORE_UNINIT_ARGNUM_MEM, make_pair(mem_alloc, mask_memlabel_e)));
    } else if (   e->get_operation_kind() == expr::OP_ITE
               && e->is_array_sort()) {
      if (mask_memlabel_e) {
        mask_memlabel.insert(make_pair(1, make_pair(mem_alloc, mask_memlabel_e)));
        mask_memlabel.insert(make_pair(2, make_pair(mem_alloc, mask_memlabel_e)));
      }
    } else {
      if (!(e->is_const() || e->is_var() || !e->is_array_sort() || e->get_sort()->get_range_sort()->is_memlabel_kind())) {
        context* ctx = e->get_context();
        cout << _FNLN_ << ": e =\n" << ctx->expr_to_string_table(e) << endl;
      }
      ASSERT(e->is_const() || e->is_var() || !e->is_array_sort() || e->get_sort()->get_range_sort()->is_memlabel_kind());
    }
    return make_pair(false, mask_memlabel);
  }
  virtual T_VAL visit(expr_ref const &e, bool represents_loc, set<graph_loc_id_t> const &m_locs, vector<T_VAL> const &argvals) = 0;

  T_VAL identify_locs_and_visit(expr_ref const &e, vector<T_VAL> const &argvals, expr_ref const& mem_alloc, expr_ref const &mask_memlabel_e)
  {
    graph_loc_id_t loc_id;
    //cout << __func__ << " " << __LINE__ << ": entry, e = " << expr_string(e) << endl;
    if (   e->is_var()
        && e->is_array_sort()
        && e->get_sort()->get_range_sort()->is_bv_kind()
        /*&& mem_alloc->is_var()*/) {
      //ASSERTCHECK(mem_alloc, cout << "e = " << expr_string(e) << endl << "mask_memlabel_e = " << expr_string(mask_memlabel_e) << endl);
      ASSERT(mask_memlabel_e);
      //expr_ref mem_alloc = get_corresponding_mem_alloc_from_mem_expr(e); //Because the analyses that utilize expr_loc_visitor (aliasing, locs identification) do not rely on the allocation map and only rely on the data map, we simply ignore the mem_alloc argument and simply use the canonical (corresponding) mem_alloc variable
      if (mask_memlabel_e->is_var()) {
        NOT_IMPLEMENTED();
      } else {
        ASSERT(mask_memlabel_e->is_const());
        memlabel_t const &ml_e = mask_memlabel_e->get_memlabel_value();
        set<graph_loc_id_t> loc_ids;
        if (!ml_e.memlabel_is_top()) {
          set<memlabel_ref> atomic_mls = ml_e.get_atomic_memlabels();
          for (const auto &mlr : atomic_mls) {
            auto const& ml = mlr->get_ml();
            expr_ref mask_e = m_ctx->mk_memmask(e, mem_alloc, ml);
            loc_id = get_loc_id_for_expr(mask_e);
            ASSERTCHECK((loc_id != GRAPH_LOC_ID_INVALID), cout << __func__ << ' ' << __LINE__ << ": " << m_fname << ": could not find loc_id for expr: " << expr_string(mask_e) << "\ne = " << expr_string(e) << endl);
            loc_ids.insert(loc_id);
            //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
          }
        }
        return this->visit(e, true, loc_ids, {});
      }
    } else if ((loc_id = get_loc_id_for_expr(e)) != GRAPH_LOC_ID_INVALID) {
      //cout << __func__ << " " << __LINE__ << ": visiting loc " << loc_id << " represented by " << expr_string(e) << endl;
      return visit(e, true, { loc_id }, argvals);
    } else {
      //cout << __func__ << " " << __LINE__ << ": visiting children for " << expr_string(e) << endl;
      return visit(e, false, {}, argvals);
    }
  }

  graph_loc_id_t get_loc_id_for_expr(expr_ref const& e) const
  {
    if (m_expr_id_to_loc_id_map.count(e->get_id())) {
      return m_expr_id_to_loc_id_map.at(e->get_id());
    } else {
      //{
      //  context* ctx = e->get_context();
      //  cout << _FNLN_ << ": e (id " << e->get_id() << ") =\n" << ctx->expr_to_string_table(e, true) << endl;
      //  for (auto const& e2l : m_expr_id_to_loc_id_map) {
      //     cout << "  " << e2l.first << " -> " << e2l.second << "\n";
      //     //cout << ctx->expr_to_string_table(this->get_locs().at(e2l.second));
      //  }
      //}
      return GRAPH_LOC_ID_INVALID;
    }
  }

private:
  context *m_ctx;
  expr_ref m_expr;
  map<expr_id_t, graph_loc_id_t> const& m_expr_id_to_loc_id_map;
  string const& m_fname;
  //map<expr_id_t, expr_ref> m_mask_memlabel;
};

}
