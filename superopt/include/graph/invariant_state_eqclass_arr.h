#pragma once
#include <map>
#include <string>

#include "graph/invariant_state_eqclass.h"

namespace eqspace {

using namespace std;

class addr_that_may_differ_t
{
private:
  expr_ref m_addr;
  memlabel_ref m_memlabel;
  size_t m_nbytes;
public:
  addr_that_may_differ_t(expr_ref const& addr, memlabel_ref const& ml, size_t nbytes) : m_addr(addr), m_memlabel(ml), m_nbytes(nbytes)
  { }
  expr_ref const& get_addr() const { return m_addr; }
  memlabel_ref const& get_memlabel() const { return m_memlabel; }
  size_t get_nbytes() const { return m_nbytes; }
  bool equals(addr_that_may_differ_t const& other) const { return m_addr == other.m_addr && m_memlabel == other.m_memlabel && m_nbytes == other.m_nbytes; }
  string to_string() const {
    stringstream ss;
    ss << "addr: " << expr_string(m_addr) << " m_memlabel: " << m_memlabel->get_ml().to_string() << " nbytes " << m_nbytes << endl;
    return ss.str();
  }
};

//XXX: KNOWN BUG: we do not account for the fact that different addrs may be overlapping non-exactly; for that we would need to do a byte-granular analysis; for now, we make do with an "access-granular" analysis by assuming that two accesses are either identical (perfect overlap) or disjoint.
class addr_partition_t
{
private:
  using partition_set_status_t = enum { SRC_DST_AGREE, SRC_DST_DISAGREE };
  vector<addr_that_may_differ_t> const& m_addrs_that_may_differ;
  long m_id;
  vector<pair<vector<addr_that_may_differ_t>, partition_set_status_t>> m_partition;
  bool m_arrs_are_unequal;
public:
  string partition_to_string()
  {
    stringstream ss;
    int i =1;
    for(auto const &p : m_partition) 
    {
      ss << "partition " << i++ << endl;
      ss << "addrs " << endl;
      for(auto const &a : p.first)
      {
        ss << a.to_string();
      }
      if(p.second == 0)
        ss << "partition_set_status : SRC_DST_AGREE " << endl;
      else
        ss << "partition_set_status : SRC_DST_DISAGREE " << endl;
    }
    return ss.str();
  }
  addr_partition_t(long id, vector<addr_that_may_differ_t> const& addrs_that_may_differ) : m_addrs_that_may_differ(addrs_that_may_differ), m_id(id), m_partition({ make_pair(addrs_that_may_differ, SRC_DST_AGREE) }), m_arrs_are_unequal(false)
  { }
  expr_ref get_indicator_variable_for_pset_num(size_t pset_num, sort_ref const& addr_sort, context *ctx) const
  {
    stringstream ss;
    ss << ADDR_DIFF_INDICATOR_VAR_PREFIX << m_id << "." << pset_num;
    return ctx->mk_var(ss.str(), addr_sort);
  }

  bool
  arrays_differ_beyond_addr_vals(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt) const
  {
    if (arr1.get_default_value() != arr2.get_default_value()) {
      return true;
    }
    bool ranges_differ = arr1.range_mappings_differ(arr2);
    if (ranges_differ) {
      return true;
    }
    list<expr_ref> addrs_that_differ = arr1.addr_mappings_differ(arr2);
    for (auto const& addr : addrs_that_differ) {
      ssize_t addr_idx = this->identify_containing_addr(addr, addr_pt);
      if (addr_idx == -1) {
        return true;
      }
    }
    return false;
  }

  ssize_t identify_containing_addr(expr_ref const& byte_addr, point_ref const& addr_pt) const
  {
    ASSERT(byte_addr->is_const());
    mybitset const& byte_addr_mbs = byte_addr->get_mybitset_value();
    bv_const const& byte_addr_const = byte_addr_mbs.get_unsigned_mpz();
    for (size_t i = 0; i < m_addrs_that_may_differ.size(); i++) {
      size_t nbytes = m_addrs_that_may_differ.at(i).get_nbytes();
      bv_const const& addr_const = addr_pt->at(i).get_bv_const();
      if (bv_const_belongs_to_range(byte_addr_const, addr_const, nbytes)) {
        return i;
      }
    }
    return -1;
  }

  vector<vector<addr_that_may_differ_t>>
  refine_pset_using_addr_pt(vector<addr_that_may_differ_t> const& pset, point_ref const& addr_pt) const
  {
    map<bv_const, vector<addr_that_may_differ_t>> buckets;
    for (auto const& addr : pset) {
      size_t addr_idx = this->get_addr_idx(addr);
      buckets[addr_pt->at(addr_idx).get_bv_const()].push_back(addr);
    }
    vector<vector<addr_that_may_differ_t>> ret;
    for (auto const& b : buckets) {
      ret.push_back(b.second);
    }
    return ret;
  }

  void
  refine_partition_using_addr_pt(point_ref const& addr_pt)
  {
    vector<pair<vector<addr_that_may_differ_t>, partition_set_status_t>> new_partition; 
    CPP_DBG_EXEC(EQCLASS_ARR, cout << __func__ << " " << __LINE__ << "  " << this->partition_to_string());
    for (auto const& pelem: m_partition) {
      auto const& pset = pelem.first;
      vector<vector<addr_that_may_differ_t>> ret = refine_pset_using_addr_pt(pset, addr_pt);
      ASSERT(ret.size() >= 1);
      if (ret.size() > 1) {
        for (auto const& r : ret) {
          new_partition.push_back(make_pair(r, SRC_DST_AGREE/*pelem.second*/)); //optimistically set to agree
        }
      } else {
        new_partition.push_back(pelem);
      }
    }
    m_partition = new_partition;
    CPP_DBG_EXEC(EQCLASS_ARR, cout << __func__ << " " << __LINE__ << "  " << this->partition_to_string());
  }

  size_t get_addr_idx(addr_that_may_differ_t const& addr) const
  {
    for (size_t i = 0; i < m_addrs_that_may_differ.size(); i++) {
      if (m_addrs_that_may_differ.at(i).equals(addr)) {
        return i;
      }
    }
    NOT_REACHED();
  }

  bool arrays_differ_at_addr(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, addr_that_may_differ_t const& addr, sort_ref const& addr_sort) const
  {
    context *ctx = addr_sort->get_context();
    consts_struct_t const& cs = ctx->get_consts_struct();
    size_t addr_idx = this->get_addr_idx(addr);
    point_val_t const& pval = addr_pt->at(addr_idx);
    bv_const const& addr_const = pval.get_bv_const();
    expr_ref addr_const_ex = ctx->mk_bv_const(addr_sort->get_size(), addr_const);
    size_t nbytes = addr.get_nbytes();
    expr_ref data1 = arr1.array_select({ addr_const_ex }, nbytes, false);
    expr_ref data2 = arr2.array_select({ addr_const_ex }, nbytes, false);
    return data1 != data2;
  }

  void label_partition_sets_using_arrays(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, sort_ref const& addr_sort)
  {
    for (auto& pelem : m_partition) {
      if (pelem.second == SRC_DST_DISAGREE) {
        continue;
      }
      auto const& pset = pelem.first;
      ASSERT(pset.size() > 0);
      if (arrays_differ_at_addr(arr1, arr2, addr_pt, pset.at(0), addr_sort)) {
        pelem.second = SRC_DST_DISAGREE;
      }
    }
  }

  void refine_using_point(array_constant const& arr1, array_constant const& arr2, point_ref const& addr_pt, sort_ref const& addr_sort)
  {
    vector<addr_that_may_differ_t> differing_addrs;
    if (arrays_differ_beyond_addr_vals(arr1, arr2, addr_pt)) {
      DYN_DEBUG(eqclass_arr, cout << __func__ << ':' << __LINE__ << ": arrays_differ_beyond_addr_vals returned true.\narray1 = " << arr1.to_string() << "\narray2 = " << arr2.to_string() << endl);
      m_arrs_are_unequal = true;
      return;
    }

    this->refine_partition_using_addr_pt(addr_pt);
    this->label_partition_sets_using_arrays(arr1, arr2, addr_pt, addr_sort);
  }

  static indicator_varset_t get_indicator_varset_from_preconds(map<expr_id_t, pair<expr_ref, set<expr_ref>>> const& preconds, context *ctx)
  {
    indicator_varset_t ret;
    for (auto const& precond : preconds) {
      expr_ref const& var = precond.second.first;
      set<expr_ref> const& s = precond.second.second;
      map<expr_id_t, expr_ref> m;
      for (auto const& e : s) {
        m.insert(make_pair(e->get_id(), e));
      }
      ret.push_back(indicator_var_t(var, m));
    }
    return ret;
  }

  predicate_set_t get_preds(expr_ref const& arr1, expr_ref const& arr2) const
  {
    context *ctx = arr1->get_context();
    if (m_arrs_are_unequal) {
      DYN_DEBUG(eqclass_arr, cout << __func__ << ':' << __LINE__ << ": arrays are unequal; no arr predicate generated!\n");
      return predicate_set_t();
    }
    vector<sort_ref> addrs_sort = arr1->get_sort()->get_domain_sort();
    ASSERT(addrs_sort.size() == 1);
    sort_ref addr_sort = addrs_sort.at(0);
    map<expr_id_t, pair<expr_ref, set<expr_ref>>> preconds;
    vector<addr_that_may_differ_t> masks;
    for (size_t i = 0; i < m_partition.size(); i++) {
      auto const& pelem = m_partition.at(i);
      if (pelem.second == SRC_DST_AGREE) {
        continue;
      }
      auto const& pset = pelem.first;
      ASSERT(pset.size() > 0);
      if (pset.size() > 1) {
        memlabel_ref ml = pset_get_memlabel(pset);
        size_t nbytes = pset_get_nbytes(pset);
        expr_ref indicator_var = get_indicator_variable_for_pset_num(i, addr_sort, ctx);
        set<expr_ref> eqaddrs;
        for (auto const& addr : pset) {
          eqaddrs.insert(addr.get_addr());
        }
        preconds[indicator_var->get_id()] = make_pair(indicator_var, eqaddrs);
        masks.push_back(addr_that_may_differ_t(indicator_var, ml, nbytes));
      } else {
        masks.push_back(pset.at(0));
      }
    }

    ASSERT(arr1->get_operation_kind() == expr::OP_MEMMASK);
    ASSERT(arr2->get_operation_kind() == expr::OP_MEMMASK);
    ASSERT(arr1->get_args().at(1) == arr2->get_args().at(1));
    memlabel_t ml = arr1->get_args().at(1)->get_memlabel_value();

    expr_ref masked_arr1 = arr1;
    expr_ref masked_arr2 = arr2;
    for (auto const& mask : masks) {
      masked_arr1 = apply_mask(masked_arr1, mask);
      masked_arr2 = apply_mask(masked_arr2, mask);
    }
    expr_ref memeq = ctx->mk_memmasks_are_equal(masked_arr1, masked_arr2, ml);
    memeq = ctx->expr_do_simplify(memeq);
    indicator_varset_t ivset = get_indicator_varset_from_preconds(preconds, ctx);
    
    stringstream ss;
    ///ss << PRED_MEM_PREFIX << mem1->get_args().at(1)->get_memlabel_value().to_string();
    ss << PRED_MEM_PREFIX << "mem-nonstack";
    predicate_ref pred = predicate::mk_predicate_ref(precond_t(ivset, ctx), memeq, expr_true(ctx), ss.str());
    pred = pred->predicate_canonicalized();

    return { pred };
  }

  static expr_ref apply_mask(expr_ref const& arr, addr_that_may_differ_t const& mask)
  {
    context *ctx = arr->get_context();
    expr_ref const& addr = mask.get_addr();
    size_t nbytes = mask.get_nbytes();
    memlabel_ref const& ml = mask.get_memlabel();
    expr_ref zerobv = ctx->mk_zerobv(nbytes * BYTE_LEN);
    return ctx->mk_store(arr, ml->get_ml(), addr, zerobv, nbytes, false);
  }

  static memlabel_ref pset_get_memlabel(vector<addr_that_may_differ_t> const& pset)
  {
    ASSERT(pset.size() > 0);
    return pset.at(0).get_memlabel();
  }
  static size_t pset_get_nbytes(vector<addr_that_may_differ_t> const& pset)
  {
    ASSERT(pset.size() > 0);
    return pset.at(0).get_nbytes();
  }
  static expr_ref indicator_var_precond_get_expr(map<expr_id_t, pair<expr_ref, set<expr_ref>>> const& preconds, context *ctx)
  {
    expr_ref ret = expr_true(ctx);
    for (auto const& precond : preconds) {
      expr_ref indicator_var = precond.second.first;
      expr_ref cur_precond_expr = expr_false(ctx);
      for (const auto& possible_var : precond.second.second) {
        cur_precond_expr = expr_or(cur_precond_expr, expr_eq(indicator_var, possible_var));
      }
      ret = expr_and(ret, cur_precond_expr);
    }
    return ret;
  }
};

template<typename T_PC, typename T_N, typename T_E, typename T_PRED>
class invariant_state_eqclass_arr_t : public invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>
{
private:
  vector<addr_that_may_differ_t> m_addrs_that_may_differ;
  expr_group_ref m_addrs_expr_group;
  point_set_t m_addr_point_set; //addr points are maintained in the same order as the corresponding array points in m_point_set
  long m_id; //a unique id is assigned to each eqclass_arr_t instance; used for naming quantified variables
  static long m_eqclass_arr_id; //global counter used to assign a unique id to each eqclass instance
public:
  invariant_state_eqclass_arr_t(expr_group_ref const &exprs_to_be_correlated, map<graph_loc_id_t, graph_cp_location> const& locs, context *ctx) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(exprs_to_be_correlated, ctx), m_addrs_that_may_differ(compute_addrs_that_may_differ(exprs_to_be_correlated, locs, ctx)), m_addrs_expr_group(compute_addrs_expr_group_from_addrs_that_may_differ(m_addrs_that_may_differ)), m_addr_point_set(m_addrs_expr_group, ctx), m_id(m_eqclass_arr_id++)
  {
    ASSERT(this->m_point_set.size() == m_addr_point_set.size());
  }
  invariant_state_eqclass_arr_t(invariant_state_eqclass_arr_t const& other) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(other),
                                                                              m_addrs_that_may_differ(other.m_addrs_that_may_differ),
                                                                              m_addrs_expr_group(other.m_addrs_expr_group),
                                                                              m_addr_point_set(other.m_addr_point_set),
                                                                              m_id(other.m_id)
  {
    ASSERT(this->m_point_set.size() == m_addr_point_set.size());
  }

  invariant_state_eqclass_arr_t(istream& is, string const& inprefix/*, state const& start_state*/, context* ctx, map<graph_loc_id_t, graph_cp_location> const& locs) : invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>(is, inprefix/*, start_state*/, ctx), m_addrs_that_may_differ(compute_addrs_that_may_differ(this->get_exprs_to_be_correlated(), locs, ctx)), m_addrs_expr_group(compute_addrs_expr_group_from_addrs_that_may_differ(m_addrs_that_may_differ)), m_addr_point_set(m_addrs_expr_group, ctx), m_id(m_eqclass_arr_id++)
  { }

  virtual invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED> *clone() const override
  {
    PRINT_PROGRESS("%s %s() %d:\n", __FILE__, __func__, __LINE__);
    return new invariant_state_eqclass_arr_t<T_PC, T_N, T_E, T_PRED>(*this);
  }

  virtual void invariant_state_eqclass_to_stream(ostream& os, string const& inprefix) const override
  {
    string prefix = inprefix + "type arr";
    os << "=" + prefix + "\n";
    this->invariant_state_eqclass_to_stream_helper(os, prefix + " ");
  }

private:
  //returns TRUE if we are done
  virtual bool recompute_preds_for_points(bool last_proof_success, unordered_set<shared_ptr<T_PRED const>>& preds) override
  {
    autostop_timer func_timer(string("invariant_state_eqclass_arr.") + __func__);
    if (last_proof_success) {
      return true;
    }
    if (this->m_timeout_info.timeout_occurred()) {
      preds = {};
      return true;
    }
    preds = compute_preds_for_arr_points();
    return false;
  }

  static vector<addr_that_may_differ_t>
  compute_addrs_that_may_differ(expr_group_ref const& exprs_to_be_correlated, map<graph_loc_id_t, graph_cp_location> const& locs, context *ctx)
  {
    ASSERT(exprs_to_be_correlated->size() <= 2);
    if (exprs_to_be_correlated->size() < 2) {
      return vector<addr_that_may_differ_t>();
    }
    vector<expr_ref> const& exprs = exprs_to_be_correlated->get_expr_vec();
    ASSERT(exprs.at(0)->get_operation_kind() == expr::OP_MEMMASK);
    memlabel_t const& ml = exprs.at(0)->get_args().at(1)->get_memlabel_value();
    set<addr_that_may_differ_t> ret_set;
    vector<addr_that_may_differ_t> ret;
    for (auto const& loc : locs) {
      if (   loc.second.is_memslot()
          && !memlabel_t::memlabel_intersection_is_empty(ml, loc.second.m_memlabel->get_ml())) {
        addr_that_may_differ_t new_a = addr_that_may_differ_t(loc.second.m_addr, loc.second.m_memlabel, loc.second.m_nbytes);
        bool is_unique = true;
        for(auto const &a : ret){
          if(a.equals(new_a)) {
            is_unique = false;
            break;
          }
        }
        if(is_unique)
          ret.push_back(new_a);
      }
    }
    return ret;
  }

  predicate_set_t
  compute_preds_for_arr_points() const
  {
    point_set_t const& point_set = this->m_point_set;
    expr_group_ref const& exprs_to_be_correlated = this->get_exprs_to_be_correlated();
    context *ctx = this->m_ctx;
    CPP_DBG_EXEC(EQCLASS_ARR, cout << __func__ << " " << __LINE__ << ": entry\n");
    //list<expr_idx_t> const &arr_indices = m_exprs->get_arr_indices();
    ASSERT(exprs_to_be_correlated->size() <= 2);
    if (exprs_to_be_correlated->size() == 0) {
      return predicate_set_t();
    }
    if (exprs_to_be_correlated->size() == 1) {
      return predicate_set_t(); //this can happen if only esp is present
    }
    ASSERT(exprs_to_be_correlated->size() == 2);

    expr_ref const& arr1_expr = exprs_to_be_correlated->at(0);
    expr_ref const& arr2_expr = exprs_to_be_correlated->at(1);

    vector<sort_ref> addrs_sort = arr1_expr->get_sort()->get_domain_sort();
    ASSERT(addrs_sort.size() == 1);
    sort_ref addr_sort = addrs_sort.at(0);

    addr_partition_t addr_partition(this->m_id, m_addrs_that_may_differ);
    vector<point_ref> const& arr_points_vec = point_set.get_points_vec();
    vector<point_ref> const& addr_points_vec = m_addr_point_set.get_points_vec();
    //cout << "arr_points_vec.size() = " << arr_points_vec.size() << endl;
    //cout << "addr_points_vec.size() = " << addr_points_vec.size() << endl;
    ASSERT(this->point_sets_are_consistent());
    CPP_DBG_EXEC(EQCLASS_ARR, 
        for(auto const &a : m_addrs_that_may_differ) 
          cout << __func__ << "  " << __LINE__ << "  " << a.to_string();
          cout << __func__ << "  " << __LINE__ << " \n " << point_set.point_set_to_string(" ") << endl;
          cout << __func__ << "  " << __LINE__ << " \n " << m_addr_point_set.point_set_to_string(" ") << endl;
    );
    for (size_t i = 0; i < arr_points_vec.size(); i++) {
      auto const& arr_pt = arr_points_vec.at(i);
      auto const& addr_pt = addr_points_vec.at(i);
      point_val_t const& pv1 = arr_pt->at(0);
      point_val_t const& pv2 = arr_pt->at(1);
      if (pv1.get_arr_const() != pv2.get_arr_const()) {
        //CPP_DBG_EXEC(POINT_SET, cout << __func__ << " " << __LINE__ << ": arrays mismatch.\n"
        //                             << expr_string(exprs_to_be_correlated->at(0/*idx1*/)) << " = " << pt->at(0/*idx1*/).get_arr_const().to_string() << '\n'
        //                             << expr_string(exprs_to_be_correlated->at(1/*idx2*/)) << " = " << pt->at(1/*idx2*/).get_arr_const().to_string() << endl);
        //CPP_DBG_EXEC(POINT_SET, cout << __func__ << " " << __LINE__ << ": returning ret.size() = " << ret.size() << '\n');
        //return ret; // counter example found
        addr_partition.refine_using_point(*pv1.get_arr_const(), *pv2.get_arr_const(), addr_pt, addr_sort);
      }
    }

    auto ret = addr_partition.get_preds(arr1_expr, arr2_expr);
    CPP_DBG_EXEC(EQCLASS_ARR, cout << __func__ << " " << __LINE__ << ": returning preds\n"; predicate::predicate_set_to_stream(cout, "", ret));
    return ret;
  }

  static expr_group_ref
  compute_addrs_expr_group_from_addrs_that_may_differ(vector<addr_that_may_differ_t> const& addrs_that_may_differ)
  {
    vector<expr_ref> vec;
    for (auto const& a : addrs_that_may_differ) {
      vec.push_back(a.get_addr());
    }
    return mk_expr_group(expr_group_t::EXPR_GROUP_TYPE_MEMADDRS_DIFF, vec);
  }

  virtual bool
  invariant_state_eqclass_add_point_using_CE(nodece_ref<T_PC, T_N, T_E> const &nce, graph_with_guessing<T_PC, T_N, T_E, T_PRED> const &g) override
  {
    m_addr_point_set.add_point_using_CE<T_PC, T_N, T_E, T_PRED>(nce, g);
    bool ret = this->invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_add_point_using_CE(nce, g);
    ASSERT(this->point_sets_are_consistent());
    return ret;
  }

  virtual void
  invariant_state_eqclass_pop_back_last_point() override
  {
    ASSERT(this->point_sets_are_consistent());
    this->invariant_state_eqclass_t<T_PC, T_N, T_E, T_PRED>::invariant_state_eqclass_pop_back_last_point();
    m_addr_point_set.pop_back();
    ASSERT(this->point_sets_are_consistent());
  }

  bool point_sets_are_consistent() const
  {
    point_set_t const& point_set = this->m_point_set;
    vector<point_ref> const& arr_points_vec = point_set.get_points_vec();
    vector<point_ref> const& addr_points_vec = m_addr_point_set.get_points_vec();
    return    arr_points_vec.size() == addr_points_vec.size()
           || arr_points_vec.size() + 1 == addr_points_vec.size() //this can happen if we are in the middle of invariant_state_eqclass_add_point_using_CE (recall that our base class's version of this function calls recompute_preds_for_points()
    ;
  }

  bool check_stability() const override { return !this->get_preds().empty(); }
};

//class pcpair;
//class corr_graph_node;
//class corr_graph_edge;
//class predicate;
//template<>
//extern long invariant_state_eqclass_arr_t<pcpair, corr_graph_node, corr_graph_edge, predicate>::m_eqclass_arr_id;

}
