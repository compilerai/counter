#pragma once

#include "graph/graph_with_locs.h"

namespace eqspace {
using namespace std;

template<typename T_PC, typename T_N, typename T_E, typename T_PRED, typename T_VAL>
class expr_loc_visitor
{
public:
  expr_loc_visitor(context *ctx, graph_with_locs<T_PC, T_N, T_E, T_PRED> const& graph/*std::function<graph_loc_id_t (expr_ref const &e)> expr2locid_fn*/) : m_ctx(ctx), m_graph(graph)//m_expr2loc(expr2locid_fn)
  { }

  T_VAL visit_recursive(expr_ref const &e, expr_ref const &mask_memlabel)
  {
    //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    pair<bool, map<int, expr_ref>> pval = previsit(e, mask_memlabel);
    vector<T_VAL> argvals;
    if (!pval.first) { //visit the children only if this is not a loc itself
      map<int, expr_ref> const &arg_masks = pval.second;
      for (size_t i = 0; i < e->get_args().size(); i++) {
        expr_ref const &arg = e->get_args().at(i);
        argvals.push_back(visit_recursive(arg, arg_masks.count(i) ? arg_masks.at(i) : nullptr));
      }
    }
    return identify_locs_and_visit(e, argvals, mask_memlabel);
  }

private:
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
  pair<bool, map<int, expr_ref>> previsit(expr_ref const &e, expr_ref const &mask_memlabel_e)
  {
    //cout << __func__ << " " << __LINE__ << ": entry, e = " << expr_string(e) << endl;
    graph_loc_id_t loc_id;
    //loc_id = m_expr2loc(e);
    loc_id = m_graph.graph_expr_to_locid(e);
    if (loc_id != GRAPH_LOC_ID_INVALID) {
      //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
      return make_pair(true, map<int, expr_ref>());
    }
    map<int, expr_ref> mask_memlabel;
    if (e->get_operation_kind() == expr::OP_SELECT) {
      mask_memlabel.insert(make_pair(0, e->get_args().at(1)));
    } else if (e->get_operation_kind() == expr::OP_MEMMASK) {
      expr_ref new_mask = mask_memlabel_intersect(mask_memlabel_e, e->get_args().at(1));
      mask_memlabel.insert(make_pair(0, new_mask));
    } else if (e->get_operation_kind() == expr::OP_FUNCTION_CALL) {
      //expr_ref new_mask = mask_memlabel_intersect(mask_memlabel_e, e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE));
      // ignore mask_mem since fcall is reading from these memlabels
      expr_ref new_mask = e->get_args().at(OP_FUNCTION_CALL_ARGNUM_MEMLABEL_CALLEE_READABLE);
      if (e->is_array_sort()) {
        ASSERT(mask_memlabel_e);
        mask_memlabel.insert(make_pair(OP_FUNCTION_CALL_ARGNUM_MEM, new_mask));
        //mask_memlabel.insert(make_pair(OP_FUNCTION_CALL_ARGNUM_ORIG_MEMVAR, new_mask));
      } else {
        mask_memlabel.insert(make_pair(OP_FUNCTION_CALL_ARGNUM_MEM, new_mask));
        //mask_memlabel.insert(make_pair(OP_FUNCTION_CALL_ARGNUM_ORIG_MEMVAR, new_mask));
      }
    } else if (   e->get_operation_kind() == expr::OP_STORE
               || e->get_operation_kind() == expr::OP_ALLOCA) {
      //m_mask_memlabel.insert(make_pair(e->get_args().at(0)->get_id(), m_mask_memlabel.at(e->get_id())));
      ASSERT(mask_memlabel_e);
      mask_memlabel.insert(make_pair(0, mask_memlabel_e));
    } else if (   e->get_operation_kind() == expr::OP_ITE
               && e->is_array_sort()) {
      if (mask_memlabel_e) {
        mask_memlabel.insert(make_pair(1, mask_memlabel_e));
        mask_memlabel.insert(make_pair(2, mask_memlabel_e));
      }
    } else ASSERT(e->is_const() || e->is_var() || !e->is_array_sort());
    return make_pair(false, mask_memlabel);
  }
  virtual T_VAL visit(expr_ref const &e, bool represents_loc, set<graph_loc_id_t> const &m_locs, vector<T_VAL> const &argvals) = 0;

  T_VAL identify_locs_and_visit(expr_ref const &e, vector<T_VAL> const &argvals, expr_ref const &mask_memlabel_e)
  {
    graph_loc_id_t loc_id;
    //cout << __func__ << " " << __LINE__ << ": entry, e = " << expr_string(e) << endl;
    if (e->is_var() && e->is_array_sort()) {
      ASSERT(mask_memlabel_e);
      //expr_ref const &ml_expr = m_mask_memlabel.at(e->get_id());
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
            expr_ref mask_e = m_ctx->mk_memmask(e, ml);
            //loc_id = m_expr2loc(mask_e);
            loc_id = m_graph.graph_expr_to_locid(mask_e);
            ASSERTCHECK((loc_id != GRAPH_LOC_ID_INVALID), cout << __func__ << ' ' << __LINE__ << ": could not find loc_id for expr: " << expr_string(mask_e) << "\ne = " << expr_string(e) << endl);
            loc_ids.insert(loc_id);
            //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << endl;
          }
        }
        return this->visit(e, true, loc_ids, {});
      }
    } else if ((loc_id = m_graph.graph_expr_to_locid(e)) != GRAPH_LOC_ID_INVALID) {
      //cout << __func__ << " " << __LINE__ << ": visiting loc " << loc_id << " represented by " << expr_string(e) << endl;
      return visit(e, true, { loc_id }, argvals);
    } else {
      //cout << __func__ << " " << __LINE__ << ": visiting children for " << expr_string(e) << endl;
      return visit(e, false, {}, argvals);
    }
  }

private:
  context *m_ctx;
  //std::function<graph_loc_id_t (expr_ref const &e)> m_expr2loc;
  graph_with_locs<T_PC, T_N, T_E, T_PRED> const& m_graph;
  map<expr_id_t, expr_ref> m_mask_memlabel;
};

}
