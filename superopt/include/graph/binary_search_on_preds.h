#ifndef EQCHECKBINARY_SEARCH_ON_PREDS_H
#define EQCHECKBINARY_SEARCH_ON_PREDS_H
#include "support/binary_search.h"

namespace eqspace {

template <typename T_PC, typename T_N, typename T_E> class graph;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class graph_with_predicates;

template <typename T_PC, typename T_N, typename T_E, typename T_PRED>
class binary_search_on_preds_t : binary_search_t {
public:
  typedef list<shared_ptr<T_E const>> T_PATH;
  binary_search_on_preds_t(vector<T_PRED> const &elems, T_PC const &p, graph_with_predicates<T_PC, T_N, T_E, T_PRED> &graph, list<T_PATH> const &paths) : binary_search_t(elems.size()), m_elems(elems), m_pc(p), m_graph(graph), m_paths(paths)
  {
    m_ctx = m_graph.get_context();
    do_binary_search();
  }
  vector<T_PRED> get_acceptable() {
    vector<T_PRED> ret;
    set<size_t> posvec = get_test_positive();
    for (auto pos : posvec) {
      ASSERT(pos >= 0 && pos < m_elems.size());
      ret.push_back(m_elems.at(pos));
    }
    return ret;
  }
  bool test(size_t begin, size_t end) const {
    NOT_IMPLEMENTED();
    /*for (const auto &pth : m_paths) {
      list<pair<expr_ref, expr_ref>> assumes;
      m_graph.path_collect_assumes(pth, assumes);
      expr_ref to_prove = expr_false(m_ctx);
      string comment;
      for (size_t i = begin; i < end; i++) {
        pair<expr_ref, expr_ref> lhs_rhs = m_elems.at(i).get_exprs();
        expr_ref lhs_apply = m_graph.path_apply_trans_fun_src(lhs_rhs.first, pth);
        expr_ref rhs_apply = m_graph.path_apply_trans_fun_dst(lhs_rhs.second, pth);
        to_prove = expr_or(to_prove, expr_eq(lhs_apply, rhs_apply));
        comment = comment + m_elems.at(i).get_comment() + "-";
      }
      //if (!m_ctx->prove(assumes, to_prove, expr_true(m_ctx), query_comment(query_comment::pred_provable_on_path), true)) {
      //  cout << __func__ << " " << __LINE__ << ": returned false for " << comment << endl << "to_prove = " << endl << to_prove->to_string_table() << endl;
      //  for (auto a : assumes) {
      //    cout << "assume: " << expr_string(a.first) << " == " << expr_string(a.second) << endl;
      //  }
      //  return false;
      //}
    }*/
    return true;
  }
private:
  vector<T_PRED> const &m_elems;
  T_PC const &m_pc;
  graph_with_predicates<T_PC, T_N, T_E, T_PRED> &m_graph;
  list<T_PATH> const &m_paths;
  context *m_ctx;
};
}

#endif
